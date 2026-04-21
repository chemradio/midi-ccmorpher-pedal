# MIDI Morpher

A programmable MIDI controller pedal built on ESP32-S3. Its standout feature is a real-time MIDI CC modulation engine — ramp, LFO, stepper, and random sweeps — alongside full basic MIDI (PC, CC, Note, Scene/Snapshot). Every parameter is configurable on-device via encoder, or wirelessly via a built-in web interface. Six independent presets let you store and recall complete configurations instantly.

---

## Features at a Glance

- 4 onboard footswitches + 2 external via jack — all 6 independently configurable
- 32 footswitch modes: basic MIDI + 5 modulation types + scene/snapshot (with scroll variants) for Helix, QC, Fractal, Kemper + Tap Tempo + System/Transport
- **6 presets** — store and recall complete configurations including per-preset BPM; footswitch LEDs indicate the active preset
- Per-footswitch MIDI channel override
- Modulation speed controlled by two onboard pots (UP and DOWN independently)
- Proportional return speed — feels consistent regardless of when you release
- **MIDI Clock** — internal tap tempo or slave to incoming `0xF8`; ramp/LFO speeds can be expressed as note values (1/32T … 2/1) and resolve to milliseconds at current BPM
- **Tempo LED** — pulses on the beat
- SSD1306 OLED display with live feedback (shows active preset + unsaved-changes indicator + live BPM)
- Onboard RGB NeoPixel reflects modulation value in real time
- **Activity LED** — shows the active state of the most recently pressed footswitch
- Analog expression output via AD5292 digital potentiometer
- Expression pedal input — mirrors value to expression out and sends as MIDI CC20
- mini-TRS MIDI Out + In (MIDI Thru), USB-C MIDI, **BLE MIDI** (Apple standard, iOS/macOS/Windows/Linux/Android)
- **Configurable MIDI routing** — 6 toggleable pairs: DIN↔USB, DIN↔BLE, USB↔BLE
- **Wireless web UI** — full configuration from any phone or laptop; includes footswitch trigger buttons, pot sliders, and global settings
- **Captive portal** — phones/laptops auto-open the UI when joining the network
- WiFi always on (turns off only when LOCK switch is engaged)
- LOCK switch — prevents accidental encoder/pot changes and disables WiFi
- **Encoder acceleration** — fast turns multiply steps by up to 8× for quick CC/PC/Note scanning
- All settings stored in non-volatile memory inside each preset slot

---

## Build Setup

**Board:** ESP32-S3-N16R8. In Arduino IDE, install `Espressif Systems → esp32` board package version ≥ 3.0.0.

**Libraries** (install via Arduino Library Manager):

| Library | Version |
|---------|---------|
| Adafruit GFX Library | ≥ 1.11.0 |
| Adafruit SSD1306 | ≥ 2.5.0 |
| Adafruit NeoPixel | ≥ 1.12.0 |
| NimBLE-Arduino | ≥ 2.0.0 |

Built-in (no install needed): `SPI.h`, `Wire.h`, `Preferences.h`, `USB.h`, `USBMIDI.h`, `WebServer.h`, `WiFi.h`

Full dependency manifest: [`libraries.json`](libraries.json)

Open `midi_morpher/midi_morpher.ino` and upload to your board.

---

## Hardware Overview

### Controls

| Control | Function |
|---------|----------|
| FS1–FS4 | Onboard footswitches |
| ExtFS1, ExtFS2 | External footswitches via jack |
| PRESET button | Short press: cycle presets. Long press (1.5 s): save current settings to active preset. |
| Rotary encoder | Adjust BPM (bare turn) / parameter select (while holding FS) / MIDI channel (while holding encoder button) |
| Encoder button | Short press (no FS): open main menu. Short press (FS held): open mode select. Long press (FS held, ~600 ms): per-FS channel. Long press (no FS, locked, 3 s): unlock. |
| POT1 | Modulation UP speed (0–5 s) |
| POT2 | Modulation DOWN speed (0–5 s) |
| MS2 toggle | Global Momentary / Latching mode |
| LOCK switch | Freeze encoder and pots; disable WiFi |

### I/O

