# MIDI Morpher — Claude Code Instructions

## Project Overview

MIDI Morpher is an ESP32-S3 based MIDI controller pedal with 6 independently configurable footswitches (4 onboard + 2 external via jack), a rotary encoder, two potentiometers, an SSD1306 OLED display, and a built-in WiFi web interface. Its standout feature is a MIDI CC modulation/morphing engine. Output is via mini-TRS MIDI, USB-C MIDI, BLE MIDI. The pedal supports 6 presets that store complete configurations including per-preset BPM.

**Firmware target:** ESP32-S3-N16R8, Arduino framework.

---

## Hardware

- **MCU:** ESP32-S3-N16R8 devboard, native USB (Arduino framework)
- **Display:** SSD1306 OLED (I2C, 128×64)
- **Controls:** 4 onboard footswitches (FS1–FS4), 2 external footswitches via jack (ExtFS1, ExtFS2), rotary encoder with push button, 2 pots (UP speed, DOWN speed), MS2 latching toggle, LOCK switch, PRESET momentary button
- **LEDs:** 6 footswitch LEDs (repurposed as preset indicators), 1 activity LED, 1 tempo LED, 1 NeoPixel RGB
- **External:** expression pedal in/out jacks
- **I/O:** mini-TRS MIDI out + in (MIDI Thru), USB-C MIDI out/thru, BLE MIDI in/out, expression pedal out
- **WiFi:** built-in AP mode — SSID "MIDI Morpher", password "midimorpher", UI at 192.168.4.1. Always on except when "Lock settings" is engaged.
- **BLE MIDI:** standard Apple BLE-MIDI service (UUID `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`), advertised as `MIDI Morpher`. Always on — not affected by the LOCK switch. Coexists with the WiFi AP via radio time-slicing.

---

## Preset System

- 6 presets, each storing: global MIDI channel, per-preset BPM, + per-footswitch modeIndex, midiNumber, fsChannel, rampUpMs, rampDownMs
- NVS namespace `"presets"` — key `"prs"` (PresetData[6] blob) + key `"act"` (active preset index)
- **No auto-save.** `markStateDirty()` only sets `presetDirty = true` (used for display indicator). Settings are volatile until explicitly saved.
- **PRESET button short press:** `applyPreset((activePreset+1)%6, pedal)` — loads next preset live
- **PRESET button long press (≥ PRESET_SAVE_HOLD_MS = 1500 ms):** `saveCurrentPreset(pedal)` — writes to NVS; blocked when LOCK engaged
- `presetDirty = true` is set by any setting change (encoder, pots, web UI); cleared on preset load or save
- On first boot (no `"presets"` namespace): factory defaults for all 6 slots; old `"pedal"` namespace (same struct layout as `PresetData`) migrated into slot 0 if found

### Footswitch LEDs (repurposed)

- `FSButton::_setLED()` only updates the `ledState` bool — does **not** drive GPIO
- GPIO is driven exclusively by `updatePresetLEDs()` in the main loop: only `buttons[activePreset].ledPin` is HIGH
- `updateActivityLed()` drives GPIO 45 (`ACTIVITY_LED_PIN`): reflects `buttons[lastActiveFSIndex].ledState`
- `lastActiveFSIndex` (int8_t, -1 = none) in `PedalState` — updated in main loop when a footswitch registers a new press

---

## Modulation Engine Behavior

- **RAMPER:** exponential curve (`SHAPE_EXP`), CC 0→127 on press. On release (momentary), ramps back down to 0 gradually using the DOWN pot speed — does not snap. Latching holds until next press, then ramps back down.
- **RAMPER Inverted:** resting position is 127, not 0. Same gradual return logic applies in reverse.
- **STEPPER:** linear curve (`SHAPE_LINEAR`), moves in discrete quantized steps. Return uses the DOWN pot speed.
- **STEPPER Inverted:** resting position is 127. Same gradual return logic applies in reverse.
- **RANDOM STEPPER:** linear curve. Steps to random CC values continuously; does not stop at 127. Return behavior follows the same proportional speed rule.
- **LFO:** continuous 0–127–0 sweep. Wave types: sine (`SHAPE_SINE`, raised-cosine), triangle (`SHAPE_LINEAR`), square (`SHAPE_SQUARE`). UP pot controls rise speed, DOWN pot controls descent. Momentary: gradually return to 0 on release. Latching: continues until next press, then gradually returns to 0.
- **UP/DOWN pots:** control modulation speed for the currently selected footswitch only.

