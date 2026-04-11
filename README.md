# MIDI Morpher

A programmable MIDI controller pedal built on ESP32-S3. Its standout feature is a real-time MIDI CC modulation engine — ramp, LFO, stepper, and random sweeps — alongside full basic MIDI (PC, CC, Note, Scene/Snapshot). Every parameter is configurable on-device via encoder, or wirelessly via a built-in web interface. Six independent presets let you store and recall complete configurations instantly.

---

## Features at a Glance

- 4 onboard footswitches + 2 external via jack — all 6 independently configurable
- 26 footswitch modes: basic MIDI + 5 modulation types + scene/snapshot for Helix, QC, Fractal, Kemper
- **6 presets** — store and recall complete configurations; footswitch LEDs indicate the active preset
- Per-footswitch MIDI channel override
- Modulation speed controlled by two onboard pots (UP and DOWN independently)
- Proportional return speed — feels consistent regardless of when you release
- SSD1306 OLED display with live feedback (shows active preset + unsaved-changes indicator)
- Onboard RGB NeoPixel reflects modulation value in real time
- **Activity LED** — shows the active state of the most recently pressed footswitch
- Analog expression output via AD5292 digital potentiometer
- Expression pedal input — mirrors value to expression out and sends as MIDI CC20
- mini-TRS MIDI Out + In (MIDI Thru), USB-C MIDI
- **Wireless web UI** — full configuration from any phone or laptop; includes footswitch trigger buttons and pot sliders
- WiFi always on (turns off only when LOCK switch is engaged)
- LOCK switch — prevents accidental encoder/pot changes and disables WiFi
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
| Rotary encoder | MIDI channel / parameter select |
| Encoder button | Mode select (hold FS first) |
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
| Expression Out | Analog wiper mirrors modulation (AD5292 digipot) |
| Expression In | External expression pedal → mirrors to Exp Out + sends MIDI CC20 |
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
| 19 | PRESET button |
| 20 | Activity LED |
| 21 | FS4 LED * |
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
| 3 | ExtFS2 LED * |

