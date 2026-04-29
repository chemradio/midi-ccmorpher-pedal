#pragma once
#include "../analogInOut/expInput.h"
#include "../clock/midiClock.h"
#include "../globalSettings.h"
#include "../pedalState.h"
#include "../statePersistance.h"
#include "../visual/display.h"

// ── Menu item indices ──────────────────────────────────────────────────────────
#define MENU_MIDI_CH       0
#define MENU_ROUTINGS      1
#define MENU_ENC_ACTION    2
#define MENU_ENC_CC        3
#define MENU_ENC_KEY_R     4
#define MENU_ENC_KEY_L     5
#define MENU_EXP_CC        6
#define MENU_EXP_CAL       7
#define MENU_EXP_WAKE      8
#define MENU_LEDS          9
#define MENU_TEMPO_LED     10
#define MENU_PER_FS_MOD    11
#define MENU_CLOCK_GEN     12
#define MENU_CLOCK_OUT     13
#define MENU_BRIGHTNESS    14
#define MENU_TIMEOUT       15
#define MENU_PRESET_COUNT  16
#define MENU_PRESET_ACTION 17
#define MENU_LOCK          18
#define MENU_EXIT          19
#define MENU_COUNT         20

static const char *menuItemNames[] = {
    "MIDI Channel", "Routings", "Enc Action", "Enc CC#", "Enc > Key", "Enc < Key",
    "Exp In CC", "Exp Cal", "Exp Wake", "LEDs", "Tempo LED",
    "Poly Mod", "Clock Gen", "Clock Out", "Brightness", "Screen ON",
    "Presets", "Preset Action", "Lock", "Exit"};

static const char *ledModeNames[] = {"On", "Cnsrv", "Off"};

// ── Routing sub-menu ───────────────────────────────────────────────────────────
#define NUM_ROUTES 6
static const char *routingNames[] = {
    "DIN->BLE", "DIN->USB", "USB->DIN", "USB->BLE", "BLE->DIN", "BLE->USB"};
static const uint8_t routingBits[] = {
    ROUTE_DIN_BLE, ROUTE_DIN_USB, ROUTE_USB_DIN, ROUTE_USB_BLE, ROUTE_BLE_DIN, ROUTE_BLE_USB};

// ── Helpers ───────────────────────────────────────────────────────────────────

inline void applyGlobalSettings(PedalState &pedal) {
  applyDisplayContrast(pedal.globalSettings.displayBrightness);
}

static const char *encActionNames[] = {"Tempo", "CC", "Key"};

inline bool menuItemVisible(uint8_t item, const PedalState &pedal) {
  if(item == MENU_ENC_CC)
    return pedal.globalSettings.encoderAction == EncoderAction::CC;
  if(item == MENU_ENC_KEY_R || item == MENU_ENC_KEY_L)
    return pedal.globalSettings.encoderAction == EncoderAction::KEY;
  return true;
}

// Build a list of currently-visible menu item indices. Returns count.
inline uint8_t buildVisibleMenu(const PedalState &pedal, uint8_t *out) {
  uint8_t n = 0;
  for(uint8_t i = 0; i < MENU_COUNT; i++)
    if(menuItemVisible(i, pedal)) out[n++] = i;
  return n;
}

