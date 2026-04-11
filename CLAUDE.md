# MIDI Morpher — Claude Code Instructions

## Project Overview

MIDI Morpher is an ESP32-S3 based MIDI controller pedal with 6 independently configurable footswitches (4 onboard + 2 external via jack), a rotary encoder, two potentiometers, an SSD1306 OLED display, and a built-in WiFi web interface. Its standout feature is a MIDI CC modulation/morphing engine. Output is via mini-TRS MIDI, USB-C MIDI, and an analog expression output (via AD5292-BRUZ-20 digital potentiometer). The pedal supports 6 presets that store complete configurations.

**Firmware target:** ESP32-S3-N16R8, Arduino framework.

---

## Hardware

- **MCU:** ESP32-S3-N16R8 devboard, native USB (Arduino framework)
- **Display:** SSD1306 OLED (I2C, 128×64)
- **Digital pot:** AD5292-BRUZ-20 (SPI, 1024 positions) — drives analog expression output
- **Controls:** 4 onboard footswitches (FS1–FS4), 2 external footswitches via jack (ExtFS1, ExtFS2), rotary encoder with push button, 2 pots (UP speed, DOWN speed), MS2 latching toggle, LOCK switch, PRESET momentary button
- **LEDs:** 6 footswitch LEDs (repurposed as preset indicators), 1 activity LED, 1 WiFi status LED, 1 NeoPixel RGB
- **External:** expression pedal in/out jacks
- **I/O:** mini-TRS MIDI out + in (MIDI Thru), USB-C MIDI out/thru, expression pedal out
- **WiFi:** built-in AP mode — SSID "MIDI Morpher", password "midimorpher", UI at 192.168.4.1. Always on except when LOCK switch is engaged.

### Pin Assignments

| Pin | Role |
|-----|------|
| 1 | POT1 (UP speed) |
| 2 | POT2 (DOWN speed) |
| 3 | ExtFS2 LED (preset indicator) |
| 4 | FS1 |
| 5 | FS1 LED (preset indicator) |
| 6 | FS2 |
| 7 | FS2 LED (preset indicator) |
| 8 | ExtFS2 |
| 9 | Encoder A |
| 10 | Encoder B |
| 11 | Encoder button |
| 12 | Expression In |
| 13 | MS2 (momentary/latching toggle switch) |
| 14 | FS3 |
| 15 | FS3 LED (preset indicator) |
| 16 | FS4 |
| 17 | ExtFS1 |
| 18 | ExtFS1 LED (preset indicator) |
| 19 | PRESET button (momentary, INPUT_PULLUP) |
| 20 | Activity LED |
| 21 | FS4 LED (preset indicator) |
| 22 | WiFi status LED |
| 36 | MIDI RX |
| 38 | Digipot CS (SYNC) |
| 39 | Digipot SCK |
| 40 | Digipot MOSI (SDI) |
| 41 | SDA (OLED) |
| 42 | SCL (OLED) |
| 43 | MIDI TX |
| 47 | LOCK switch |
| 48 | NeoPixel (onboard RGB) |

### AD5292-BRUZ-20 SPI Details

- Interface: SPI_MODE1, 1 MHz, MSB first
- 16-bit transfer: bits[15:10] = command, bits[9:0] = position (0–1023)
- Command `0b000111` + data `0b10`: unlocks RDAC write protect (must send on init)
- Command `0b000001` + position: writes wiper position
- MIDI 0–127 maps to position 0–1023 via `map()`
- SDO (MISO) not used — leave unconnected

---

## Footswitch Modes (full ordered list)

1. PC
2. CC momentary
3. CC latching
4. NOTE
5. RAMPER momentary
6. RAMPER latching
7. RAMPER inverted momentary
8. RAMPER inverted latching
9. LFO Sine momentary
10. LFO Sine latching
11. LFO Triangle momentary
12. LFO Triangle latching
13. LFO Square momentary
14. LFO Square latching
15. STEPPER momentary
16. STEPPER latching
17. STEPPER inverted momentary
18. STEPPER inverted latching
19. RANDOM STEPPER momentary
20. RANDOM STEPPER latching
21. RANDOM STEPPER inverted momentary
22. RANDOM STEPPER inverted latching
23. Helix Snapshots (CC69, values 0–7)
24. Quad Cortex Scenes (CC43, values 0–7)
25. Fractal Scenes (CC34, values 0–7)
26. Kemper Slots (CC50–CC54, value 1 — encoder picks CC number)

---

## Preset System

