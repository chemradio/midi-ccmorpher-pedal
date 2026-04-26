#pragma once
#include <Arduino.h>

// ── MIDI routing flag bits ─────────────────────────────────────────────────────
#define ROUTE_DIN_BLE  0x01
#define ROUTE_DIN_USB  0x02
#define ROUTE_USB_DIN  0x04
#define ROUTE_USB_BLE  0x08
#define ROUTE_BLE_DIN  0x10
#define ROUTE_BLE_USB  0x20
#define ROUTE_ALL      0x3F

// ── Pot CC "disabled" sentinel ─────────────────────────────────────────────────
#define POT_CC_OFF 0xFF

// ── LED modes ─────────────────────────────────────────────────────────────────
#define LED_MODE_ON           0  // always full on
#define LED_MODE_CONSERVATIVE 1  // blink on preset load/save, otherwise fully off
#define LED_MODE_OFF          2
#define CONSERVATIVE_ACTIVITY_BLINK_MS  240 // full period (on + off) after preset load
#define CONSERVATIVE_ACTIVITY_BLINK_DUR 960 // total duration of activity blink burst (ms)

// ── Display timeout table (index 0–3) ─────────────────────────────────────────
static constexpr uint32_t    DISP_TIMEOUT_MS[]    = {2000, 5000, 10000, 0};
static constexpr const char *DISP_TIMEOUT_NAMES[] = {"2s", "5s", "10s", "Always"};
static constexpr uint8_t     NUM_DISP_TIMEOUTS    = 4;
static constexpr uint8_t     DISP_TIMEOUT_DEF_IDX = 0; // 2 s

// ── Menu state ────────────────────────────────────────────────────────────────
enum class MenuState : uint8_t {
    NONE,
    ROOT,
    EDITING,
    ROUTING,
    LOCK_CONFIRM,
};

// ── Persisted global settings (not per-preset) ────────────────────────────────
struct GlobalSettings {
    uint8_t routingFlags      = ROUTE_ALL;
    uint8_t ledMode           = LED_MODE_ON;  // LED_MODE_ON / _CONSERVATIVE / _OFF
    bool    tempoLedEnabled   = true;
    bool    neoPixelEnabled   = true;
    uint8_t displayBrightness = 78;           // percent 0–100
    uint8_t displayTimeoutIdx = DISP_TIMEOUT_DEF_IDX;
    uint8_t pot1CC            = 20;           // 0–127 = CC num; POT_CC_OFF = disabled
    uint8_t pot2CC            = 21;
    uint8_t  expCC            = 20;           // expression input CC number (0–127)
    bool     expWakesDisplay  = true;         // show exp value on OLED when pedal moves
    uint16_t expCalMin        = 0xFFFF;       // 0xFFFF = not calibrated (auto-cal takes over)
    uint16_t expCalMax        = 0;
    bool     perFsModulator   = true;         // true = independent modulator per footswitch
    bool     clockGenerate    = true;         // generate and send internal MIDI clock
    bool     clockOutput      = true;         // forward clock (internal/external) to MIDI outputs
    bool     captivePortal    = true;         // redirect all unknown URLs to UI (OS "Sign in" prompt)
    uint8_t  presetCount      = 6;            // 1–128: PRESET button and web UI cycle within [0, presetCount)
};