### Shared modulator — per-press sync

There is one `MidiCCModulator` shared by all 6 footswitches (`PedalState::modulator`). On every press, `FSButton::_applyPressState` must sync `modType`, `latching`, `rampShape`, `restingHigh`, ramp times, channel, and CC number from the button's `modMode`. Without this, `rampShape` and `restingHigh` would only update on mode-cycle/preset-load, causing e.g. LFO Tri on FS2 to inherit `SHAPE_SINE` from a previously configured LFO Sine on FS1.

### Proportional Return Speed

- Return speed = DOWN pot speed × (current CC value / 127)
- Ensures return duration feels consistent regardless of when the footswitch was released.

### Encoder acceleration

`handleEncoder` multiplies `delta` by a time-based factor for CC/PC/Note selection (range 0–127):

- dt < 15 ms → ×8
- dt < 30 ms → ×4
- dt < 60 ms → ×2
- else → ×1

Scene modes (range 0–7 / 0–4), per-FS channel select, and global channel select intentionally skip acceleration because their ranges are too small to accelerate cleanly.

---

## Basic Mode Behavior

- **PC:** momentary only. Sends Program Change on press.
- **CC momentary:** hold sends 127, release sends 0.
- **CC latching:** alternates between 0 and 127 on each press.
- **NOTE:** momentary only. Note On on press, Note Off on release.
- **Scene/Snapshot modes (Helix, QC, Fractal, Kemper):** encoder selects the target value or CC number depending on the unit.

---

## Global Settings (`src/globalSettings.h`)

Non-preset settings persisted to NVS namespace `"globals"`. Accessed via the main menu (encoder button short press, no FS held).

- **MIDI routing flags** (6-bit): 6 toggleable pairs — DIN→USB, USB→DIN, DIN→BLE, BLE→DIN, USB→BLE, BLE→USB. Default: `ROUTE_ALL` (0x3F, full mesh).
- **LED mode**: On (preset LED always lit + slow blink if dirty), Conservative (minimal; brief blink on load/save), Off.
- **Tempo LED**: enabled/disabled.
- **NeoPixel**: enabled/disabled.
- **Display brightness**: 0–100%, persisted.
- **Display timeout**: 2 s / 5 s / 10 s / always-on.
- **Pot 1 / Pot 2 CC**: 0–127 or `POT_CC_OFF` (0xFF) to disable CC sending.
- **Expression pedal CC**: 0–127.
- **Expression pedal calibration**: min/max ADC recorded during a 5-second sweep.
- **Exp wake display**: when enabled, expression pedal movement briefly shows value on OLED.

---

## Memory

- Settings live in 6 preset slots in NVS (namespace `"presets"`).
- **No auto-save.** Changes are not persisted until the user saves the preset via long-press or web UI.
- `markStateDirty()` is kept as a call-site-compatible wrapper that sets `presetDirty = true`.
- No factory reset functionality needed or planned.

---

## Expression I/O

- **Expression out:** mirrors all modulation output via AD5292-BRUZ-20 digital pot.
- **Expression in:** if a pedal is connected, mirrors its value to expression out and sends it as MIDI CC20.

---

## MIDI Thru

- mini-TRS MIDI input acts as MIDI Thru — passes incoming MIDI through to output for daisy-chaining.
- In the main loop, while reading DIN-MIDI bytes and before forwarding, `0xF8` bytes are tapped into `midiClock.receiveClock()`. USB-MIDI packets with `CIN == 0x0F` and first byte `0xF8` are also forwarded to the clock.

---

## MIDI Clock (`src/clock/midiClock.h`)