- 6 presets, each storing: global MIDI channel + per-footswitch modeIndex, midiNumber, fsChannel, rampUpMs, rampDownMs
- NVS namespace `"presets"` — key `"prs"` (PresetData[6] blob) + key `"act"` (active preset index)
- **No auto-save.** `markStateDirty()` only sets `presetDirty = true` (used for display indicator). Settings are volatile until explicitly saved.
- **PRESET button short press:** `applyPreset((activePreset+1)%6, pedal)` — loads next preset live
- **PRESET button long press (≥ PRESET_SAVE_HOLD_MS = 1500 ms):** `saveCurrentPreset(pedal)` — writes to NVS; blocked when LOCK engaged
- `presetDirty = true` is set by any setting change (encoder, pots, web UI); cleared on preset load or save
- On first boot (no `"presets"` namespace): factory defaults for all 6 slots; old `"pedal"` namespace (same struct layout as `PresetData`) migrated into slot 0 if found

### Footswitch LEDs (repurposed)

- `FSButton::_setLED()` only updates the `ledState` bool — does **not** drive GPIO
- GPIO is driven exclusively by `updatePresetLEDs()` in the main loop: only `buttons[activePreset].ledPin` is HIGH
- `updateActivityLed()` drives GPIO 20: reflects `buttons[lastActiveFSIndex].ledState`
- `lastActiveFSIndex` (int8_t, -1 = none) in `PedalState` — updated in main loop when a footswitch registers a new press

---

## Modulation Engine Behavior

- **RAMPER:** exponential curve, CC 0→127 on press. On release (momentary), ramps back down to 0 gradually using the DOWN pot speed — does not snap. Latching holds until next press, then ramps back down.
- **RAMPER Inverted:** resting position is 127, not 0. Same gradual return logic applies in reverse.
- **STEPPER:** same as RAMPER but in discrete steps for a quantized/robotic feel. On release/next press, steps back down using DOWN pot speed.
- **STEPPER Inverted:** resting position is 127. Same gradual return logic applies in reverse.
- **RANDOM STEPPER:** steps to random CC values continuously; does not stop at 127. Return behavior follows the same proportional speed rule.
- **LFO:** continuous 0–127–0 sweep. Wave types: sine, triangle, square. UP pot controls rise speed, DOWN pot controls descent. Momentary: gradually return to 0 on release. Latching: continues until next press, then gradually returns to 0.
- **UP/DOWN pots:** control modulation speed for the currently selected footswitch only.

### Proportional Return Speed

- Return speed = DOWN pot speed × (current CC value / 127)
- Ensures return duration feels consistent regardless of when the footswitch was released.

---

## Encoder & UI Logic

- **Encoder turn (no FS held):** changes global MIDI output channel.
- **Hold FS + press encoder:** enters mode select for that footswitch.
- **Hold FS + turn encoder (modulation modes):** selects MIDI CC number.
- **Hold FS + turn encoder (basic modes):** selects PC/CC/Note number.
- **Hold FS + turn pots:** adjusts UP and DOWN modulation speed for that footswitch.
- **LOCK switch:** disables encoder and pot input; stops WiFi AP. Preset loading still works when locked; saving does not.

---

## Basic Mode Behavior

- **PC:** momentary only. Sends Program Change on press.
- **CC momentary:** hold sends 127, release sends 0.
- **CC latching:** alternates between 0 and 127 on each press.
- **NOTE:** momentary only. Note On on press, Note Off on release.
- **Scene/Snapshot modes (Helix, QC, Fractal, Kemper):** encoder selects the target value or CC number depending on the unit.

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

---

## WiFi Web Interface

- AP SSID: `MIDI Morpher`, password: `midimorpher`
- UI served at `http://192.168.4.1` — dark-themed SPA embedded as PROGMEM string in `src/wifi/webUI.h`
- **WiFi is always on** at boot. It turns off only when the LOCK switch is engaged, and restarts when LOCK is disengaged. There is no user-toggleable WiFi setting.
- WiFi LED: GPIO 22, HIGH while AP running

### REST API (`src/wifi/webServer.h`)

| Method | Path | Body | Action |
|--------|------|------|--------|
| GET | `/api/state` | — | Full state JSON: channel, latching, activePreset, presetDirty, buttons[6] |
| GET | `/api/presets` | — | All 6 PresetData slots + activePreset index |
| POST | `/api/channel` | `{"channel":0}` | Set global MIDI channel (0–15) |
| POST | `/api/latching` | `{"latching":true}` | Set modulator latching mode |
| POST | `/api/button/:id` | `{modeIndex, midiNumber, fsChannel, rampUpMs, rampDownMs}` | Update footswitch config |
| POST | `/api/button/:id/press` | — | Simulate footswitch press (`simulatePress(true,…)`) |
| POST | `/api/button/:id/release` | — | Simulate footswitch release (`simulatePress(false,…)`) |
| POST | `/api/pot` | `{"id":0,"value":64}` | Send CC 20 (id=0) or CC 21 (id=1) directly |
| POST | `/api/preset/load/:id` | — | `applyPreset(id, pedal)` — apply preset live |
| POST | `/api/preset/save/:id` | — | `saveCurrentPreset(pedal)` — write to NVS; blocked when locked |

