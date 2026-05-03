# MIDI Receiver — Wiring Guide

Companion sketch for ESP32-S3 / S2 / S2-mini that receives MIDI wirelessly
(WiFi UDP / BLE / ESP-NOW) from the MIDI Morpher and re-outputs it via:

- **USB MIDI** — native USB on the ESP32-S2/S3. No extra circuitry. Plug into a
  computer or USB host.
- **Hardware MIDI out** — UART TX on `MIDI_TX_PIN` (default GPIO 17), wired to a
  TRS or 5-pin DIN connector via a small resistor pad.

## Pin map (defaults from `config.h`)

| Function              | GPIO | Notes                                            |
| --------------------- | ---- | ------------------------------------------------ |
| MIDI UART TX          | 17   | 3.3V logic, 31 250 baud, 8N1                     |
| Transport select btn  | 4    | Momentary to GND, internal pull-up               |
| Status NeoPixel       | 48   | Single WS2812; onboard RGB on most S3 DevKits    |

Set `STATUS_NEOPIXEL_PIN` to `-1` in `config.h` if your board has no NeoPixel.

## MIDI output stage (3.3V, recommended)

This is the simplest option and what the firmware assumes. No buffer IC,
no level shifter, no extra power rail — just two resistors.

### TRS Type A (mini-TRS, the modern standard)

```
            +3.3V
              │
            [10 Ω]            (sleeve = GND)
              │
   TRS ring ──┘
   TRS tip  ──[33 Ω]── GPIO 17 (TX)
   TRS slv  ──────────── GND
```

Use a stereo 3.5 mm jack. This matches the MIDI-CA "TRS Type A" pinout used by
Korg, Make Noise, Boss, etc. Cheap TRS-to-DIN adapters work with this wiring.

### 5-pin DIN (classic)

```
            +3.3V
              │
            [220 Ω]
              │
   DIN pin 4 ─┘
   DIN pin 5 ─[220 Ω]── GPIO 17 (TX)
   DIN pin 2 ───────── GND
```

Pin numbering is on the **female** connector, looking at the solder side.
Pins 1 and 3 are unused.

## Why the resistors are not optional

MIDI is a 5 mA current loop. The receiver end has an optocoupler (6N138, H11L1,
etc.) whose LED must be current-limited or it will burn out. The resistors set
that current:

```
I = (Vsupply − Vf_opto) / (R_tip + R_ring)
  ≈ (3.3 V − 1.2 V) / 43 Ω  ≈ 49 mA   ← TRS values, on the high side but OK
                                         for a fraction of a bit-time
  ≈ (3.3 V − 1.2 V) / 440 Ω ≈ 4.8 mA  ← 220 Ω + 220 Ω, textbook
```

Without them, you'll either cook the receiver's opto or sink excessive current
back into the ESP32 GPIO.

## Why 5V from USB is more trouble than it's worth

It's tempting to use the 5V USB rail for the MIDI output to match the original
5V MIDI 1.0 spec. But:

- The ESP32 TX pin only swings to **3.3V**, not 5V.
- In a 5V MIDI circuit, idle state is the full supply voltage. With a 3.3V TX
  driving into a 5V pull-up, the opto LED sees ~1.7V at idle — enough to
  partially light it and corrupt the line.
- Fix requires a level shifter or a hex buffer (74HCT14, 74HC04, etc.) to lift
  TX to 5V. That's the "buffer" you'll see referenced in older Arduino MIDI
  schematics.

If you have a buffer IC and want the classic circuit, wire it as:

```
   GPIO 17 ──► 74HCT04 ──► [220 Ω] ──► DIN pin 5
   +5V ────────────────── [220 Ω] ──► DIN pin 4
   GND ────────────────────────────── DIN pin 2
   74HCT04 powered from +5V; one inverter unused or use a non-inverting buffer.
```

Note that `74HCT04` is **inverting** — you'd need two stages (or a non-inverting
74HCT34 / 74HC4050) to keep MIDI polarity correct. Easier to just stay on 3.3V.

## Status NeoPixel colors

| Transport | Color  | Hex      |
| --------- | ------ | -------- |
| WiFi UDP  | Orange | `FF8000` |
| BLE       | Blue   | `0000FF` |
| ESP-NOW   | Purple | `A020F0` |

Brightness is `STATUS_NEOPIXEL_BRIGHTNESS` (default 26 ≈ 10%). The pixel
blinks while the active transport is connecting and goes solid once
connected.

## Transport select button

Short press cycles transports in order. Selection is persisted to NVS
(`rxcfg` / `transport`) and restored on next boot. Wire any momentary switch
between `TRANSPORT_SELECT_BTN_PIN` and GND.