// Right-side status string shown in the root menu list.
inline String _menuItemRhs(const PedalState &pedal, uint8_t item) {
  switch(item) {
  case MENU_MIDI_CH:
    return String(pedal.midiChannel + 1);
  case MENU_ENC_ACTION: {
    uint8_t a = (uint8_t)pedal.globalSettings.encoderAction;
    return String(a < 3 ? encActionNames[a] : encActionNames[0]);
  }
  case MENU_ENC_CC:
    return String(F("CC")) + String(pedal.globalSettings.encoderCCNum + 1);
  case MENU_ENC_KEY_R: {
    uint8_t k = pedal.globalSettings.encoderKeyRight;
    return String(k < NUM_HID_KEYS ? hidKeys[k].name : "?");
  }
  case MENU_ENC_KEY_L: {
    uint8_t k = pedal.globalSettings.encoderKeyLeft;
    return String(k < NUM_HID_KEYS ? hidKeys[k].name : "?");
  }
  case MENU_EXP_CC:
    return pedal.globalSettings.expCC == POT_CC_OFF
                ? String(F("Off"))
                : String(F("CC")) + String(pedal.globalSettings.expCC + 1);
  case MENU_EXP_CAL:
    return String();
  case MENU_EXP_WAKE:
    return String(pedal.globalSettings.expWakesDisplay ? F("ON") : F("OFF"));
  case MENU_LEDS: {
    uint8_t m = pedal.globalSettings.ledMode;
    return String(m < 3 ? ledModeNames[m] : ledModeNames[0]);
  }
  case MENU_TEMPO_LED:
    return String(pedal.globalSettings.tempoLedEnabled ? F("ON") : F("OFF"));
  case MENU_PER_FS_MOD:
    return String(pedal.globalSettings.perFsModulator ? F("ON") : F("OFF"));
  case MENU_CLOCK_GEN:
    return String(pedal.globalSettings.clockGenerate ? F("ON") : F("OFF"));
  case MENU_CLOCK_OUT: {
    bool avail = pedal.globalSettings.clockGenerate || midiClock.externalSync;
    return avail ? String(pedal.globalSettings.clockOutput ? F("ON") : F("OFF"))
                 : String(F("N/A"));
  }
  case MENU_BRIGHTNESS:
    return String(pedal.globalSettings.displayBrightness) + String('%');
  case MENU_TIMEOUT:
    return String(DISP_TIMEOUT_NAMES[pedal.globalSettings.displayTimeoutIdx]);
  case MENU_PRESET_COUNT:
    return String(pedal.globalSettings.presetCount);
  case MENU_PRESET_ACTION: {
    const FSActionPersisted &la = pedal.liveLoadAction;
    if(!la.enabled) return String(F("Off"));
    uint8_t mi = la.modeIndex < NUM_MODES ? la.modeIndex : 0;
    return String(modes[mi].name);
  }
  default:
    return String();
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

  uint8_t vis[MENU_COUNT];
  uint8_t visCnt = buildVisibleMenu(pedal, vis);

  // Find cursor position in the visible list
  uint8_t visIdx = 0;
  for(uint8_t i = 0; i < visCnt; i++)
    if(vis[i] == pedal.menuItemIdx) { visIdx = i; break; }

  int8_t start = (int8_t)visIdx - 2;
  if(start < 0) start = 0;
  if(start > (int8_t)visCnt - 5) start = (visCnt >= 5) ? (int8_t)(visCnt - 5) : 0;

  for(uint8_t i = 0; i < 5; i++) {
    int8_t vi = start + (int8_t)i;
    if(vi < 0 || vi >= (int8_t)visCnt) break;
    uint8_t item = vis[vi];
    int y = 11 + i * 10;
    display.setCursor(0, y);
    display.print(item == pedal.menuItemIdx ? '>' : ' ');
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
  case MENU_ENC_ACTION: {
    uint8_t a = (uint8_t)pedal.globalSettings.encoderAction;
    display.setTextSize(2);
    display.setCursor(0, 22);
    display.print(a < 3 ? encActionNames[a] : encActionNames[0]);
    break;
  }
  case MENU_ENC_CC: {
    display.setTextSize(2);
    display.setCursor(0, 22);
    display.print(F("CC "));
    display.print(pedal.globalSettings.encoderCCNum + 1);
    break;
  }
  case MENU_ENC_KEY_R:
  case MENU_ENC_KEY_L: {
    uint8_t k = (pedal.menuItemIdx == MENU_ENC_KEY_R)
                    ? pedal.globalSettings.encoderKeyRight
                    : pedal.globalSettings.encoderKeyLeft;
    display.setTextSize(2);
    display.setCursor(0, 22);
    display.print(k < NUM_HID_KEYS ? hidKeys[k].name : "?");
    break;
  }
  case MENU_EXP_CC: {
    uint8_t cc = pedal.globalSettings.expCC;
    display.setTextSize(2);
    display.setCursor(0, 22);
    if (cc == POT_CC_OFF) display.print(F("Off"));
    else { display.print(F("CC ")); display.print(cc + 1); }
    break;
  }
  case MENU_LEDS: {
    uint8_t m = pedal.globalSettings.ledMode;
    display.setTextSize(2);
    display.setCursor(0, 22);
    static const char *fullNames[] = {"On", "Conservative", "Off"};
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
  case MENU_PER_FS_MOD:
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print(pedal.globalSettings.perFsModulator ? F("ON") : F("OFF"));
    display.setTextSize(1);
    display.setCursor(0, 38);
    display.print(F("Single mod: clean"));
    display.setCursor(0, 48);
    display.print(F("ramps, baud safe"));
    break;
  case MENU_PRESET_COUNT:
    display.setTextSize(3);
    display.setCursor(0, 18);
    display.print(pedal.globalSettings.presetCount);
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
  if(pedal.menuRoutingIdx == 0)
    display.print(F(">NO   YES"));
  else
    display.print(F(" NO  >YES"));
  display.setTextSize(1);
  display.setCursor(2, 52);
  display.print(F("Scroll / Press=OK"));
  display.display();
}

// ── Navigation handlers ───────────────────────────────────────────────────────

inline void handleMenuRotate(PedalState &pedal, int delta) {
  switch(pedal.menuState) {
  case MenuState::ROOT: {
    uint8_t vis[MENU_COUNT];
    uint8_t visCnt = buildVisibleMenu(pedal, vis);
    // Find cursor in visible list, move by delta steps through visible items only
    int8_t visIdx = 0;
    for(uint8_t i = 0; i < visCnt; i++)
      if(vis[i] == pedal.menuItemIdx) { visIdx = (int8_t)i; break; }
    visIdx = (int8_t)constrain((int)visIdx + delta, 0, (int)visCnt - 1);
    pedal.menuItemIdx = vis[visIdx];
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
    case MENU_ENC_ACTION: {
      int a = constrain((int)pedal.globalSettings.encoderAction + delta, 0, 2);
      pedal.globalSettings.encoderAction = (EncoderAction)a;
      displayMenuEditing(pedal);
      break;
    }
    case MENU_ENC_CC: {
      int cc = constrain((int)pedal.globalSettings.encoderCCNum + delta, 0, 127);
      pedal.globalSettings.encoderCCNum = (uint8_t)cc;
      displayMenuEditing(pedal);
      break;
    }
    case MENU_ENC_KEY_R: {
      int k = constrain((int)pedal.globalSettings.encoderKeyRight + delta, 0, NUM_HID_KEYS - 1);
      pedal.globalSettings.encoderKeyRight = (uint8_t)k;
      displayMenuEditing(pedal);
      break;
    }
    case MENU_ENC_KEY_L: {
      int k = constrain((int)pedal.globalSettings.encoderKeyLeft + delta, 0, NUM_HID_KEYS - 1);
      pedal.globalSettings.encoderKeyLeft = (uint8_t)k;
      displayMenuEditing(pedal);
      break;
    }
    case MENU_EXP_CC: {
      uint8_t cur = pedal.globalSettings.expCC;
      int next = (cur == POT_CC_OFF) ? (delta > 0 ? 0 : -1) : ((int)cur + delta);
      pedal.globalSettings.expCC = (next < 0) ? POT_CC_OFF : (uint8_t)constrain(next, 0, 127);
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
    case MENU_PER_FS_MOD:
      pedal.globalSettings.perFsModulator = !pedal.globalSettings.perFsModulator;
      displayMenuEditing(pedal);
      break;
    case MENU_PRESET_COUNT: {
      int cnt = constrain((int)pedal.globalSettings.presetCount + delta, 1, NUM_PRESETS);
      pedal.globalSettings.presetCount = (uint8_t)cnt;
      displayMenuEditing(pedal);
      break;
    }
    default:
      break;
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
    case MENU_ENC_ACTION:
    case MENU_ENC_CC:
    case MENU_ENC_KEY_R:
    case MENU_ENC_KEY_L:
    case MENU_EXP_CC:
    case MENU_LEDS:
    case MENU_BRIGHTNESS:
    case MENU_TIMEOUT:
    case MENU_PER_FS_MOD:
    case MENU_PRESET_COUNT:
      pedal.menuState = MenuState::EDITING;
      displayMenuEditing(pedal);
      break;
    case MENU_EXP_CAL:
      startExpCalibration();
      pedal.menuState = MenuState::NONE;
      displayExpCalibrating(5);
      break;
    case MENU_EXP_WAKE:
      pedal.globalSettings.expWakesDisplay = !pedal.globalSettings.expWakesDisplay;
      saveGlobalSettings(pedal);
      displayMenuRoot(pedal);
      break;
    case MENU_ROUTINGS:
      pedal.menuState = MenuState::ROUTING;
      pedal.menuRoutingIdx = 0;
      displayMenuRouting(pedal);
      break;
    case MENU_TEMPO_LED:
      pedal.globalSettings.tempoLedEnabled = !pedal.globalSettings.tempoLedEnabled;
      saveGlobalSettings(pedal);
      displayMenuRoot(pedal);
      break;
    case MENU_CLOCK_GEN:
      pedal.globalSettings.clockGenerate = !pedal.globalSettings.clockGenerate;
      saveGlobalSettings(pedal);
      displayMenuRoot(pedal);
      break;
    case MENU_CLOCK_OUT: {
      bool avail = pedal.globalSettings.clockGenerate || midiClock.externalSync;
      if(avail) {
        pedal.globalSettings.clockOutput = !pedal.globalSettings.clockOutput;
        saveGlobalSettings(pedal);
      }
      displayMenuRoot(pedal);
      break;
    }
    case MENU_PRESET_ACTION: {
      // Populate loadActionEditBtn from the live load action, then open cursor edit menu
      const FSActionPersisted &la = pedal.liveLoadAction;
      FSButton &lb = pedal.loadActionEditBtn;
      if(la.enabled && la.modeIndex < NUM_MODES) {
        applyModeIndex(lb, la.modeIndex, &pedal.modulators[0]);
        lb.midiNumber = la.midiNumber;
        lb.fsChannel  = la.fsChannel;
        lb.ccLow      = la.ccLow;
        lb.ccHigh     = (la.ccHigh == 0) ? 127 : la.ccHigh;
        lb.velocity   = (la.velocity == 0) ? 100 : la.velocity;
        lb.rampUpMs   = la.rampUpMs;
        lb.rampDownMs = la.rampDownMs;
      } else {
        applyModeIndex(lb, 0, &pedal.modulators[0]);
        lb.midiNumber = 0;
        lb.fsChannel  = 0xFF;
        lb.ccLow      = 0;
        lb.ccHigh     = 127;
        lb.velocity   = 100;
      }
      pedal.menuState      = MenuState::NONE;
      pedal.inFSEdit       = true;
      pedal.fsEditFSIdx    = FS_LOAD_ACTION_IDX;
      pedal.fsEditExtraType = -1;
      pedal.fsEditCursor   = 0;
      pedal.fsEditEditing  = false;
      displayFSEditMenu(pedal);
      break;
    }
    case MENU_LOCK:
      pedal.menuState = MenuState::LOCK_CONFIRM;
      pedal.menuRoutingIdx = 0;
      displayMenuLockConfirm(pedal);
      break;
    case MENU_EXIT:
      pedal.menuState = MenuState::NONE;
      displayHomeScreen(pedal);
      break;
    default:
      break;
    }
    break;

  case MenuState::EDITING:
    if(pedal.menuItemIdx == MENU_PER_FS_MOD) {
      for(auto &mod : pedal.modulators) { mod.restingHigh = false; mod.reset(); }
    }
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
      pedal.menuState = MenuState::NONE;
      displayLockChange(true);
    } else {
      pedal.menuState = MenuState::ROOT;
      displayMenuRoot(pedal);
    }
    break;

  default:
    break;
  }
}

inline void handleMenuLongPress(PedalState &pedal) {
  if(pedal.menuState == MenuState::ROUTING) {
    pedal.menuState = MenuState::ROOT;
    displayMenuRoot(pedal);
  }
}
