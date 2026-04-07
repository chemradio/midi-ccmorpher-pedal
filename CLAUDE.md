# MIDI Morpher — Claude Code Instructions

## Project Overview

MIDI Morpher is an ESP32-S3 based MIDI controller pedal with 4 footswitches, a rotary encoder, two potentiometers, and an SSD1306 OLED display. Its standout feature is a MIDI CC modulation/morphing engine. Output is via mini-TRS MIDI, USB-C MIDI, and an analog expression output (via AD5292-BRUZ-20 digital potentiometer).

**Firmware target:** ESP32-S3-N16R8, Arduino framework.

---

## Hardware

- **MCU:** ESP32-S3-N16R8 devboard, native USB (Arduino framework)
- **Display:** SSD1306 OLED (I2C)
- **Digital pot:** AD5292-BRUZ-20 (SPI) — drives analog expression output
- **Controls:** 4 onboard footswitches, rotary encoder with push button, 2 pots (UP speed, DOWN speed), LOCK switch
- **External:** 2 additional footswitches via jack, expression pedal in/out jack
- **I/O:** mini-TRS MIDI out + in (MIDI Thru), USB-C MIDI out/thru, expression pedal out

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
- **LFO:** continuous 0–127–0 sweep. Wave types: sine, triangle, square. UP pot controls rise speed, DOWN pot controls descent. Momentary: **snaps immediately to 0** on release (exception to the proportional return rule). Latching: continues until next press, then snaps to 0.
- **UP/DOWN pots:** control modulation speed for the currently selected footswitch only.

### Proportional Return Speed

For all modulation types **except LFO**, the return speed scales with how far the ramp progressed:

- Return speed = DOWN pot speed × (current CC value / 127)
- Example: ramp reached CC 64 (50%) → return speed is 50% of the DOWN pot setting
- Example: ramp reached CC 96 (75%) → return speed is 75% of the DOWN pot setting
- This ensures return duration feels consistent regardless of when the footswitch was released.
- LFO is exempt — it always snaps to 0 instantly on release.

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

## Coding Rules (follow strictly)

1. **Plan before coding.** For every feature request, present a plan (files to touch, approach, what won't change) and wait for approval before writing any code.
2. **One feature at a time.** Only implement what was explicitly requested. Do not refactor, reorganize, or "improve" unrelated code.
3. **Minimal changes.** Prefer adding small, isolated code over restructuring existing logic. If you can solve something with fewer lines, do that.
4. **Do not change working code.** If a feature is already implemented and working, leave it alone unless the task explicitly involves it.
5. **ESP32/Arduino constraints.** Keep memory usage in mind (heap, stack). Avoid dynamic allocation in hot paths. Use `millis()` for timing, never `delay()` in main loop.
6. **No placeholder/stub implementations.** Every function must be real and complete. Do not leave TODOs or empty stubs.
7. **Ask before assuming.** If hardware pin assignments, I2C/SPI config, or library choices are not specified, ask before proceeding.
