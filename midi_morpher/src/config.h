// config.h — Pin assignments and firmware constants
// Target: ESP32-S3-N16R8, Arduino framework

#pragma once
#include <Arduino.h>

// ── Footswitches (onboard) ────────────────────────────────────────────────────
#define FS1_PIN      4
#define FS1_LED      5
#define FS2_PIN      6
#define FS2_LED      7
#define FS3_PIN     14
#define FS3_LED     15
#define FS4_PIN     16
#define FS4_LED     21

// ── Footswitches (external, via jack) ────────────────────────────────────────
#define EXTFS1_PIN  17
#define EXTFS1_LED  18
#define EXTFS2_PIN   8
#define EXTFS2_LED   3

// ── WiFi status LED ───────────────────────────────────────────────────────────
#define WIFI_LED_PIN 22

// ── Encoder ───────────────────────────────────────────────────────────────────
#define ENC_A        9
#define ENC_B       10
#define ENC_BTN     11

// ── Analog inputs ─────────────────────────────────────────────────────────────
#define POT1_PIN     1   // UP speed / CC pot
#define POT2_PIN     2   // DOWN speed / CC pot
#define EXP_IN      12   // Expression pedal input

// ── Toggle switches ───────────────────────────────────────────────────────────
#define MS2_PIN     13   // HotSwitch: Momentary / Latching
#define LST_PIN     47   // Lock settings

// ── I2C display (SSD1306 OLED) ────────────────────────────────────────────────
#define SDA_PIN         41
#define SCL_PIN         42
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDRESS  0x3C

// ── Status LED (onboard NeoPixel) ─────────────────────────────────────────────
#define RGB_PIN         48
#define NEOPIXEL_COUNT   1

// ── Digital potentiometer (AD5292-BRUZ-20 — analog expression output) ────────
#define DIGIPOT_CS   38   // SYNC (active low chip select)
#define DIGIPOT_SCK  39   // SCLK
#define DIGIPOT_MOSI 40   // SDI

// ── MIDI serial ───────────────────────────────────────────────────────────────
#define MIDI_TX     43
#define MIDI_RX     36

// ── Timing ────────────────────────────────────────────────────────────────────
#define DEBOUNCE_DELAY           80   // ms — footswitch & button debounce
#define DISPLAY_TIMEOUT        2000   // ms — temp screen hold before home screen returns
#define CHANNEL_SELECT_HOLD_MS  600   // ms — encoder button hold time to enter per-FS channel select

// ── Sentinel ──────────────────────────────────────────────────────────────────
#define NO_LED_PIN 255   // Marker for controls without a status LED

// ── MIDI CC assignments ───────────────────────────────────────────────────────
#define POT1_CC  20   // CC sent by UP pot in send-CC mode
#define POT2_CC  21   // CC sent by DOWN pot in send-CC mode

// ── Modulation ────────────────────────────────────────────────────────────────
#define DEFAULT_RAMP_SPEED     1000   // ms — per-footswitch ramp time on first boot
#define RAMP_DURATIONS_MIN_MS     0   // ms — pot full left  = instant
#define RAMP_DURATIONS_MAX_MS  5000   // ms — pot full right = 5 s

// ── Presets ───────────────────────────────────────────────────────────────────
// GPIO 19/20 are the ESP32-S3 USB D-/D+ lines and cannot be used as GPIO when
// USB is active — moved to GPIO 44 / 45.
#define PRESET_BTN_PIN      44    // Momentary preset select / save button (U0RXD, UART0 unused)
#define ACTIVITY_LED_PIN    45    // Lights while the last-pressed footswitch is active (strap; keep LOW at reset)
#define NUM_PRESETS          6
#define PRESET_SAVE_HOLD_MS 1500  // ms — hold to save current state to active preset

// ── Display layout ────────────────────────────────────────────────────────────
// Only the 4 onboard footswitches fit on the 64 px tall OLED home screen.
// Increase this if you ever swap to a taller display.
static constexpr uint8_t DISPLAY_FS_ROWS = 4;

// ── MIDI Clock ────────────────────────────────────────────────────────────────
// CLOCK_SYNC_FLAG is OR'd into rampUpMs / rampDownMs (bit 31) to signal that
// the low byte is a note-value index rather than milliseconds.
// Valid ms values are 0–5000, so bit 31 is always clear for plain ms.
#define CLOCK_SYNC_FLAG       0x80000000UL
#define NUM_NOTE_VALUES       17           // 1/32T … 2/1, incl. dotted & triplets
#define DEFAULT_BPM           120.0f
#define BPM_MIN               20.0f
#define BPM_MAX               300.0f
#define CLOCK_SYNC_TIMEOUT_MS 2000         // ms with no 0xF8 → revert to internal

// ── Tempo LED ─────────────────────────────────────────────────────────────────
// GPIOs 33–37 are reserved for octal PSRAM on N16R8 — never use.
// GPIO 46 is a strapping pin but defaults LOW at reset, which is compatible
// with a standard LED wired anode→pin, cathode→resistor→GND.
#define TEMPO_LED_PIN 46