- `fsChannel` of 255 (0xFF) means "follow global channel"
- `handleButtonPress` also sets `_webPedal->lastActiveFSIndex = idx` to update the activity LED

### Web UI features

- Global MIDI channel dropdown
- Latching / Momentary toggle
- Preset bar (P1–P6 buttons + Save Preset button); active preset highlighted; `● Unsaved` badge when dirty
- POT1 / POT2 virtual sliders (send CC 20 / CC 21)
- 6 footswitch cards, each with:
  - Trigger button (hold for momentary modes; click-toggle for latching modes)
  - Mode dropdown (all 26)
  - MIDI number input
  - Channel dropdown (Global or Ch 1–16)
  - Ramp Up / Ramp Down sliders (visible for modulation modes only)

---

## Key Source Files

| File | Purpose |
|------|---------|
| `src/config.h` | All pin assignments and firmware constants |
| `src/sharedTypes.h` | `ModulationType` enum |
| `src/pedalState.h` | `PedalState` struct (global state container) |
| `src/statePersistance.h` | Preset NVS storage: `PresetData`, `loadAllPresets`, `saveCurrentPreset`, `applyPreset`, `markStateDirty` |
| `src/presets.h` | `handlePresetButton`, `updatePresetLEDs`, `updateActivityLed` |
| `src/footswitches/footswitchObject.h` | `FSButton`, mode table, `applyModeFlags`, `applyModeIndex`, `simulatePress` |
| `src/midiCCModulator.h` | Modulation engine (ramp, LFO, stepper, random) |
| `src/controls/encoder.h` | Encoder ISR and turn handler |
| `src/controls/encoderButton.h` | Encoder button tap/long-press handler |
| `src/controls/pots.h` | Pot read, ramp speed or CC send |
| `src/controls/toggles.h` | MS2 and LOCK toggle handlers |
| `src/visual/display.h` | OLED rendering (home screen, all param screens, preset load/save screens) |
| `src/visual/neopx.h` | NeoPixel RGB modulation indicator |
| `src/analogInOut/digipot.h` | AD5292 SPI driver |
| `src/analogInOut/expInput.h` | Expression pedal input |
| `src/wifi/webServer.h` | WiFi AP, route registration, all REST handlers |
| `src/wifi/webUI.h` | Embedded HTML/CSS/JS SPA (PROGMEM) |

---

## Libraries

External libraries (install via Arduino Library Manager):

| Library | Min version |
|---------|-------------|
| Adafruit GFX Library | 1.11.0 |
| Adafruit SSD1306 | 2.5.0 |
| Adafruit NeoPixel | 1.12.0 |

Built-in with ESP32 Arduino core (no install needed): `SPI.h`, `Wire.h`, `Preferences.h`, `USB.h`, `USBMIDI.h`, `WebServer.h`, `WiFi.h`.

Full dependency manifest: `libraries.json` at project root.

### Pot Noise Filtering (`src/potentiometers/analogReadHelpers.h`)

- Raw read: 8-sample average per read (`readPotAvg`)
- EMA filter: alpha = 1/4 — `filtered = filtered - (filtered >> 2) + (raw >> 2)`
- Deadband: 20 counts on 12-bit ADC (0–4095) before a change is reported

---

## Coding Rules (follow strictly)

1. **Plan before coding.** For every feature request, present a plan (files to touch, approach, what won't change) and wait for approval before writing any code.
2. **One feature at a time.** Only implement what was explicitly requested. Do not refactor, reorganize, or "improve" unrelated code.
3. **Minimal changes.** Prefer adding small, isolated code over restructuring existing logic. If you can solve something with fewer lines, do that.
4. **Do not change working code.** If a feature is already implemented and working, leave it alone unless the task explicitly involves it or its current implementation is flawed.
5. **ESP32/Arduino constraints.** Keep memory usage in mind (heap, stack). Avoid dynamic allocation in hot paths. Use `millis()` for timing, never `delay()` in main loop.
6. **No placeholder/stub implementations.** Every function must be real and complete. Do not leave TODOs or empty stubs.
7. **Ask before assuming.** If hardware pin assignments, I2C/SPI config, or library choices are not specified, ask before proceeding.
8. **LED GPIO rule.** `FSButton::_setLED()` only updates `ledState` — never drives GPIO directly. All footswitch LED GPIOs are driven by `updatePresetLEDs()` in the main loop. Do not bypass this.
9. **Save rule.** Settings are not auto-saved. Never add auto-save logic. All NVS writes go through `saveCurrentPreset()` or `saveAllPresets()`.
