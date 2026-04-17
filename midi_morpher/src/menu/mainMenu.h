#pragma once
#include "../globalSettings.h"
#include "../clock/midiClock.h"
#include "../pedalState.h"
#include "../statePersistance.h"
#include "../visual/display.h"
#include "../controls/pots.h"

// ── Menu item indices ──────────────────────────────────────────────────────────
#define MENU_MIDI_CH    0
#define MENU_ROUTINGS   1
#define MENU_POT1_CC    2
#define MENU_POT2_CC    3
#define MENU_EXP_CC     4
#define MENU_LEDS       5
#define MENU_TEMPO_LED  6
#define MENU_NEOPIXEL   7
#define MENU_BRIGHTNESS 8
#define MENU_TIMEOUT    9
#define MENU_LOCK       10
#define MENU_EXIT       11
#define MENU_COUNT      12

static const char *menuItemNames[] = {
    "MIDI Channel", "Routings",  "Pot 1 CC",  "Pot 2 CC",
    "Exp In CC",    "LEDs",      "Tempo LED", "NeoPixel",
    "Brightness",   "Screen Off","Lock",      "Exit"
};

static const char *ledModeNames[] = { "On", "Cnsrv", "Off" };

// ── Routing sub-menu ───────────────────────────────────────────────────────────
#define NUM_ROUTES 6
static const char    *routingNames[] = {
    "DIN->BLE","DIN->USB","USB->DIN","USB->BLE","BLE->DIN","BLE->USB"
};
static const uint8_t  routingBits[]  = {
    ROUTE_DIN_BLE, ROUTE_DIN_USB, ROUTE_USB_DIN,
    ROUTE_USB_BLE, ROUTE_BLE_DIN, ROUTE_BLE_USB
};

// ── Helpers ───────────────────────────────────────────────────────────────────

inline void applyGlobalSettings(PedalState &pedal) {
    applyDisplayContrast(pedal.globalSettings.displayBrightness);
    analogPots[0].midiCCNumber = pedal.globalSettings.pot1CC;
    analogPots[1].midiCCNumber = pedal.globalSettings.pot2CC;
}

inline uint8_t stepPotCC(uint8_t current, int delta) {
    if(current == POT_CC_OFF) return (delta > 0) ? 0 : POT_CC_OFF;
    int next = (int)current + delta;
    if(next < 0) return POT_CC_OFF;
    if(next > 127) return 127;
    return (uint8_t)next;
}

// Right-side status string shown in the root menu list.
inline String _menuItemRhs(const PedalState &pedal, uint8_t item) {
    switch(item) {
        case MENU_MIDI_CH:    return String(pedal.midiChannel + 1);
        case MENU_POT1_CC:
            return pedal.globalSettings.pot1CC == POT_CC_OFF
                   ? String(F("Off")) : String(F("CC")) + String(pedal.globalSettings.pot1CC + 1);
        case MENU_POT2_CC:
            return pedal.globalSettings.pot2CC == POT_CC_OFF
                   ? String(F("Off")) : String(F("CC")) + String(pedal.globalSettings.pot2CC + 1);
        case MENU_EXP_CC:     return String(F("CC")) + String(pedal.globalSettings.expCC + 1);
        case MENU_LEDS: {
            uint8_t m = pedal.globalSettings.ledMode;
            return String(m < 3 ? ledModeNames[m] : ledModeNames[0]);
        }
        case MENU_TEMPO_LED:  return String(pedal.globalSettings.tempoLedEnabled ? F("ON") : F("OFF"));
        case MENU_NEOPIXEL:   return String(pedal.globalSettings.neoPixelEnabled ? F("ON") : F("OFF"));
        case MENU_BRIGHTNESS: return String(pedal.globalSettings.displayBrightness) + String('%');
        case MENU_TIMEOUT:    return String(DISP_TIMEOUT_NAMES[pedal.globalSettings.displayTimeoutIdx]);
        default:              return String();
    }
}

// ── Display functions ─────────────────────────────────────────────────────────

inline void displayMenuRoot(PedalState &pedal) {
    displayMode = DISPLAY_PARAM;
    lastInteraction = millis();
    display.clearDisplay();
    display.invertDisplay(false);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(40, 0);
    display.print(F("-- MENU --"));
    display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

    uint8_t idx = pedal.menuItemIdx;
    int8_t start = (int8_t)idx - 2;
    if(start < 0) start = 0;
    if(start > (int8_t)(MENU_COUNT - 5)) start = (int8_t)(MENU_COUNT - 5);

    for(uint8_t i = 0; i < 5; i++) {
        uint8_t item = (uint8_t)start + i;
        if(item >= MENU_COUNT) break;
        int y = 11 + i * 10;
        display.setCursor(0, y);
        display.print(item == idx ? '>' : ' ');
        display.print(menuItemNames[item]);

        String rhs = _menuItemRhs(pedal, item);
        if(rhs.length() > 0) {
            display.setCursor(128 - (int)rhs.length() * 6, y);
            display.print(rhs);
        }
    }
    display.display();
}

