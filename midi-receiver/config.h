#pragma once

// ── Transport select button ───────────────────────────────────────────────────
// Short press cycles: WiFi UDP → ESP-NOW → …
// Selection is persisted to NVS (namespace "rxcfg", key "transport").
// Wire a momentary switch to GND; internal pull-up is enabled.
#define TRANSPORT_SELECT_BTN_PIN  4   // free GPIO, not a strapping pin

// ── Status NeoPixel ───────────────────────────────────────────────────────────
// Single WS2812 lit in a per-transport color: WiFi UDP = orange,
// ESP-NOW = purple. Set pin to -1 to disable.
#define STATUS_NEOPIXEL_PIN         48   // GPIO 48 = onboard RGB on most ESP32-S3 DevKits
#define STATUS_NEOPIXEL_BRIGHTNESS  26   // ~10% of 255

// ── MIDI output ───────────────────────────────────────────────────────────────
// UART TX feeds the MIDI output stage. See WIRING.md for schematic options.
#define MIDI_TX_PIN  17

// ── MIDI activity LED ─────────────────────────────────────────────────────────
// Built-in blue LED on most ESP32-S3 DevKit-C1 boards. Pulses briefly whenever
// MIDI bytes are received. Set to -1 to disable.
#define MIDI_ACTIVITY_LED_PIN  38
#define MIDI_ACTIVITY_LED_MS   40   // pulse duration in ms

// ── WiFi UDP transport ────────────────────────────────────────────────────────
#define WIFI_SSID       "MIDI Morpher"
#define WIFI_PASS       "midimorpher"
#define UDP_MIDI_PORT   5005

// ── ESP-NOW transport ─────────────────────────────────────────────────────────
// Must match the WiFi channel of the Morpher's AP (default 1).
#define ESPNOW_WIFI_CHANNEL  1