| Port | Function |
|------|----------|
| mini-TRS MIDI Out | Primary MIDI output |
| mini-TRS MIDI In | MIDI Thru (passes through to output) |
| USB-C | USB MIDI out/thru + power |
| BLE MIDI | Wireless MIDI (Apple BLE-MIDI standard) — always on |
| Expression Out | Analog wiper mirrors modulation (AD5292 digipot) |
| Expression In | External expression pedal → mirrors to Exp Out + sends MIDI CC (configurable) |
| External FS jack | Connect 2 additional footswitches |

### Pin Assignments

| Pin | Role |
|-----|------|
| 1 | POT1 (UP speed) |
| 2 | POT2 (DOWN speed) |
| 3 | ExtFS2 LED |
| 4 | FS1 |
| 5 | FS1 LED * |
| 6 | FS2 |
| 7 | FS2 LED * |
| 8 | ExtFS2 |
| 9 | Encoder A |
| 10 | Encoder B |
| 11 | Encoder button |
| 12 | Expression In |
| 13 | MS2 (momentary/latching toggle) |
| 14 | FS3 |
| 15 | FS3 LED * |
| 16 | FS4 |
| 17 | ExtFS1 |
| 18 | ExtFS1 LED * |
| 21 | FS4 LED * |
| 36 | MIDI RX |
| 38 | Digipot CS (SYNC) |
| 39 | Digipot SCK |
| 40 | Digipot MOSI (SDI) |
| 41 | SDA (OLED) |
| 42 | SCL (OLED) |
| 43 | MIDI TX |
| 44 | PRESET button |
| 45 | Activity LED |
| 46 | Tempo LED |
| 47 | LOCK switch |
| 48 | NeoPixel (onboard RGB) |