inline void displayMenuEditing(PedalState &pedal) {
    displayMode = DISPLAY_PARAM;
    lastInteraction = millis();
    display.clearDisplay();
    display.invertDisplay(false);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(menuItemNames[pedal.menuItemIdx]);
    display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

    switch(pedal.menuItemIdx) {
        case MENU_MIDI_CH:
            display.setTextSize(3);
            display.setCursor(0, 18);
            display.print(pedal.midiChannel + 1);
            break;
        case MENU_POT1_CC:
        case MENU_POT2_CC: {
            uint8_t cc = (pedal.menuItemIdx == MENU_POT1_CC)
                         ? pedal.globalSettings.pot1CC
                         : pedal.globalSettings.pot2CC;
            display.setTextSize(2);
            display.setCursor(0, 22);
            if(cc == POT_CC_OFF) display.print(F("Off"));
            else { display.print(F("CC ")); display.print(cc + 1); }
            break;
        }
        case MENU_EXP_CC:
            display.setTextSize(2);
            display.setCursor(0, 22);
            display.print(F("CC "));
            display.print(pedal.globalSettings.expCC + 1);
            break;
        case MENU_LEDS: {
            uint8_t m = pedal.globalSettings.ledMode;
            display.setTextSize(2);
            display.setCursor(0, 22);
            static const char *fullNames[] = { "On", "Conservative", "Off" };
            display.print(fullNames[m < 3 ? m : 0]);
            break;
        }
        case MENU_BRIGHTNESS:
            display.setTextSize(2);
            display.setCursor(0, 22);
            display.print(pedal.globalSettings.displayBrightness);
            display.print('%');
            break;
        case MENU_TIMEOUT:
            display.setTextSize(2);
            display.setCursor(0, 22);
            display.print(DISP_TIMEOUT_NAMES[pedal.globalSettings.displayTimeoutIdx]);
            break;
        default:
            break;
    }

    display.setTextSize(1);
    display.setCursor(2, 56);
    display.print(F("Scroll / Press=Back"));
    display.display();
}

inline void displayMenuRouting(PedalState &pedal) {
    displayMode = DISPLAY_PARAM;
    lastInteraction = millis();
    display.clearDisplay();
    display.invertDisplay(false);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Routings"));
    display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

    for(uint8_t i = 0; i < NUM_ROUTES; i++) {
        int y = 11 + i * 9;
        display.setCursor(0, y);
        display.print(i == pedal.menuRoutingIdx ? '>' : ' ');
        display.print(routingNames[i]);
        bool on = (pedal.globalSettings.routingFlags & routingBits[i]) != 0;
        display.setCursor(92, y);
        display.print(on ? F("ON ") : F("OFF"));
    }
    display.display();
}

inline void displayMenuLockConfirm(PedalState &pedal) {
    displayMode = DISPLAY_PARAM;
    lastInteraction = millis();
    display.clearDisplay();
    display.invertDisplay(false);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Lock Settings?"));
    display.setTextSize(2);
    display.setCursor(0, 20);
    if(pedal.menuRoutingIdx == 0) display.print(F(">NO   YES"));
    else                           display.print(F(" NO  >YES"));
    display.setTextSize(1);
    display.setCursor(2, 52);
    display.print(F("Scroll / Press=OK"));
    display.display();
}

// ── Navigation handlers ───────────────────────────────────────────────────────