- 24 ppqn standard. Internal BPM default `DEFAULT_BPM = 120`, range `BPM_MIN–BPM_MAX` (20–300).
- **External sync:** incoming `0xF8` bytes set `externalSync = true` and drive BPM from the interval between ticks. If no `0xF8` is received for `CLOCK_SYNC_TIMEOUT_MS = 2000 ms`, `externalSync` flips back to false and internal BPM resumes.
- **Tap tempo:** `FootswitchMode::TapTempo` calls `midiClock.receiveTap()` on press and the main loop shows the `displayTapTempo(midiClock.bpm)` screen.
- **Tempo LED:** GPIO 46 pulsed on the beat by `midiClock.tick()`.
- **Clock-synced ramps:** `rampUpMs` / `rampDownMs` are `uint32_t`. Bit 31 = `CLOCK_SYNC_FLAG`. When set, the low byte is a note-value index into `noteValueNames[]` (17 values: 1/32T … 2/1, incl. dotted & triplets). `midiClock.syncToMs(raw)` resolves it to ms at current BPM. Plain ms values (0–5000) always have bit 31 clear.
- **Where resolved:** `_applyPressState` resolves sync values to `modulator.rampUpTimeMs` / `rampDownTimeMs` on press. The main loop re-resolves the active button's ramp times each tick so live BPM changes affect in-flight modulation.
- **Wire loop:** `midiClock.tick()` is called every loop iteration from `midi_morpher.ino`.

---

## WiFi Web Interface

- AP SSID: `MIDI Morpher`, password: `midimorpher`
- UI served at `http://192.168.4.1` — dark-themed SPA embedded as PROGMEM string in `src/wifi/webUI.h`
- **WiFi is always on** at boot. It turns off only when the LOCK switch is engaged, and restarts when LOCK is disengaged. There is no user-toggleable WiFi setting.
- **Captive portal:** `DNSServer` catches all DNS queries and redirects to `192.168.4.1`. `webServer.onNotFound(handleCaptivePortal)` returns a 302 to `/` for any unregistered URL, including OS probes like `/generate_204`, `/hotspot-detect.html`, `/ncsi.txt`. Phones/laptops then show a "Sign in to network" prompt that opens the UI automatically.

### REST API (`src/wifi/webServer.h`)

| Method | Path                      | Body                                                       | Action                                                                             |
| ------ | ------------------------- | ---------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| GET    | `/api/state`              | —                                                          | Full state JSON: channel, activePreset, presetDirty, bpm, externalSync, buttons[6] |
| GET    | `/api/presets`            | —                                                          | All 6 PresetData slots + activePreset index                                        |
| GET    | `/api/global`             | —                                                          | All global settings as JSON                                                        |
| POST   | `/api/global`             | global settings fields                                     | Update global settings (routing, LED mode, display, pot CCs, etc.)                 |
| POST   | `/api/channel`            | `{"channel":0}`                                            | Set global MIDI channel (0–15)                                                     |
| POST   | `/api/button/:id`         | `{modeIndex, midiNumber, fsChannel, rampUpMs, rampDownMs}` | Update footswitch config                                                           |
| POST   | `/api/button/:id/press`   | —                                                          | Simulate footswitch press (`simulatePress(true,…)`)                                |
| POST   | `/api/button/:id/release` | —                                                          | Simulate footswitch release (`simulatePress(false,…)`)                             |
| POST   | `/api/bpm`                | `{"bpm":120}`                                              | Set internal BPM (20–300); clears external sync                                    |
| GET    | `/api/poll`               | —                                                          | Lightweight poll: `{bpm, externalSync, activePreset, presetDirty}`                 |
| POST   | `/api/pot`                | `{"id":0,"value":64}`                                      | Send CC 20 (id=0) or CC 21 (id=1) directly                                         |
| POST   | `/api/preset/load/:id`    | —                                                          | `applyPreset(id, pedal)` — apply preset live                                       |
| POST   | `/api/preset/save/:id`    | —                                                          | `saveCurrentPreset(pedal)` — write to NVS; blocked when locked                     |
| GET    | `/api/expcal`             | —                                                          | Poll calibration status `{running, min, max}`                                      |
| POST   | `/api/expcal`             | —                                                          | Start 5-second expression pedal calibration sweep                                  |
| GET    | `/dismiss`                | —                                                          | Clean shutdown of WiFi AP                                                          |