*\* Footswitch LEDs are repurposed as preset indicators — see [Preset LEDs](#preset-leds) below.*

---

## Using the Pedal

### Presets

The pedal stores 6 independent presets. Each preset saves the complete configuration: all 6 footswitch modes, MIDI numbers, per-footswitch channels, ramp speeds, and the global MIDI channel.

**Switching presets:**
Press the PRESET button once. The pedal cycles forward through presets (P1 → P2 → … → P6 → P1). The OLED briefly shows the preset number, and the corresponding footswitch LED lights up.

**Saving a preset:**
Hold the PRESET button for 1.5 seconds. The display flashes (inverted) with the preset number and "SAVED". The current live settings are written to that preset slot. Saving is blocked when LOCK is engaged.

**Unsaved changes:**
Any time you change a setting (encoder, pots, or web UI) without saving, the OLED home screen shows `*` next to the preset number. This clears when you save.

#### Preset LEDs

The 6 footswitch LEDs serve as a preset indicator — only the LED for the currently active preset is lit. A separate **Activity LED** (GPIO 20) takes over the job of showing footswitch activity: it reflects the active/latched state of the most recently pressed footswitch.

### Changing the Global MIDI Channel

Turn the encoder knob (without holding any footswitch). The OLED shows the new channel (1–16) in real time. The change is part of the current preset — save it to keep it.

### Changing a Footswitch Mode

1. Hold the footswitch you want to change.
2. While holding it, press the encoder button.
3. The display shows the current mode. Turn the encoder to cycle through all 26 modes.
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

These modes are tailored to specific guitar modellers. Encoder selects the target scene/slot.

| Mode | CC | Values | Notes |
|------|----|--------|-------|
| Helix Snap | CC 69 | 0–7 | Encoder selects snapshot value |
| QC Scene | CC 43 | 0–7 | Encoder selects scene value |
| Fractal Scene | CC 34 | 0–7 | Encoder selects scene value |
| Kemper Slot | CC 50–54 | 1 | Encoder selects CC number (slot 0 = CC50, slot 4 = CC54) |

### Modulation Modes

All modulation modes output to the CC number set for that footswitch. The same output also drives the expression pedal output jack.

#### RAMPER

Sends a smooth ramp from 0 to 127 (exponential curve) on press. On release, ramps back down using the DOWN pot speed.

- **Momentary:** ramp up on press, ramp down on release.
- **Latching:** ramp up on press, hold at 127 on release, ramp down on next press.
- **Inverted:** resting value is 127. Ramps down on press, returns up on release/next press.

#### STEPPER

Same as RAMPER but moves in discrete quantized steps for a robotic/quantized feel.

- Momentary and Latching variants work the same as RAMPER.
- Inverted variant available.

#### RANDOM STEPPER

Steps to random CC values continuously. Does not stop at 127 — keeps randomizing until released/unlatched.

- Momentary and Latching variants.
- Inverted variant available.

#### LFO

Continuous oscillation between 0 and 127. Three wave shapes available.

| Mode | Wave |
|------|------|
| LFO Sine | Smooth sine wave |
| LFO Tri | Linear triangle wave |
| LFO Sq | Square wave (0 ↔ 127) |

- **Momentary:** LFO runs while held, returns to 0 on release.
- **Latching:** LFO runs after press, continues on release, returns to 0 on next press.

POT1 controls rise time (half-cycle up), POT2 controls fall time (half-cycle down).

### Proportional Return Speed

For all modulation types, the return speed scales with the current value at the moment of release:

> Return time = DOWN pot speed × (current CC value ÷ 127)

A ramp that only reached CC 50 will return twice as fast as one that reached CC 127.

---

## Wireless Web Interface

The pedal broadcasts its own WiFi access point (always on, except when LOCK is engaged):

- **Network:** `MIDI Morpher`
- **Password:** `midimorpher`
- **Address:** `http://192.168.4.1`

Connect from any phone, tablet, or laptop — no app required.

### What You Can Configure via Web UI

- Global MIDI channel (1–16)
- Global Latching / Momentary mode toggle
- **Presets** — switch between all 6 presets; save the current settings to the active preset
- Per-footswitch mode (all 26 modes)
- Per-footswitch MIDI CC / PC / Note number
- Per-footswitch MIDI channel override (or Global)
- Per-footswitch ramp UP and DOWN speed (sliders appear only for modulation modes)
- **Footswitch trigger buttons** — activate footswitches directly from the browser (hold for momentary, click-toggle for latching)
- **POT1 / POT2 sliders** — send CC 20 / CC 21 directly from the web UI

### Unsaved Changes

Any setting change marks the active preset as unsaved. A `● Unsaved` badge appears in the web UI header. Click **Save Preset** to write the current state to the active preset slot in non-volatile memory.

### WiFi LED

GPIO 22 drives a WiFi status LED. It is HIGH while the access point is running and LOW when the LOCK switch is engaged.

### WiFi and LOCK

WiFi turns off automatically when the LOCK switch is engaged and restarts when it is disengaged. There is no separate WiFi on/off toggle — the LOCK switch is the only way to disable it.

### REST API

| Method | Path | Body | Action |
|--------|------|------|--------|
| GET | `/api/state` | — | Full state JSON (channel, latching, activePreset, presetDirty, all 6 buttons) |
| GET | `/api/presets` | — | All 6 preset slots + active index |
| POST | `/api/channel` | `{"channel":0}` | Set global MIDI channel (0–15) |
| POST | `/api/latching` | `{"latching":true}` | Set global latching mode |
| POST | `/api/button/:id` | `{"modeIndex":4,"midiNumber":11,"fsChannel":255,"rampUpMs":1000,"rampDownMs":1000}` | Update footswitch config |
| POST | `/api/button/:id/press` | — | Simulate footswitch press |
| POST | `/api/button/:id/release` | — | Simulate footswitch release |
| POST | `/api/pot` | `{"id":0,"value":64}` | Send CC 20 (id=0) or CC 21 (id=1) |
| POST | `/api/preset/load/:id` | — | Apply preset N (0–5) to live state |
| POST | `/api/preset/save/:id` | — | Save current live state to preset N |

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
- For each of 6 footswitches: mode, MIDI number, per-footswitch channel, ramp UP/DOWN speed

Settings are **not auto-saved**. Changes are held in memory until you explicitly save (long-press PRESET button or click Save Preset in the web UI).

On first boot after a firmware update from an older version, the old single-preset settings are migrated into preset slot 1. All other slots initialize to factory defaults.

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