*\* Footswitch LEDs are repurposed as preset indicators — see [Preset LEDs](#preset-leds) below.*

**Pins to avoid on ESP32-S3-N16R8:** GPIOs 19/20 are native USB D-/D+ (unusable while USB is active — continuous USB-MIDI traffic causes phantom reads). GPIOs 33–37 are reserved for octal PSRAM. GPIOs 45 and 46 are strapping pins but default LOW at reset, so they are safe for output LEDs wired cathode→resistor→GND.

---

## Using the Pedal

### Presets

The pedal stores 6 independent presets. Each preset saves the complete configuration: all 6 footswitch modes, MIDI numbers, per-footswitch channels, ramp speeds, the global MIDI channel, and the current BPM.

**Switching presets:**
Press the PRESET button once. The pedal cycles forward through presets (P1 → P2 → … → P6 → P1). The OLED briefly shows the preset number, and the corresponding footswitch LED lights up.

**Saving a preset:**
Hold the PRESET button for 1.5 seconds. The display flashes (inverted) with the preset number and "SAVED". The current live settings are written to that preset slot. Saving is blocked when LOCK is engaged.

**Unsaved changes:**
Any time you change a setting (encoder, pots, or web UI) without saving, the OLED home screen shows `*` next to the preset number. This clears when you save.

#### Preset LEDs

The 6 footswitch LEDs serve as a preset indicator — only the LED for the currently active preset is lit. A separate **Activity LED** (GPIO 20) takes over the job of showing footswitch activity: it reflects the active/latched state of the most recently pressed footswitch.

### Adjusting BPM

Turn the encoder knob (without holding any footswitch or the encoder button). The OLED shows the new BPM. Manual adjustment also breaks external MIDI clock sync.

### Changing the Global MIDI Channel

Hold the encoder button and turn the knob. The OLED shows the new channel (1–16) in real time. The change is part of the current preset — save it to keep it.

### Main Menu

Short-press the encoder button (without holding any footswitch) to open the main menu. Turn the encoder to scroll through 14 items; press the encoder button to select. Menu items:

1. MIDI Channel
2. MIDI Routings (6 toggleable pairs: DIN↔USB, DIN↔BLE, USB↔BLE)
3. Pot 1 CC (0–127 or Off)
4. Pot 2 CC (0–127 or Off)
5. Exp In CC
6. Exp Calibration (triggers 5-second sweep)
7. Exp Wake Display
8. LED Mode (On / Conservative / Off)
9. Tempo LED (on/off)
10. NeoPixel (on/off)
11. Display Brightness (0–100%)
12. Display Timeout (2 s / 5 s / 10 s / Always)
13. Lock Settings
14. Exit

### Changing a Footswitch Mode

1. Hold the footswitch you want to change.
2. While holding it, press the encoder button.
3. The display shows a two-level picker: first choose the category (Basic, Ramper, LFO, Stepper, Random, Scenes, Utility), press the encoder to drill in, then choose the variant. Press again to apply.
4. Release the footswitch to confirm. Save the preset to keep it.

### Changing the MIDI Number (CC, PC, Note, Scene value)

1. Hold the footswitch.
2. Turn the encoder. The display shows the updated number.
3. Release. Save the preset to keep it.

### Adjusting Modulation Speed (Ramp, LFO, Stepper modes)

1. Hold the footswitch.
2. Turn POT1 to adjust the UP speed (0–5 s).
3. Turn POT2 to adjust the DOWN speed (0–5 s).
4. Speeds apply only to the footswitch you are holding. Each footswitch has its own independent speed settings.

### Setting a Per-Footswitch MIDI Channel

By default all footswitches use the global MIDI channel. To override:

1. Hold the footswitch.
2. Hold the encoder button for ~600 ms until the channel screen appears.
3. Turn the encoder to select the channel (1–16) or scroll past 16 to return to Global.
4. Release. Save the preset to keep it.

### LOCK Switch

Engage the LOCK switch to freeze all encoder and pot input. The OLED shows "LOCKED" and inverts the display. WiFi also turns off while locked (restarts automatically when unlocked). Disengage to resume editing.

Preset *loading* (short PRESET press) works even when LOCK is engaged. Preset *saving* (long PRESET press) is blocked.

---

## Footswitch Modes

### Basic Modes

| Mode | Behavior |
|------|----------|
| PC | Sends a Program Change on press. Encoder selects PC number (1–128). |
| CC | Momentary: holds 127 while pressed, sends 0 on release. Encoder selects CC number. |
| CC Latch | Alternates between CC 127 and CC 0 on each press. Encoder selects CC number. |
| Note | Note On on press, Note Off on release. Encoder selects note (shown as note name + octave). |

### Scene / Snapshot Modes

These modes are tailored to specific guitar modellers. Each comes in two variants:

- **Normal**: Encoder pre-selects a value; every press sends that same value.
- **Scroll**: Each press advances to the next scene/slot and sends it (wraps around).

| Mode | CC | Values | Notes |
|------|----|--------|-------|
| Helix Snap / Helix Scrl | CC 69 | 0–7 | Snapshot select or scroll |
| QC Scene / QC Scrl | CC 43 | 0–7 | Scene select or scroll |
| Fractal Scene / Fract Scrl | CC 34 | 0–7 | Scene select or scroll |
| Kemper Slot / Kemper Scrl | CC 50–54 | 1 | Encoder selects CC number (slot 0 = CC50, slot 4 = CC54); scroll cycles CC numbers |

### System / Transport Mode

Encoder selects one of 8 transport commands. Each press sends both an MMC SysEx message (for DAWs) and the corresponding System Real-Time byte (for hardware).

| Command | MMC | SysEx byte |
|---------|-----|------------|
| Play | F0 7F 7F 06 02 F7 | FA |
| Stop | F0 7F 7F 06 01 F7 | FC |
| Continue | F0 7F 7F 06 03 F7 | FB |
| Record | F0 7F 7F 06 06 F7 | — |
| Pause | F0 7F 7F 06 09 F7 | — |
| Rewind | F0 7F 7F 06 05 F7 | — |
| Fast Fwd | F0 7F 7F 06 04 F7 | — |
| Goto Start | Song Position Pointer to 0 | — |

### Modulation Modes

All modulation modes output to the CC number set for that footswitch. Scroll the encoder one step below CC 0 to select **Pitch Bend** output instead — the modulator then sends 14-bit pitch bend (0–16383, center 8192). The same output also drives the expression pedal output jack.

#### RAMPER

Sends a smooth ramp from 0 to 127 (exponential curve) on press. On release, ramps back down using the DOWN pot speed.

- **Momentary:** ramp up on press, ramp down on release.
- **Latching:** ramp up on press, hold at 127 on release, ramp down on next press.
- **Inverted:** resting value is 127. Ramps down on press, returns up on release/next press.

#### STEPPER

Linear-quantized movement from 0 to 127 in discrete steps for a robotic feel.

- Momentary and Latching variants.
- Inverted variant available.

#### RANDOM STEPPER

Linear timing, but steps land on random CC values continuously. Does not stop at 127 — keeps randomizing until released/unlatched.

- Momentary and Latching variants.
- Inverted variant available.

#### LFO

Continuous oscillation between 0 and 127. Three wave shapes available.

| Mode | Wave |
|------|------|
| LFO Sine | Raised-cosine sine wave |
| LFO Tri | Linear triangle wave |
| LFO Sq | Square wave (0 ↔ 127) |

- **Momentary:** LFO runs while held, returns to 0 on release.
- **Latching:** LFO runs after press, continues on release, returns to 0 on next press.

POT1 controls rise time (half-cycle up), POT2 controls fall time (half-cycle down).

#### Tap Tempo

A 27th mode with no MIDI output — pressing the footswitch updates the pedal's internal BPM via tap averaging. The OLED briefly shows the new BPM. Use this to set the global clock that drives clock-synced ramps.

### Proportional Return Speed

For all modulation types, the return speed scales with the current value at the moment of release:

> Return time = DOWN pot speed × (current CC value ÷ 127)

A ramp that only reached CC 50 will return twice as fast as one that reached CC 127.

---

## BLE MIDI

The pedal advertises itself as **MIDI Morpher** using the standard Apple BLE-MIDI service (UUID `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`). Compatible with iOS, macOS, Windows, Linux, and Android. BLE MIDI is always active — the LOCK switch does not affect it.

BLE MIDI coexists with the WiFi AP via radio time-slicing. BLE TX and RX can be routed to/from DIN and USB via the configurable MIDI routing flags.

---

## Wireless Web Interface

The pedal broadcasts its own WiFi access point (always on, except when LOCK is engaged):

- **Network:** `MIDI Morpher`
- **Password:** `midimorpher`
- **Address:** `http://192.168.4.1` or `http://midimorpher.local` (mDNS)

Connect from any phone, tablet, or laptop — no app required.

### What You Can Configure via Web UI

- Global MIDI channel (1–16) + live BPM readout (with EXT badge when slaved to incoming MIDI clock)
- **Presets** — switch between all 6 presets; save the current settings to the active preset
- Per-footswitch mode (all 32 modes)
- Per-footswitch MIDI CC / PC / Note number — **1-indexed** UI, mode-aware bounds (1–128 for CC/PC/Note, 1–8 for scene modes, 1–5 for Kemper slot), disabled for Tap Tempo
- Per-footswitch MIDI channel override (or Global)
- Per-footswitch ramp UP and DOWN speed — either a millisecond slider or a note-value dropdown (1/32T … 2/1), toggled by an inline `sync` checkbox. Visible for modulation modes only.
- **Footswitch trigger buttons** — activate footswitches directly from the browser (hold for momentary, click-toggle for latching)
- **POT1 / POT2 sliders** — send CC 20 / CC 21 directly from the web UI
- **Global settings** — LED mode, tempo LED, NeoPixel, display brightness, display timeout, pot CC assignments, expression CC, expression pedal calibration, MIDI routing flags

### Unsaved Changes

Any setting change marks the active preset as unsaved. A `● Unsaved` badge appears in the web UI header. Click **Save Preset** to write the current state to the active preset slot in non-volatile memory.

### Captive Portal

When you connect to the `MIDI Morpher` network, the pedal's DNS server catches every query and its HTTP server 302-redirects any unknown URL (including OS probes like `/generate_204`, `/hotspot-detect.html`, `/ncsi.txt`) to `http://192.168.4.1/`. Phones and laptops respond by popping up a "Sign in to network" prompt that opens the UI directly — no need to type the address.

### WiFi and LOCK

WiFi turns off automatically when the LOCK switch is engaged and restarts when it is disengaged. There is no separate WiFi on/off toggle — the LOCK switch is the only way to disable it.

### REST API

| Method | Path | Body | Action |
|--------|------|------|--------|
| GET | `/api/state` | — | Full state JSON (channel, activePreset, presetDirty, bpm, externalSync, all 6 buttons) |
| GET | `/api/presets` | — | All 6 preset slots + active index |
| GET | `/api/global` | — | All global settings as JSON |
| POST | `/api/global` | global settings fields | Update global settings |
| POST | `/api/channel` | `{"channel":0}` | Set global MIDI channel (0–15) |
| POST | `/api/button/:id` | `{"modeIndex":4,"midiNumber":11,"fsChannel":255,"rampUpMs":1000,"rampDownMs":1000}` | Update footswitch config. Server clamps `midiNumber` against mode range (`sceneMaxVal` for scene modes, 127 otherwise). Ramp values with bit 31 set encode a note-value index (see MIDI Clock below). |
| POST | `/api/button/:id/press` | — | Simulate footswitch press |
| POST | `/api/button/:id/release` | — | Simulate footswitch release |
| POST | `/api/bpm` | `{"bpm":120}` | Set internal BPM (20–300); clears external sync |
| GET | `/api/poll` | — | Lightweight poll: `{bpm, externalSync, activePreset, presetDirty}` |
| POST | `/api/pot` | `{"id":0,"value":64}` | Send CC 20 (id=0) or CC 21 (id=1) |
| POST | `/api/preset/load/:id` | — | Apply preset N (0–5) to live state |
| POST | `/api/preset/save/:id` | — | Save current live state to preset N |
| GET | `/api/expcal` | — | Poll calibration status `{running, min, max}` |
| POST | `/api/expcal` | — | Start 5-second expression pedal calibration sweep |
| GET | `/dismiss` | — | Clean shutdown of WiFi AP |

---

## OLED Display

The home screen shows:

```
Ch:1  P:2*  Latch  LOCK
─────────────────────────
1 Ramp         CC:5
2 LFO Sine     CC:11
3 PC           PC:1
4 CC Latch     CC:74
```

- Top left: global MIDI channel
- Top center-left: active preset number (`P:N`) with `*` when unsaved changes exist
- Top center: Latch or Mom (current modulator mode)
- Top right: LOCK indicator when engaged
- Rows: footswitch number, mode name, parameter value (CC/PC/Note/Scene)
- If a footswitch has a per-channel override, the channel number appears left of the parameter

Temporary screens appear on any interaction and return to the home screen after 2 seconds.

---

## NeoPixel Status LED

The onboard RGB LED (GPIO 48) reflects the current modulation output level:

- Hue shifts from green (low) to red (high) as the CC value rises from 0 to 127
- Brightness scales with the CC value

---

## Memory / Presets

All settings are stored in 6 preset slots in internal NVS (non-volatile storage). Each preset contains:

- Global MIDI channel
- BPM (loaded when the preset is applied)
- For each of 6 footswitches: mode, MIDI number, per-footswitch channel, ramp UP/DOWN speed

Global settings (LED mode, display, routing, pot/exp CCs, calibration) are stored separately in NVS and are not preset-specific.

Settings are **not auto-saved**. Changes are held in memory until you explicitly save (long-press PRESET button or click Save Preset in the web UI).

On first boot after a firmware update from an older version, the old single-preset settings are migrated into preset slot 1. All other slots initialize to factory defaults.

---

## Expression Pedal Calibration

The expression pedal input auto-ranges, but a manual calibration gives the best accuracy:

1. Open the main menu (encoder button short press, no FS held) and select **Exp Calibration**, or press the **Exp Calibration** button in the web UI.
2. Slowly sweep the expression pedal from heel to toe and back over 5 seconds.
3. The min and max ADC readings are saved to non-volatile storage.

If the pedal moves beyond the stored range during normal use, the range expands automatically.

---

## MIDI Thru

The mini-TRS MIDI In passes all received MIDI messages directly to the MIDI Out. Use this to daisy-chain other MIDI controllers while the MIDI Morpher is in the signal path.

---

## Internal Components

- ESP32-S3-N16R8 devboard (16 MB flash, 8 MB PSRAM, native USB)
- SSD1306 128×64 OLED (I2C)
- AD5292-BRUZ-20 digital potentiometer (SPI, 1024 positions) — analog expression output
- Adafruit NeoPixel RGB LED (onboard, GPIO 48)
- Powered via USB-C (standard 5 V)
