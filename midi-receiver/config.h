#pragma once

// ── Transport select button ───────────────────────────────────────────────────
// Short press cycles: WiFi UDP → BLE (S3 only) → ESP-NOW → …
// Selection is persisted to NVS (namespace "rxcfg", key "transport").
#define TRANSPORT_SELECT_BTN_PIN  0   // GPIO 0 = BOOT button on most DevKit boards

// ── Status NeoPixel ───────────────────────────────────────────────────────────
// Single WS2812 lit in a per-transport color: WiFi UDP = orange, BLE = blue,
// ESP-NOW = purple. Set pin to -1 to disable.
#define STATUS_NEOPIXEL_PIN         48   // GPIO 48 = onboard RGB on most ESP32-S3 DevKits
#define STATUS_NEOPIXEL_BRIGHTNESS  26   // ~10% of 255

// ── MIDI DIN output ───────────────────────────────────────────────────────────
#define MIDI_TX_PIN  17    // UART TX → mini-TRS or 5-pin DIN via buffer

// ── WiFi UDP transport ────────────────────────────────────────────────────────
#define WIFI_SSID       "MIDI Morpher"
#define WIFI_PASS       "midimorpher"
#define UDP_MIDI_PORT   5005

// ── ESP-NOW transport ─────────────────────────────────────────────────────────
// Must match the WiFi channel of the Morpher's AP (default 1).
#define ESPNOW_WIFI_CHANNEL  1
