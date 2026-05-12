# Tilt4

Tilt4 is an ESP32-S3 based MIDI modulation pedal with 6 independently configurable footswitches (4 onboard + 2 external via jack), 128 presets for all footswitches, a rotary encoder, an SSD1306 OLED display, and a built-in WiFi web interface. Its standout feature is a MIDI CC modulation/morphing engine. Output is via mini-TRS MIDI, USB-C MIDI, BLE MIDI. Each preset can store completely different configurations per footswitch, including per-preset BPM.
Firmware target: ESP32-S3-N16R8, Arduino framework.

---

## Hardware

- ESP32-S3-N16R8 devboard, native USB (Arduino framework)
- SSD1306 OLED (I2C, 128×64)
- 4 onboard footswitches (FS1–FS4), 2 external footswitches via jack (ExtFS1, ExtFS2), rotary encoder with push button
- 7 Neopixels - one per footswitch + tempo
- expression pedal in jack
- mini-TRS MIDI out + in (MIDI Thru), USB-C MIDI out/thru, BLE MIDI in/out
- built-in AP mode — SSID "Tilt4", password "tilt4pedal", UI at 192.168.4.1. Always on except when Lock Settings is engaged.
- standard Apple BLE-MIDI service (UUID `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`), advertised as `Tilt4`. Always on. Coexists with the WiFi AP via radio time-slicing.

---

## Preset System

