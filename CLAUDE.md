# MIDI CCMorpher

MIDI CCMorpher is an ESP32-S3 based MIDI controller pedal with 6 independently configurable footswitches (4 onboard + 2 external via jack), 128 presets for all footswitches, a rotary encoder, two potentiometers, an SSD1306 OLED display, and a built-in WiFi web interface. Its standout feature is a MIDI CC modulation/morphing engine. Output is via mini-TRS MIDI, USB-C MIDI, BLE MIDI. Each preset can store completely different configurations per footswitch, including per-preset BPM.
Firmware target: ESP32-S3-N16R8, Arduino framework.

---

## Hardware

- ESP32-S3-N16R8 devboard, native USB (Arduino framework)
- SSD1306 OLED (I2C, 128×64)
- 4 onboard footswitches (FS1–FS4), 2 external footswitches via jack (ExtFS1, ExtFS2), rotary encoder with push button
- 7 Neopixels - one per footswitch + tempo
- expression pedal in jack
- mini-TRS MIDI out + in (MIDI Thru), USB-C MIDI out/thru, BLE MIDI in/out
- built-in AP mode — SSID "MIDI Morpher", password "midimorpher", UI at 192.168.4.1. Always on except when "Lock settings" is engaged.
- standard Apple BLE-MIDI service (UUID `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`), advertised as `MIDI Morpher`. Always on — not affected by the LOCK switch. Coexists with the WiFi AP via radio time-slicing.

---

## Preset System

(Don't case about previously stored presets. Always opt for better code design than preserving current presets. All of the presets can be erased, reformatted etc.)

- 128 presets, each storing: global MIDI channel, per-preset BPM, + per-footswitch mode, midiNumber, fsChannel, rampUpMs, rampDownMs and global settings
- settings are volatile until explicitly saved
- `presetDirty = true` is set by any setting change (encoder, web UI); cleared on preset load or save
- On first boot (no `"presets"` namespace): factory defaults for all 6 slots
- Holding encoder button triggers preset save

---

## Modulation Engine Behavior

- **RAMPER:** exponential curve (`SHAPE_EXP`), CC 0→127 on press. On release (momentary), ramps back down to 0 gradually using the DOWN pot speed — does not snap. Latching holds until next press, then ramps back down. Inverted mode: resting position is 127, not 0. Same gradual return logic applies in reverse.
- **STEPPER:** linear curve (`SHAPE_LINEAR`), moves in discrete quantized steps. Return uses the DOWN pot speed. Inverted mode: resting position is 127. Same gradual return logic applies in reverse.
- **RANDOM STEPPER:** linear curve. Steps to random CC values continuously; does not stop at 127. Return behavior follows the same proportional speed rule.
- **LFO:** continuous 0–127–0 sweep. Wave types: sine (`SHAPE_SINE`, raised-cosine), triangle (`SHAPE_LINEAR`), square (`SHAPE_SQUARE`). UP pot controls rise speed, DOWN pot controls descent. Momentary: gradually return to 0 on release. Latching: continues until next press, then gradually returns to 0.

### Shared or per-fs modulator

`MidiCCModulator` is share across footswitches or each footswitch is assigned a different one. Switchable in global settings menu.

### Proportional Return Speed

- Return speed = DOWN pot speed × (current CC value / 127)
- Ensures return duration feels consistent regardless of when the footswitch was released.

### Encoder acceleration

`handleEncoder` multiplies `delta` by a time-based factor for CC/PC/Note selection (range 0–127).
Scene modes (range 0–7 / 0–4), per-FS channel select, and global channel select intentionally skip acceleration.

---

## Basic Mode Behavior

- **PC:** momentary only. Sends Program Change on press.
- **CC:** hold sends 127, release sends 0. Latching mode - alternates between 127 and 0. Single mode continuously sends single CC value on each press.
- **NOTE:** momentary only. Note On on press, Note Off on release.
- **Scene/Snapshot modes (Helix, QC, Fractal, Kemper):** encoder selects the target value or CC number depending on the unit. Scroll mode - scrolls through values from 1 up to user-defined max in a loop.

---

## Global Settings (`src/globalSettings.h`)

Non-preset settings persisted to NVS namespace `"globals"`. Accessed via the main menu (encoder button short press, no FS held).

- **MIDI routing flags** (6-bit): 6 toggleable pairs — DIN→USB, USB→DIN, DIN→BLE, BLE→DIN, USB→BLE, BLE→USB. Default: `ROUTE_ALL` (0x3F, full mesh).
- **Tempo LED**: enabled/disabled.
- **Display timeout**: 2 s / 5 s / 10 s / always-on.
- **Expression pedal CC**: 0–127 or OFF.
- **Expression pedal calibration**: min/max ADC recorded during a 5-second sweep.
- **Exp wake display**: when enabled, expression pedal movement briefly shows value on OLED.

---

## Memory

- Settings live in 128 preset slots in NVS (namespace `"presets"`).
- **No auto-save.** Changes are not persisted until the user saves the preset via long-press or web UI.
- `markStateDirty()` is kept as a call-site-compatible wrapper that sets `presetDirty = true`.

---

## Expression Input

- **Expression in:** if a pedal is connected, mirrors its value to expression out and sends it as MIDI CC, configurable in global settings.

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

---

## Libraries

External libraries (install via Arduino Library Manager):
Full dependency manifest: `libraries.json` at project root.

---

## Coding Rules (follow strictly)

Be conservative with tokens, be precise. For every feature request, present a plan (files to touch, approach) and wait for approval before writing any code. Only implement what was explicitly requested. Do not refactor, reorganize, or "improve" unrelated code. Prefer adding small, isolated code over restructuring existing logic. If you can solve something with fewer lines, do that. If a feature is already implemented and working, leave it alone unless the task explicitly involves it or its current implementation is flawed. Keep memory usage in mind (heap, stack). Avoid dynamic allocation in hot paths. Use `millis()` for timing, never `delay()` in main loop.