- `fsChannel` of 255 (0xFF) means "follow global channel"
- `handleButtonPress` also sets `_webPedal->lastActiveFSIndex = idx` to update the activity LED
- `rampUpMs` / `rampDownMs` are `uint32_t`: plain ms (0–5000) or `CLOCK_SYNC_FLAG | noteIdx` (bit 31 set). Parsed via `jsonUint`.
- **Server-side guardrail:** `handlePostButton` clamps `midiNumber` to `btn.modMode.sceneMaxVal` for scene modes (7 or 4), 127 otherwise.

### Web UI features

- Global MIDI channel dropdown, **editable BPM** (number input + nudge buttons) with EXT indicator when slaved to incoming MIDI clock; BPM input disabled during external sync
- **Auto-polling** (2 s interval via `GET /api/poll`): keeps BPM, external sync, preset dirty, and active preset in sync with hardware changes
- Preset bar (P1–P6 buttons + Save button); active preset highlighted; `● Unsaved` badge when dirty
- POT1 / POT2 virtual sliders (send CC 20 / CC 21)
- **Global settings grid**: LED mode, tempo LED, NeoPixel, display brightness, display timeout, pot CC numbers, expression CC, expression calibration button
- 6 footswitch cards, each with:
    - Full-width trigger button at bottom (hold for momentary modes; click-toggle for latching; mouseleave auto-releases momentary)
    - Mode dropdown with `<optgroup>` categories (Basic, Ramper, LFO, Stepper, Random, Scenes, Utility)
    - MIDI number input — **1-indexed** (1–128 for CC/PC/Note, 1–8 / 1–5 for scenes); converted to 0-based on POST
    - Mode-aware input `max` attribute + blur clamp + pre-POST clamp to prevent out-of-range values
    - Input is disabled when mode is Tap Tempo or System
    - Channel dropdown (Global or Ch 1–16)
    - Ramp Up / Ramp Down: slider (ms) or note-value dropdown, toggled by inline "sync" checkbox. Visible for modulation modes only.

---

## Libraries

External libraries (install via Arduino Library Manager):

| Library              | Min version |
| -------------------- | ----------- |
| Adafruit GFX Library | 1.11.0      |
| Adafruit SSD1306     | 2.5.0       |
| Adafruit NeoPixel    | 1.12.0      |
| NimBLE-Arduino       | 2.0.0       |

Built-in with ESP32 Arduino core (no install needed): `SPI.h`, `Wire.h`, `Preferences.h`, `USB.h`, `USBMIDI.h`, `WebServer.h`, `WiFi.h`.

Full dependency manifest: `libraries.json` at project root.

### Pot Noise Filtering (`src/potentiometers/analogReadHelpers.h`)

- Raw read: 8-sample average per read (`readPotAvg`)
- EMA filter: alpha = 1/4 — `filtered = filtered - (filtered >> 2) + (raw >> 2)`
- Deadband: 20 counts on 12-bit ADC (0–4095) before a change is reported

---

## Coding Rules (follow strictly)

For every feature request, present a plan (files to touch, approach, what won't change) and wait for approval before writing any code. Only implement what was explicitly requested. Do not refactor, reorganize, or "improve" unrelated code. Prefer adding small, isolated code over restructuring existing logic. If you can solve something with fewer lines, do that. If a feature is already implemented and working, leave it alone unless the task explicitly involves it or its current implementation is flawed. Keep memory usage in mind (heap, stack). Avoid dynamic allocation in hot paths. Use `millis()` for timing, never `delay()` in main loop.
`FSButton::_setLED()` only updates `ledState` — never drives GPIO directly. All footswitch LED GPIOs are driven by `updatePresetLEDs()` in the main loop. Do not bypass this.
