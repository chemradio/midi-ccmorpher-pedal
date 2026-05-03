# MIDI Receiver

Companion firmware for the [MIDI Morpher](../README.md). Receives MIDI
wirelessly from the Morpher and re-outputs it locally over USB MIDI and/or
hardware MIDI (TRS / 5-pin DIN). Designed for cheap ESP32 dev boards so you can
drop a wireless MIDI endpoint anywhere on stage without running cables.

## Supported boards

| Board                  | USB MIDI | Hardware MIDI | WiFi UDP | BLE | ESP-NOW |
| ---------------------- | :------: | :-----------: | :------: | :-: | :-----: |
| ESP32-S3 (any variant) |    ✓     |       ✓       |    ✓     |  ✓  |    ✓    |
| ESP32-S2               |    ✓     |       ✓       |    ✓     |  —  |    ✓    |
| ESP32-S2 mini          |    ✓     |       ✓       |    ✓     |  —  |    ✓    |

BLE requires a chip with Bluetooth hardware. S2 / S2-mini compile fine without
it and only see two transport slots (WiFi UDP + ESP-NOW).

The default `STATUS_NEOPIXEL_PIN = 48` matches the onboard RGB found on most
ESP32-S3 DevKit boards. For boards without a NeoPixel, set it to `-1` in
`config.h` or move it to a free pin and wire an external WS2812.

## Supported transports

- **WiFi UDP** — joins the Morpher's AP (`MIDI Morpher` / `midimorpher`) and
  listens for broadcast MIDI on UDP port 5005. Status LED: orange.
- **BLE MIDI** — scans for the Morpher's BLE-MIDI service (Apple UUID
  `03B80E5A-…`) and subscribes to incoming notifications. Status LED: blue.
- **ESP-NOW** — pairs with the Morpher via a `MMPR` / `MMPA` handshake and
  receives unicast MIDI frames. Re-pairs automatically every 30 s so it
  recovers from Morpher reboots. Status LED: purple.

Only one transport is active at a time. Selection cycles via the on-board
button and is persisted to NVS, so it survives power cycles.

## Basic operation

1. Wire the MIDI output stage if you want hardware MIDI out. See
   [WIRING.md](WIRING.md) for schematics and pin map. USB MIDI works with no
   extra wiring.
2. Open `midi-receiver.ino` in the Arduino IDE, select your ESP32 board with
   USB CDC enabled, and flash.
3. On the Morpher's web UI (`http://192.168.4.1`), enable the transport(s) you
   want to use (UDP / ESP-NOW). BLE is always advertised.
4. Power up the receiver. The status NeoPixel lights in the active transport's
   color. Short-press the transport-select button (GPIO 4 by default) to cycle
   between WiFi UDP → BLE → ESP-NOW.
5. The receiver appears as a USB MIDI device to a connected computer, and any
   MIDI it receives wirelessly is forwarded to both USB MIDI and the hardware
   MIDI output simultaneously.

## Configuration

All build-time settings live in [config.h](config.h):

- `TRANSPORT_SELECT_BTN_PIN` — momentary-to-GND, internal pull-up.
- `STATUS_NEOPIXEL_PIN` / `STATUS_NEOPIXEL_BRIGHTNESS` — set pin to `-1` to
  disable.
- `MIDI_TX_PIN` — UART TX feeding the MIDI output stage.
- `WIFI_SSID` / `WIFI_PASS` / `UDP_MIDI_PORT` — must match the Morpher's AP.
- `ESPNOW_WIFI_CHANNEL` — must match the Morpher's AP channel (default 1).

Runtime state (active transport index) is stored in NVS namespace `rxcfg`.

## Files

- `midi-receiver.ino` — entry point.
- `config.h` — pins and protocol parameters.
- `src/transport_manager.h` — transport switching, NVS persistence, status LED.
- `src/transport/` — one header per transport (UDP / BLE / ESP-NOW).
- `src/midi_output.h` — fans incoming bytes out to USB MIDI and UART.
- `WIRING.md` — schematics and hardware notes.
