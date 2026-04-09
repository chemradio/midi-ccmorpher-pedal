# MIDI Morpher — Claude Code Instructions

## Project Overview

MIDI Morpher is an ESP32-S3 based MIDI controller pedal with 6 independently configurable footswitches (4 onboard + 2 external via jack), a rotary encoder, two potentiometers, an SSD1306 OLED display, and a built-in WiFi web interface. Its standout feature is a MIDI CC modulation/morphing engine. Output is via mini-TRS MIDI, USB-C MIDI, and an analog expression output (via AD5292-BRUZ-20 digital potentiometer).

**Firmware target:** ESP32-S3-N16R8, Arduino framework.

---

## Hardware

- **MCU:** ESP32-S3-N16R8 devboard, native USB (Arduino framework)
- **Display:** SSD1306 OLED (I2C, 128×64)
- **Digital pot:** AD5292-BRUZ-20 (SPI, 1024 positions) — drives analog expression output
- **Controls:** 4 onboard footswitches (FS1–FS4), 2 external footswitches via jack (ExtFS1, ExtFS2), rotary encoder with push button, 2 pots (UP speed, DOWN speed), LOCK switch
- **External:** expression pedal in/out jacks
- **I/O:** mini-TRS MIDI out + in (MIDI Thru), USB-C MIDI out/thru, expression pedal out
- **WiFi:** built-in AP mode — SSID "MIDI Morpher", password "midimorpher", UI at 192.168.4.1

### Pin Assignments

| Pin | Role |
|-----|------|
| 1 | POT1 (UP speed) |
| 2 | POT2 (DOWN speed) |
| 3 | ExtFS2 LED |
| 4 | FS1 |
| 5 | FS1 LED |
| 6 | FS2 |
| 7 | FS2 LED |
| 8 | ExtFS2 |
| 9 | Encoder A |
| 10 | Encoder B |
| 11 | Encoder button |
| 12 | Expression In |
| 13 | MS2 (momentary/latching toggle switch) |
| 14 | FS3 |
| 15 | FS3 LED |
| 16 | FS4 |
| 17 | ExtFS1 |
| 18 | ExtFS1 LED |
| 21 | FS4 LED |
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

## Modulation Engine Behavior

- **RAMPER:** exponential curve, CC 0→127 on press. On release (momentary), ramps back down to 0 gradually using the DOWN pot speed — does not snap. Latching holds until next press, then ramps back down.
- **RAMPER Inverted:** resting position is 127, not 0. Same gradual return logic applies in reverse.
- **STEPPER:** same as RAMPER but in discrete steps for a quantized/robotic feel. On release/next press, steps back down using DOWN pot speed.
- **STEPPER Inverted:** resting position is 127. Same gradual return logic applies in reverse.
- **RANDOM STEPPER:** steps to random CC values continuously; does not stop at 127. Return behavior follows the same proportional speed rule.
- **LFO:** continuous 0–127–0 sweep. Wave types: sine, triangle, square. UP pot controls rise speed, DOWN pot controls descent. Momentary: gradualy return to 0 on release. Latching: continues until next press, then gradualy returns to 0.
- **UP/DOWN pots:** control modulation speed for the currently selected footswitch only.

### Proportional Return Speed

For all modulation types, the return speed scales with how far the ramp progressed:

- Return speed = DOWN pot speed × (current CC value / 127)
- Example: ramp reached CC 64 (50%) → return speed is 50% of the DOWN pot setting
- Example: ramp reached CC 96 (75%) → return speed is 75% of the DOWN pot setting
- This ensures return duration feels consistent regardless of when the footswitch was released.

---

## Encoder & UI Logic

- **Encoder turn (no FS held):** changes global MIDI output channel.
- **Hold FS + press encoder:** enters mode select for that footswitch.
- **Hold FS + turn encoder (modulation modes):** selects MIDI CC number.
- **Hold FS + turn encoder (basic modes):** selects PC/CC/Note number.
- **Hold FS + turn pots:** adjusts UP and DOWN modulation speed for that footswitch.
- **LOCK switch:** disables encoder and pot input to prevent accidental changes.

---

## Basic Mode Behavior

- **PC:** momentary only. Sends Program Change on press.
- **CC momentary:** hold sends 127, release sends 0.
- **CC latching:** alternates between 0 and 127 on each press.
- **NOTE:** momentary only. Note On on press, Note Off on release.
- **Scene/Snapshot modes (Helix, QC, Fractal, Kemper):** encoder selects the target value or CC number depending on the unit.

---

## Memory

- All footswitch modes and values stored in internal non-volatile memory (NVS/EEPROM).
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
- REST API in `src/wifi/webServer.h`:
  - `GET /api/state` — returns full JSON state (channel, wifiEnabled, all 6 buttons with mode/CC/channel/rampMs)
  - `POST /api/channel` — set global MIDI channel `{"channel": 0}`
  - `POST /api/button/:id` — update a footswitch `{"modeIndex":4,"midiNumber":11,"fsChannel":255,"rampUpMs":1000,"rampDownMs":1000}`
  - `POST /api/wifi` — enable/disable AP `{"enabled": false}`
- WiFi LED: GPIO 22, HIGH while AP running
- LOCK switch turns off AP; AP restarts when LOCK is disengaged (if WiFi enabled)
- Recovery: hold FS1 + FS2 for 3 seconds to re-enable WiFi after it was disabled via UI
- `fsChannel` of 255 (0xFF) means "follow global channel"

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
4. **Do not change working code.** If a feature is already implemented and working, leave it alone unless the task explicitly involves it or it's current implementation is flawed.
5. **ESP32/Arduino constraints.** Keep memory usage in mind (heap, stack). Avoid dynamic allocation in hot paths. Use `millis()` for timing, never `delay()` in main loop.
6. **No placeholder/stub implementations.** Every function must be real and complete. Do not leave TODOs or empty stubs.
7. **Ask before assuming.** If hardware pin assignments, I2C/SPI config, or library choices are not specified, ask before proceeding.