inline void handleMenuRotate(PedalState &pedal, int delta) {
    switch(pedal.menuState) {
        case MenuState::ROOT: {
            int next = constrain((int)pedal.menuItemIdx + delta, 0, MENU_COUNT - 1);
            pedal.menuItemIdx = (uint8_t)next;
            displayMenuRoot(pedal);
            break;
        }
        case MenuState::EDITING: {
            switch(pedal.menuItemIdx) {
                case MENU_MIDI_CH: {
                    uint8_t ch = (uint8_t)constrain((int)pedal.midiChannel + delta, 0, 15);
                    pedal.setMidiChannel(ch);
                    markStateDirty();
                    displayMenuEditing(pedal);
                    break;
                }
                case MENU_POT1_CC:
                    pedal.globalSettings.pot1CC = stepPotCC(pedal.globalSettings.pot1CC, delta);
                    analogPots[0].midiCCNumber  = pedal.globalSettings.pot1CC;
                    displayMenuEditing(pedal);
                    break;
                case MENU_POT2_CC:
                    pedal.globalSettings.pot2CC = stepPotCC(pedal.globalSettings.pot2CC, delta);
                    analogPots[1].midiCCNumber  = pedal.globalSettings.pot2CC;
                    displayMenuEditing(pedal);
                    break;
                case MENU_EXP_CC: {
                    int next = constrain((int)pedal.globalSettings.expCC + delta, 0, 127);
                    pedal.globalSettings.expCC = (uint8_t)next;
                    displayMenuEditing(pedal);
                    break;
                }
                case MENU_LEDS: {
                    int m = constrain((int)pedal.globalSettings.ledMode + delta, 0, 2);
                    pedal.globalSettings.ledMode = (uint8_t)m;
                    displayMenuEditing(pedal);
                    break;
                }
                case MENU_BRIGHTNESS: {
                    int b = constrain((int)pedal.globalSettings.displayBrightness + delta, 0, 100);
                    pedal.globalSettings.displayBrightness = (uint8_t)b;
                    applyDisplayContrast((uint8_t)b);
                    displayMenuEditing(pedal);
                    break;
                }
                case MENU_TIMEOUT: {
                    int t = constrain((int)pedal.globalSettings.displayTimeoutIdx + delta, 0, NUM_DISP_TIMEOUTS - 1);
                    pedal.globalSettings.displayTimeoutIdx = (uint8_t)t;
                    displayMenuEditing(pedal);
                    break;
                }
                default: break;
            }
            break;
        }
        case MenuState::ROUTING: {
            int next = constrain((int)pedal.menuRoutingIdx + delta, 0, NUM_ROUTES - 1);
            pedal.menuRoutingIdx = (uint8_t)next;
            displayMenuRouting(pedal);
            break;
        }
        case MenuState::LOCK_CONFIRM:
            pedal.menuRoutingIdx = (pedal.menuRoutingIdx == 0) ? 1 : 0;
            displayMenuLockConfirm(pedal);
            break;
        default:
            break;
    }
}

inline void handleMenuPress(PedalState &pedal) {
    switch(pedal.menuState) {
        case MenuState::ROOT:
            switch(pedal.menuItemIdx) {
                case MENU_MIDI_CH:
                case MENU_POT1_CC:
                case MENU_POT2_CC:
                case MENU_EXP_CC:
                case MENU_LEDS:
                case MENU_BRIGHTNESS:
                case MENU_TIMEOUT:
                    pedal.menuState = MenuState::EDITING;
                    displayMenuEditing(pedal);
                    break;
                case MENU_ROUTINGS:
                    pedal.menuState      = MenuState::ROUTING;
                    pedal.menuRoutingIdx = 0;
                    displayMenuRouting(pedal);
                    break;
                case MENU_TEMPO_LED:
                    pedal.globalSettings.tempoLedEnabled = !pedal.globalSettings.tempoLedEnabled;
                    if(!pedal.globalSettings.tempoLedEnabled)
                        digitalWrite(TEMPO_LED_PIN, LOW);
                    saveGlobalSettings(pedal);
                    displayMenuRoot(pedal);
                    break;
                case MENU_NEOPIXEL:
                    pedal.globalSettings.neoPixelEnabled = !pedal.globalSettings.neoPixelEnabled;
                    saveGlobalSettings(pedal);
                    displayMenuRoot(pedal);
                    break;
                case MENU_LOCK:
                    pedal.menuState      = MenuState::LOCK_CONFIRM;
                    pedal.menuRoutingIdx = 0;
                    displayMenuLockConfirm(pedal);
                    break;
                case MENU_EXIT:
                    pedal.menuState = MenuState::NONE;
                    displayHomeScreen(pedal);
                    break;
                default: break;
            }
            break;

        case MenuState::EDITING:
            saveGlobalSettings(pedal);
            pedal.menuState = MenuState::ROOT;
            displayMenuRoot(pedal);
            break;

        case MenuState::ROUTING:
            pedal.globalSettings.routingFlags ^= routingBits[pedal.menuRoutingIdx];
            saveGlobalSettings(pedal);
            displayMenuRouting(pedal);
            break;

        case MenuState::LOCK_CONFIRM:
            if(pedal.menuRoutingIdx == 1) {
                pedal.settingsLocked = true;
                pedal.menuState      = MenuState::NONE;
                displayLockChange(true);
            } else {
                pedal.menuState = MenuState::ROOT;
                displayMenuRoot(pedal);
            }
            break;

        default: break;
    }
}

inline void handleMenuLongPress(PedalState &pedal) {
    if(pedal.menuState == MenuState::ROUTING) {
        pedal.menuState = MenuState::ROOT;
        displayMenuRoot(pedal);
    }
}