(Don't case about previously stored presets. Always opt for better code design than preserving current presets. All of the presets can be erased, reformatted etc.)

- Up to 128 preset slots in `/presets.bin` (LittleFS). User-visible count is `globalSettings.presetCount` (1–128, default 6); the PRESET button + web UI cycle within `[0, presetCount)`. The remaining slots are reachable via direct jump (`PresetNum` mode, `/api/preset/load/:id`).
- Each preset stores: global MIDI channel, per-preset BPM, per-footswitch (modeIndex, midiNumber, fsChannel, ccLow/Hi, velocity, rampUpMs, rampDownMs, 3 extraActions), and an optional preset-load action.
- Settings are volatile until explicitly saved.
- `presetDirty = true` is set by any setting change (encoder, web UI); cleared on preset load or save.
- On first boot (file missing or header magic/version mismatch): factory defaults written for all slots.
- Holding encoder button triggers preset save (unless Lock Settings is engaged).

---

## Modulation Engine Behavior

- **RAMPER:** exponential curve (`SHAPE_EXP`), CC 0→127 on press. On release (momentary), ramps back down to 0 gradually using the configured ramp-down speed — does not snap. Latching holds until next press, then ramps back down. Inverted mode: resting position is 127, not 0. Same gradual return logic applies in reverse.
- **STEPPER:** linear curve (`SHAPE_LINEAR`), moves in discrete quantized steps. Return uses the configured ramp-down speed. Inverted mode: resting position is 127. Same gradual return logic applies in reverse.
- **RANDOM STEPPER:** linear curve. Steps to random CC values continuously; does not stop at 127. Return behavior follows the same proportional speed rule.
- **LFO:** continuous 0–127–0 sweep. Wave types: sine (`SHAPE_SINE`, raised-cosine), triangle (`SHAPE_LINEAR`), square (`SHAPE_SQUARE`). Rise and fall speeds are configured independently. Momentary: gradually return to 0 on release. Latching: continues until next press, then gradually returns to 0.

### Shared or per-fs modulator

`MidiCCModulator` is share across footswitches or each footswitch is assigned a different one. Switchable in global settings menu.

### Proportional Return Speed

- Return speed = configured ramp-down speed × (current CC value / 127)
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

- Presets live in `/presets.bin` on **LittleFS** (replaced SPIFFS, which is upstream-deprecated). The active-preset index is in NVS namespace `"presets"`. Global settings: NVS `"globals"`. Multi-scenes / exp calibration: their own NVS keys.
- The preset file starts with a `PresetFileHeader { magic="MMP1", version, presetCount, recordSize }`. Mismatch → factory defaults; no silent corruption. Bump `PRESET_FILE_VERSION` whenever `PresetData` / `FSButtonPersisted` layout changes.
- **No auto-save.** Changes are not persisted until the user saves the preset via long-press or web UI.
- `markStateDirty()` sets `presetDirty = true`.
- The active preset's load action is edited via `PedalState.liveLoadAction`; only flushed into `presets[activePreset].loadAction` on save (mirrors how button edits work). Switching presets reseeds it from the new slot — unsaved load-action edits are dropped, same as unsaved button edits.

---

## UI mode state

Transient UI state on `PedalState` (`inModeSelect`, `inActionSelect`, `inChannelSelect`, `inFSEdit`, `menuState`) is meant to be mutually-exclusive. **Never** clear flags individually at a "exit to home" site — call `pedal.exitAllUIModes()`. Use `pedal.anyUIModeActive()` to check whether any of them are set. Centralized so adding a new flag doesn't require hunting every clear site.

---

## Pedal-wide event flags

`presetNavRequest` and `presetNavDirect` (inline globals, declared in `pedalState.h`) are the FS→loop event channel for preset navigation. FS handlers in `footswitchObject.h` set them; the main loop consumes and clears them on the next tick.

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

- AP SSID: `Tilt4`, password: `tilt4pedal`
- UI served at `http://192.168.4.1` — dark-themed SPA embedded as PROGMEM string in `src/wifi/webUI.h`
- **WiFi is always on** at boot. It turns off only when Lock Settings is engaged, and restarts when disengaged. There is no separate WiFi toggle.
- **Captive portal:** `DNSServer` catches all DNS queries and redirects to `192.168.4.1`. `webServer.onNotFound(handleCaptivePortal)` returns a 302 to `/` for any unregistered URL, including OS probes like `/generate_204`, `/hotspot-detect.html`, `/ncsi.txt`. Phones/laptops then show a "Sign in to network" prompt that opens the UI automatically.

---

## MIDI Receiver Sub-project (`midi-receiver/`)

Companion sketch for ESP32-S3, ESP32-S2, or ESP32-S2-mini. Receives MIDI wirelessly from Tilt4 and re-outputs it via USB MIDI (native USB) and DIN/TRS MIDI (Serial1).

- **Transport selection:** runtime, short-press `TRANSPORT_SELECT_BTN_PIN` (default GPIO 0 / BOOT button). Cycles: WiFi UDP → ESP-NOW → … Persisted to NVS namespace `"rxcfg"` key `"transport"`.
- **Status NeoPixel:** `STATUS_NEOPIXEL_PIN` (default GPIO 48 = onboard RGB on most ESP32-S3 DevKits) shows the active transport's color — orange = WiFi UDP, purple = ESP-NOW. Blinks at 1 Hz while connecting, solid once `connected()` is true; falls back to blinking on disconnect. Brightness `STATUS_NEOPIXEL_BRIGHTNESS` (~26/255 ≈ 10%). Set pin to `-1` to disable.
- **MIDI output:** `src/midi_output.h` — bytes forwarded to DIN UART immediately; USB MIDI packets assembled via static state machine.
- **Tilt4 side:** `udpEnabled` / `espNowEnabled` toggles in `GlobalSettings` (web UI). ESP-NOW pairing: Tilt4 always listens for `MMPR` magic, registers unicast peer, sends `MMPA` ACK. Receiver re-sends `MMPR` every 2 s.
- **Key files:** `config.h` (pins + WiFi/ESP-NOW params), `src/transport_manager.h` (runtime switching + NVS + LED blink), `src/midi_output.h`, `src/transport/` (one `.h` per transport).

---

## Libraries

External libraries (install via Arduino Library Manager):
Full dependency manifest: `libraries.json` at project root.

---

## Coding Rules (follow strictly)

Be conservative with tokens, be precise. For every feature request, present a plan (files to touch, approach) and wait for approval before writing any code. Only implement what was explicitly requested. Do not refactor, reorganize, or "improve" unrelated code. Prefer adding small, isolated code over restructuring existing logic. If you can solve something with fewer lines, do that. If a feature is already implemented and working, leave it alone unless the task explicitly involves it or its current implementation is flawed. Keep memory usage in mind (heap, stack). Avoid dynamic allocation in hot paths. Use `millis()` for timing, never `delay()` in main loop.
