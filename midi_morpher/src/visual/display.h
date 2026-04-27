// display.h - OLED display functions
#pragma once
#include "../clock/midiClock.h"
#include "../config.h"
#include "../footswitches/footswitchObject.h"
#include "../pedalState.h"
#include "../statePersistance.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

inline Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum DisplayMode {
  DISPLAY_DEFAULT,
  DISPLAY_PARAM
};

inline unsigned long lastInteraction = 0;
inline DisplayMode displayMode = DISPLAY_DEFAULT;
inline uint8_t displayContrastPct = 78; // kept in sync with globalSettings.displayBrightness
inline bool    displayDimmed      = false;

// ── FS home-screen info ────────────────────────────────────────────────────
// Set by displayFSUpdateHome(); consumed by displayHomeScreen().
inline int8_t        g_homeFSIdx      = -1;
inline FSButton     *g_homeFSBtn      = nullptr;
inline unsigned long g_homeFSMs       = 0;
inline bool          g_homeFSNeedsDraw = false;
static const uint32_t FS_INFO_LINGER_MS = GLOBAL_TIMEOUT_MS;

// Set and persist the display contrast level (0–100 %). Call whenever brightness
// changes — also used to restore brightness after a timeout dim.
inline void applyDisplayContrast(uint8_t pct) {
  displayContrastPct = pct;
  if(!displayDimmed) {
    uint8_t c = (uint8_t)((uint16_t)pct * 255 / 100);
    display.ssd1306_command(0x81);
    display.ssd1306_command(c);
  }
}

// Turn display back on after a timeout blank. No-op if not currently blanked.
inline void undimDisplay() {
  if(!displayDimmed) return;
  displayDimmed = false;
  display.ssd1306_command(0xAF); // SSD1306_DISPLAYON — restores at last-set contrast
}

inline bool initDisplay() {
  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    return false;
  }
  display.clearDisplay();
  display.dim(true);   // keep dark until applyGlobalSettings sets real brightness
  display.display();
  return true;
}

inline void showStartupScreen() {
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println(F("MIDI"));
  display.setCursor(10, 30);
  display.println(F("Morpher"));
  display.display();
  displayMode = DISPLAY_PARAM;
}
inline void displayLockChange(bool locked) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(locked);
  display.setTextSize(3);
  display.setCursor(0, 10);
  display.println(locked ? F("LOCKED") : F("UNLOCKD"));
  display.display();
}

inline void displayLockedMessage(String whoSays = "") {
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(F("Settings"));
  display.println(F("LOCKED"));
  display.print("From");
  display.println(whoSays);
  display.display();
  displayMode = DISPLAY_PARAM;
}

// Compact FS info line for the status bar when a FS fires.
// Format: "[idx+1] [mode≤7] [value] [ch]"  — fits ~21 chars @ size 1.
inline static void _drawFSStatusBar(const FSButton &btn, int8_t idx, uint8_t globalCh) {
  display.setCursor(0, 0);
  display.print(idx + 1);
  display.print(' ');
  const char *m = btn.modMode.name;
  for(uint8_t c = 0; c < 7 && m[c]; c++) display.print(m[c]);
  display.print(' ');
  if(btn.isNote) {
    static const char *nn[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    display.print(nn[btn.midiNumber % 12]);
    display.print((int)(btn.midiNumber / 12) - 1);
  } else if(btn.isPC) {
    display.print(F("PC:"));
    display.print(btn.midiNumber + 1);
  } else if(btn.isScene) {
    display.print(F("V:"));
    display.print(btn.midiNumber + 1);
  } else if(btn.isModSwitch && btn.midiNumber == PB_SENTINEL) {
    display.print(F("PB"));
  } else if(!btn.isKeyboard && !btn.isSystem) {
    display.print(F("CC:"));
    display.print(btn.midiNumber + 1);
  }
  display.print(' ');
  uint8_t ch = (btn.fsChannel != 0xFF) ? btn.fsChannel : globalCh;
  display.print('c');
  display.print(ch + 1);
}

// Returns the parameter string shown on the right side of each FS row.
inline static String _buttonNumStr(const FSButton &btn) {
  if(btn.mode == FootswitchMode::Multi) {
    uint8_t si = btn.midiNumber;
    if(si < MAX_MULTI_SCENES && multiScenes[si].name[0] != '\0')
      return String(multiScenes[si].name);
    return String(F("?"));
  }
  if(btn.isKeyboard) {
    uint8_t idx = btn.midiNumber < NUM_HID_KEYS ? btn.midiNumber : 0;
    String s = String(hidKeys[idx].name);
    // Append modifier abbreviations
    uint8_t m = (btn.fsChannel == 0xFF) ? 0 : btn.fsChannel;
    if(m & KEY_MOD_CTRL)  s = "^" + s;
    if(m & KEY_MOD_SHIFT) s = "S+" + s;
    if(m & KEY_MOD_ALT)   s = "A+" + s;
    if(m & KEY_MOD_GUI)   s = "G+" + s;
    return s;
  }
  if(btn.isNote) {
    static const char *noteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    return String(noteNames[btn.midiNumber % 12]) + String((int)(btn.midiNumber / 12) - 1);
  }
  if(btn.isSystem) {
    uint8_t idx = btn.midiNumber < NUM_SYS_CMDS ? btn.midiNumber : 0;
    return String(systemCommands[idx].name);
  }
  if(btn.isScene) {
    if(btn.modMode.scenePickCC)
      return "CC:" + String(btn.modMode.sceneCC + btn.midiNumber + 1);
    return "V:" + String(btn.midiNumber + 1);
  }
  if(btn.isPC)
    return "PC:" + String(btn.midiNumber + 1);
  if(btn.isModSwitch && btn.midiNumber == PB_SENTINEL)
    return String(F("PB"));
  return "CC:" + String(btn.midiNumber + 1);
}

inline static void _printNoteName(uint8_t note) {
  static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  display.print(names[note % 12]);
  display.print((int)(note / 12) - 1);
}

inline static void _displayRampSpeed(uint32_t raw) {
  if(raw & CLOCK_SYNC_FLAG) {
    uint8_t ni = (uint8_t)(raw & 0xFF);
    if(ni >= NUM_NOTE_VALUES) ni = NUM_NOTE_VALUES - 1;
    display.print(noteValueNames[ni]);
  } else {
    float s = raw / 1000.0f;
    display.print(s, 2);
    display.print(F("s"));
  }
}

inline static int _displayModeName(const char *modeName, int y) {
  bool big = strlen(modeName) <= 10;
  display.setTextSize(big ? 2 : 1);
  display.setCursor(0, y);
  display.print(modeName);
  return y + (big ? 18 : 10);
}

inline static void _displayNumber(const FSButton &button, int y) {
  display.setTextSize(1);
  display.setCursor(0, y);
  if(button.mode == FootswitchMode::Multi) {
    uint8_t si = button.midiNumber;
    if(si < MAX_MULTI_SCENES && multiScenes[si].name[0] != '\0')
      display.print(multiScenes[si].name);
    else
      display.print(F("?"));
    return;
  }
  if(button.isKeyboard) {
    uint8_t idx = button.midiNumber < NUM_HID_KEYS ? button.midiNumber : 0;
    display.print(hidKeys[idx].name);
    uint8_t m = (button.fsChannel == 0xFF) ? 0 : button.fsChannel;
    if(m) {
      display.print(F(" ("));
      if(m & KEY_MOD_CTRL)  display.print('C');
      if(m & KEY_MOD_SHIFT) display.print('S');
      if(m & KEY_MOD_ALT)   display.print('A');
      if(m & KEY_MOD_GUI)   display.print('G');
      display.print(')');
    }
    return;
  }
  if(button.isSystem) {
    uint8_t idx = button.midiNumber < NUM_SYS_CMDS ? button.midiNumber : 0;
    display.print(systemCommands[idx].name);
    return;
  }
  if(button.isScene) {
    if(button.isSceneScroll) {
      if(button.modMode.scenePickCC) {
        display.print("CC: ");
        display.print(button.modMode.sceneCC + button.scrollLastSent + 1);
      } else {
        display.print("CC:");
        display.print(button.modMode.sceneCC + 1);
        display.print(" Val:");
        display.print(button.scrollLastSent + 1);
      }
    } else if(button.modMode.scenePickCC) {
      display.print("CC: ");
      display.print(button.modMode.sceneCC + button.midiNumber + 1);
    } else {
      display.print("CC:");
      display.print(button.modMode.sceneCC + 1);
      display.print(" Val:");
      display.print(button.midiNumber + 1);
    }
  } else if(button.isPC) {
    display.print("PC: ");
    display.print(button.midiNumber + 1);
  } else if(button.isNote) {
    display.print("Note: ");
    _printNoteName(button.midiNumber);
  } else if(button.isModSwitch && button.midiNumber == PB_SENTINEL) {
    display.print(F("Pitch Bend"));
  } else if(!button.isModSwitch && (button.mode == FootswitchMode::MomentaryCC ||
                                     button.mode == FootswitchMode::LatchingCC  ||
                                     button.mode == FootswitchMode::SingleCC)) {
    display.print("CC:");
    display.print(button.midiNumber + 1);
    display.print(" V:");
    display.print(button.mode == FootswitchMode::SingleCC
                    ? button.ccHigh
                    : (button.isActivated ? button.ccHigh : button.ccLow));
  } else {
    display.print("CC: ");
    display.print(button.midiNumber + 1);
  }
}

inline void displayHomeScreen(PedalState &pedal) {
  displayMode = DISPLAY_DEFAULT;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  bool fsInfo = (g_homeFSIdx >= 0) && (g_homeFSBtn != nullptr) &&
                ((millis() - g_homeFSMs) < FS_INFO_LINGER_MS);

  // ── Status bar ─────────────────────────────────────────────────────────────
  display.setCursor(0, 0);
  display.print(F("Ch:"));
  display.print(pedal.midiChannel + 1);
  if(presetDirty) {
    display.setCursor(30, 0);
    display.print('*');
  }
  if(pedal.settingsLocked) {
    display.setCursor(98, 0);
    display.print(F("LOCK"));
  }
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  if(fsInfo) {
    // ── Compact P + BPM row ───────────────────────────────────────────────
    display.setTextSize(1);
    display.setCursor(0, 11);
    display.print(F("P:"));
    display.print(activePreset + 1);
    if(presetDirty) display.print('*');

    char bpmBuf[12];
    snprintf(bpmBuf, sizeof(bpmBuf), "%d%sBPM",
             (int)midiClock.bpm, midiClock.externalSync ? "E" : "");
    display.setCursor(128 - (int)strlen(bpmBuf) * 6, 11);
    display.print(bpmBuf);

    display.drawFastHLine(0, 20, 128, SSD1306_WHITE);

    // ── Sent command: mode name + MIDI number ────────────────────────────
    int y = _displayModeName(g_homeFSBtn->modMode.name, 23);
    _displayNumber(*g_homeFSBtn, y);
    if(g_homeFSBtn->fsChannel != 0xFF) {
      char chBuf[5];
      snprintf(chBuf, sizeof(chBuf), "c%d", g_homeFSBtn->fsChannel + 1);
      display.setCursor(128 - (int)strlen(chBuf) * 6, 56);
      display.print(chBuf);
    }
  } else {
    // ── Big preset number + BPM ───────────────────────────────────────────
    display.setTextSize(1);
    display.setCursor(0, 12);
    display.print('P');
    display.setCursor(110, 12);
    display.print(F("BPM"));
    if(midiClock.externalSync) {
      display.setCursor(98, 12);
      display.print(F("EXT"));
    }

    // Preset digit — size 4 (32px tall)
    display.setTextSize(4);
    display.setCursor(0, 22);
    display.print(activePreset + 1);
    if(presetDirty) {
      display.setTextSize(2);
      display.setCursor(26, 22);
      display.print('*');
    }

    // BPM — size 2 (16px tall), right-aligned
    char bpmBuf[5];
    snprintf(bpmBuf, sizeof(bpmBuf), "%d", (int)midiClock.bpm);
    int bpmX = 128 - (int)strlen(bpmBuf) * 12;
    display.setTextSize(2);
    display.setCursor(bpmX, 26);
    display.print(bpmBuf);
  }

  display.display();
}

// Callback passed to handleFootswitch() — replaces displayFootswitchPress.
// Stores a snapshot of the fired button and requests a home-screen redraw.
// displayMode stays DISPLAY_DEFAULT so no param-revert timer is started.
inline void displayFSUpdateHome(FSButton &btn) {
  g_homeFSBtn      = &btn;
  g_homeFSMs       = millis();
  g_homeFSNeedsDraw = true;
}

inline void resetDisplayTimeout(PedalState &pedal) {
  static unsigned long prevLastInteraction = 0;
  unsigned long now = millis();

  // Undim immediately if a display function ran since the last check.
  if(displayDimmed && lastInteraction != prevLastInteraction)
    undimDisplay();
  prevLastInteraction = lastInteraction;

  // Param screens revert to the home screen after a short idle period. The
  // main settings menu gets 5s (user is reading a list); other transient
  // param screens get 3s. Clears transient UI state so the next encoder press
  // starts cleanly from the home screen.
  if(displayMode == DISPLAY_PARAM) {
    uint32_t revertMs = (pedal.menuState != MenuState::NONE ||
                         pedal.inActionSelect || pedal.inModeSelect ||
                         pedal.inFSEdit) ? 5000 : 3000;
    if((now - lastInteraction) > revertMs) {
      pedal.menuState                  = MenuState::NONE;
      pedal.inModeSelect               = false;
      pedal.inChannelSelect            = false;
      pedal.inActionSelect             = false;
      pedal.inFSEdit                   = false;
      pedal.fsEditEditing              = false;
      pedal.modeSelectFromActionSelect = false;
      displayHomeScreen(pedal);
    }
    return;
  }

  // Home screen: if FS info linger just expired, redraw to restore big preset/BPM view.
  if(g_homeFSIdx >= 0 && g_homeFSBtn != nullptr &&
     (now - g_homeFSMs) >= FS_INFO_LINGER_MS) {
    g_homeFSIdx = -1;
    g_homeFSBtn = nullptr;
    displayHomeScreen(pedal);
  }

  // Home screen: honour the user's "Display off" setting. "Always" → never blank.
  uint8_t tidx = pedal.globalSettings.displayTimeoutIdx;
  if(tidx >= NUM_DISP_TIMEOUTS) tidx = DISP_TIMEOUT_DEF_IDX;
  uint32_t timeoutMs = DISP_TIMEOUT_MS[tidx];
  if(timeoutMs == 0) return;
  if(!displayDimmed && (now - lastInteraction) > timeoutMs) {
    displayDimmed = true;
    display.ssd1306_command(0xAE); // SSD1306_DISPLAYOFF
  }
}

inline void displayUnlockProgress(uint8_t pct) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(24, 4);
  display.print(F("Hold to Unlock"));
  uint8_t barW = (uint8_t)((uint16_t)pct * 124 / 100);
  display.drawRect(2, 22, 124, 14, SSD1306_WHITE);
  if(barW > 0)
    display.fillRect(2, 22, barW, 14, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(46, 44);
  display.print(pct);
  display.print(F("%"));
  display.display();
}

// Shown while the user is editing a per-FS MIDI channel (hold encoder button with FS held).
inline void displayFSChannel(FSButton &btn) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(btn.name);
  display.setCursor(0, 10);
  display.print(F("MIDI Channel:"));

  display.setTextSize(3);
  display.setCursor(0, 24);
  if(btn.fsChannel == 0xFF) {
    display.print(F("GLB"));
  } else {
    display.print(btn.fsChannel + 1);
  }

  display.display();
}

inline void displayFootswitchPress(FSButton &button) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);

  // Row 1 — button name (small)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(button.name);

  // Row 2 — mode name (large if short)
  int y = _displayModeName(button.modMode.name, 10);

  // Row 3 — CC / PC / Note number + per-FS channel (right-aligned on same row)
  _displayNumber(button, y);
  if(button.fsChannel != 0xFF) {
    char chBuf[6];
    snprintf(chBuf, sizeof(chBuf), "CH:%d", button.fsChannel + 1);
    display.setCursor(128 - (int)strlen(chBuf) * 6, y);
    display.print(chBuf);
  }

  // Row 4 — state
  display.setCursor(0, y + 10);
  if(button.isKeyboard) {
    display.print(button.isActivated ? "PRESSED" : "OFF");
  } else if(button.isModSwitch) {
    display.print(button.isPressed ? "ACTIVE" : "OFF");
  } else if(button.isPC) {
    display.print(button.isPressed ? "SENT" : "");
  } else if(button.isNote) {
    display.print(button.isActivated ? "NOTE ON" : "NOTE OFF");
  } else if(button.isScene) {
    display.print(button.isPressed ? "SENT" : "");
  } else if(button.mode == FootswitchMode::Multi) {
    display.print(button.isPressed ? F("FIRED") : F(""));
  } else {
    display.print(button.isActivated ? "ON  127" : "OFF   0");
  }

  display.display();
}

// Shown when the user selects Multi category but no Multi scenes exist yet.
inline void displayNoMultisMessage(const char *fsName) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(fsName);
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);
  display.setCursor(0, 14);
  display.print(F("No Multis found."));
  display.setCursor(0, 28);
  display.print(F("Create via WebUI"));
  display.display();
}

// Encoder turned while holding a footswitch — show what value changed and to what.
// Big number = the focus; small header = context (FS name + mode).
inline void displayEncoderFSTurn(FSButton &button) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);

  // Row 1 — "FS 1  Ramp Inv" (context, small)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(button.name);
  display.print("  ");
  display.print(button.modMode.name);

  // Row 2 — label; Row 3 — value (large)
  display.setCursor(0, 12);
  display.setTextSize(1);

  if(button.mode == FootswitchMode::Multi) {
    display.print(F("Scene:"));
    display.setTextSize(2);
    display.setCursor(0, 24);
    uint8_t si = button.midiNumber;
    if(si < MAX_MULTI_SCENES && multiScenes[si].name[0] != '\0')
      display.print(multiScenes[si].name);
    else
      display.print(F("?"));
    display.display();
    return;
  }

  if(button.isKeyboard) {
    display.print("Key:");
    uint8_t kidx = button.midiNumber < NUM_HID_KEYS ? button.midiNumber : 0;
    const char *kname = hidKeys[kidx].name;
    bool big = strlen(kname) <= 6;
    display.setTextSize(big ? 3 : 2);
    display.setCursor(0, 24);
    display.print(kname);
    // Show modifier below
    uint8_t m = (button.fsChannel == 0xFF) ? 0 : button.fsChannel;
    if(m) {
      display.setTextSize(1);
      display.setCursor(0, 50);
      if(m & KEY_MOD_CTRL)  display.print("Ctrl ");
      if(m & KEY_MOD_SHIFT) display.print("Shift ");
      if(m & KEY_MOD_ALT)   display.print("Alt ");
      if(m & KEY_MOD_GUI)   display.print("Cmd");
    }
    display.display();
    return;
  }
  if(button.isSystem) {
    display.print("Cmd:");
    uint8_t idx = button.midiNumber < NUM_SYS_CMDS ? button.midiNumber : 0;
    const char *name = systemCommands[idx].name;
    bool big = strlen(name) <= 6;
    display.setTextSize(big ? 3 : 2);
    display.setCursor(0, 24);
    display.print(name);
  } else if(button.isNote) {
    // Note mode: show note name (C4, D#3 …) instead of raw number
    display.print("Note:");
    display.setTextSize(3);
    display.setCursor(0, 24);
    _printNoteName(button.midiNumber);
  } else {
    uint8_t displayVal;
    if(button.isScene) {
      if(button.isSceneScroll) {
        display.print("Max:");
        displayVal = button.midiNumber + 1;
      } else if(button.modMode.scenePickCC) {
        display.print("Slot CC:");
        displayVal = button.modMode.sceneCC + button.midiNumber + 1;
      } else {
        display.print("Scene:");
        displayVal = button.midiNumber + 1;
      }
    } else if(button.isPC) {
      display.print("PC:");
      displayVal = button.midiNumber + 1; // 1-indexed for humans
    } else if(button.isModSwitch && button.midiNumber == PB_SENTINEL) {
      // Pitch Bend destination — no numeric value; show "PB" as the big label.
      display.print(F("Target:"));
      display.setTextSize(3);
      display.setCursor(0, 24);
      display.print(F("PB"));
      display.display();
      return;
    } else {
      // CC or modulation — midiNumber is the CC number
      display.print("CC:");
      displayVal = button.midiNumber + 1;
    }
    display.setTextSize(3);
    display.setCursor(0, 24);
    display.print(displayVal);
  }

  display.display();
}

// Encoder turned with no footswitch held — MIDI channel selection.
inline void displayMidiChannel(uint8_t channel) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("MIDI Channel:");

  display.setTextSize(3);
  display.setCursor(0, 14);
  display.print(channel + 1); // display as 1–16

  display.display();
}

// Three-level mode selector.
// level 0 = category select
// level 1 = sub-group select (for Ramp/LFO/Stepper/Random) or variant select (CC/Scenes)
// level 2 = variant within sub-group (for categories with sub-groups)
//
// Inverted header bar shows FS name + depth indicator (e.g. "2/3").
// Action selector: pick which press type to configure for a footswitch.
// Collapsed (no extra actions enabled): [PRESS] [+EXPAND]
// Expanded (any extra action enabled):  [PRESS] [RELEASE] [HOLD] [DBL] [COLLAPSE]
inline void displayActionSelect(FSButton &btn, uint8_t slot) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  bool expanded = btn.extraActions[0].enabled || btn.extraActions[1].enabled || btn.extraActions[2].enabled;
  uint8_t slotCount = expanded ? 5 : 2;

  // Title + divider
  display.setCursor(0, 0);
  display.print(btn.name);
  display.print(F(" Edit"));
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  // Rows: cursor + label on left, mode name right-aligned
  for(uint8_t s = 0; s < slotCount; s++) {
    int y = 11 + s * 10;
    display.setCursor(0, y);
    display.print(s == slot ? '>' : ' ');

    const char *lbl;
    const char *rhs = nullptr;

    if(s == 0) {
      lbl = "PRESS";
      rhs = btn.modMode.name;
    } else if(!expanded && s == 1) {
      lbl = "+EXPAND";
    } else if(expanded && s == 4) {
      lbl = "COLLAPSE";
    } else {
      // s=1→REL(t=2), s=2→HOLD(t=0), s=3→DBL(t=1)
      static const char * const sLbls[] = { nullptr, "RELEASE", "HOLD", "DOUBLE" };
      uint8_t t = (s == 1) ? 2 : (s == 2) ? 0 : 1;
      lbl = sLbls[s];
      const FSAction &act = btn.extraActions[t];
      rhs = act.enabled ? (act.modeIndex < NUM_MODES ? modes[act.modeIndex].name : "???") : "---";
    }

    display.print(' ');
    display.print(lbl);

    if(rhs) {
      int x = 128 - (int)strlen(rhs) * 6;
      if(x > 42) { display.setCursor(x, y); display.print(rhs); }
    }
  }

  display.display();
}

inline void displayModeSelectScreen(const char *fsName, uint8_t catIdx,
                                    uint8_t level, uint8_t idx1, uint8_t idx2) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);

  const ModeCategory &cat = modeCategories[catIdx];
  bool isCCCat = (cat.subGroupCount == 0 && cat.count > 0 && cat.firstIdx < NUM_MODES &&
                  (modes[cat.firstIdx].mode == FootswitchMode::MomentaryCC ||
                   modes[cat.firstIdx].mode == FootswitchMode::LatchingCC));
  bool isMultiCat2 = (cat.firstIdx < NUM_MODES && modes[cat.firstIdx].mode == FootswitchMode::Multi);
  // baseDepth = selection steps before value entry
  uint8_t baseDepth = cat.autoSelect ? 1 : (isMultiCat2 ? 2 : (cat.subGroupCount > 0 ? 3 : (isCCCat ? (level >= 2 && idx2 == 2 ? 3 : 4) : 2)));
  bool hasValEntry = (!cat.autoSelect && !isMultiCat2);
  bool hasVel  = (level >= 4 && level < 6 && idx2 < NUM_MODES) ? modeNeedsVelocity(idx2)  : false;
  bool hasRamp = (level >= 6) ? true : ((level >= 4 && level < 6 && idx2 < NUM_MODES) ? modeNeedsRampEntry(idx2) : false);
  uint8_t totalDepth = baseDepth + (hasValEntry ? 1 : 0) + (hasVel ? 1 : 0) + (hasRamp ? 2 : 0);
  uint8_t stepNum;
  if(level < 4)       stepNum = level + 1;
  else if(level == 4) stepNum = baseDepth + 1;
  else if(level == 5) stepNum = baseDepth + 2;
  else if(level == 6) stepNum = baseDepth + 1 + (hasVel ? 1 : 0) + 1;
  else               stepNum = totalDepth; // level 7 = last step

  // ── Inverted header bar ────────────────────────────────────────────────────
  display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 1);
  display.print(fsName);
  // Depth indicator right-aligned
  char depth[5];
  snprintf(depth, sizeof(depth), "%d/%d", stepNum, totalDepth);
  display.setCursor(128 - (int)strlen(depth) * 6 - 2, 1);
  display.print(depth);
  display.setTextColor(SSD1306_WHITE);
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

  // ── Content based on level ─────────────────────────────────────────────────
  const char *label    = nullptr;
  const char *value    = nullptr;
  const char *noteLine = nullptr;

  if(level == 0) {
    label = "Category:";
    value = cat.name;
    if(!cat.autoSelect)
      noteLine = (cat.subGroupCount > 0) ? "4 steps" : (isCCCat ? "5 steps" : "3 steps");
  } else if(level == 1) {
    bool isMultiCat = (cat.firstIdx < NUM_MODES &&
                       modes[cat.firstIdx].mode == FootswitchMode::Multi);
    if(isMultiCat) {
      label = "Multi:";
      uint8_t si = idx1;
      value = (si < MAX_MULTI_SCENES && multiScenes[si].name[0] != '\0')
              ? multiScenes[si].name : "?";
    } else if(cat.subGroupCount > 0) {
      label    = (cat.subGroupCount == 3) ? "Wave:" : (cat.subGroupCount == 2) ? "Direction:" : "Unit:";
      value    = cat.subGroupNames ? cat.subGroupNames[idx1] : "?";
      noteLine = cat.subGroupNotes ? cat.subGroupNotes[idx1] : nullptr;
    } else {
      label = "Trigger:";
      value = cat.variantNames ? cat.variantNames[idx1] : "?";
    }
  } else if(level == 2 && cat.subGroupCount > 0) {
    // Sub-group variant select
    if(cat.subGroupNames && cat.subGroupNames[idx1]) {
      display.setTextSize(1);
      display.setCursor(0, 12);
      display.print(cat.subGroupNames[idx1]);
      display.print(F(" > Trigger:"));
    } else {
      label = "Trigger:";
    }
    value    = cat.variantNames ? cat.variantNames[idx2] : "?";
    noteLine = cat.subGroupNotes ? cat.subGroupNotes[idx1] : nullptr;
  } else if(level == 2) {
    // CC Hi / Single value
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 12);
    display.print(idx2 == 2 ? F("Value:") : F("Hi (press):"));
    display.setTextSize(3);
    display.setCursor(0, 26);
    display.print(idx1);
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Turn=value  Press=OK"));
    display.display();
    return;
  } else if(level == 3) {
    // Level 3: CC Lo value
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 12);
    display.print(F("Hi:"));
    display.print(idx1);
    display.print(F("  Lo (release):"));
    display.setTextSize(3);
    display.setCursor(0, 26);
    display.print(idx2);
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Turn=value  Press=OK"));
    display.display();
    return;
  } else if(level == 4) {
    // Level 4: primary value (CC#, PC#, Note#, scene, command, key, preset)
    // idx1 = current value (0-based), idx2 = modeSelectFinalIdx
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 12);
    if(idx2 < NUM_MODES) {
      const ModeInfo &mi = modes[idx2];
      if(mi.isSystem) {
        display.print(F("Command:"));
        display.setTextSize(2);
        display.setCursor(0, 26);
        uint8_t si = idx1 < NUM_SYS_CMDS ? idx1 : 0;
        display.print(systemCommands[si].name);
      } else if(mi.isKeyboard) {
        display.print(F("Key:"));
        display.setTextSize(2);
        display.setCursor(0, 26);
        uint8_t ki = idx1 < NUM_HID_KEYS ? idx1 : 0;
        display.print(hidKeys[ki].name);
      } else if(mi.isNote) {
        display.print(F("Note# (1-128):"));
        display.setTextSize(3);
        display.setCursor(0, 26);
        _printNoteName(idx1);
      } else if(mi.isPC) {
        display.print(F("PC# (1-128):"));
        display.setTextSize(3);
        display.setCursor(0, 26);
        display.print(idx1 + 1);
      } else if(mi.isScene) {
        display.print(F("Scene:"));
        display.setTextSize(3);
        display.setCursor(0, 26);
        display.print(idx1 + 1);
      } else if(mi.mode == FootswitchMode::PresetNum) {
        display.print(F("Preset# (1-N):"));
        display.setTextSize(3);
        display.setCursor(0, 26);
        display.print(idx1 + 1);
      } else if(mi.isModSwitch) {
        display.print(F("CC (PB=-1):"));
        display.setTextSize(3);
        display.setCursor(0, 26);
        if(idx1 == PB_SENTINEL) display.print(F("PB"));
        else display.print(idx1 + 1);
      } else {
        display.print(F("CC# (1-128):"));
        display.setTextSize(3);
        display.setCursor(0, 26);
        display.print(idx1 + 1);
      }
    } else {
      display.print(F("Value:"));
      display.setTextSize(3);
      display.setCursor(0, 26);
      display.print(idx1 + 1);
    }
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Turn=value  Press=OK"));
    display.display();
    return;
  } else if(level == 5) {
    // Level 5: Note velocity (1-127)
    // idx1 = current velocity
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 12);
    display.print(F("Velocity (1-127):"));
    display.setTextSize(3);
    display.setCursor(0, 26);
    display.print(idx1);
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Turn=value  Press=OK"));
    display.display();
    return;
  } else if(level == 6) {
    // Level 6: Up Speed — idx1 = speed table index
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 12);
    display.print(F("Up Speed:"));
    display.setTextSize(2);
    display.setCursor(0, 24);
    _displayRampSpeed(speedIdxToRampRaw(idx1));
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Turn=value  Press=OK"));
    display.display();
    return;
  } else if(level == 7) {
    // Level 7: Down Speed — idx1 = dn speed index, idx2 = up speed index (for reference)
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 12);
    display.print(F("Up: "));
    _displayRampSpeed(speedIdxToRampRaw(idx2));
    display.setCursor(0, 22);
    display.print(F("Dn Speed:"));
    display.setTextSize(2);
    display.setCursor(0, 32);
    _displayRampSpeed(speedIdxToRampRaw(idx1));
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Turn=value  Press=OK"));
    display.display();
    return;
  }

  if(label) {
    display.setTextSize(1);
    display.setCursor(0, 12);
    display.print(label);
  }

  if(value) {
    bool big = strlen(value) <= 10;
    display.setTextSize(big ? 2 : 1);
    display.setCursor(0, level == 2 ? 26 : 23);
    display.print(value);
  }

  if(noteLine) {
    display.setTextSize(1);
    display.setCursor(0, 44);
    display.print(noteLine);
  }

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print(cat.autoSelect ? F("Press=Confirm") : F("Turn=Select  Press=OK"));

  display.display();
}

inline void encoderButtonFSModeChange(FSButton &button) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);

  // Row 1 — button name (small)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(button.name);

  // Row 2 — new mode name (large if short)
  int y = _displayModeName(button.modMode.name, 10);

  // Row 3 — CC / PC / Note number
  _displayNumber(button, y);

  display.display();
}

inline void displayPotRampSpeed(String potName, uint32_t rampRaw) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(potName);
  display.println("");
  if(rampRaw & CLOCK_SYNC_FLAG) {
    uint8_t idx = rampRaw & 0xFF;
    if(idx >= NUM_NOTE_VALUES)
      idx = NUM_NOTE_VALUES - 1;
    display.print(noteValueNames[idx]);
  } else {
    float seconds = rampRaw / 1000.0f;
    display.print(seconds, 2);
    display.println(F("s"));
  }
  display.display();
}

inline void displayPotCC(String potName, uint8_t midiScaled) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(potName);
  display.println("");
  display.print("Value: ");
  display.println(midiScaled);
  display.display();
}

inline void displayPotValue(String potName, bool isMidiCC, uint8_t midiScaled, uint32_t rampRaw = 0) {
  if(isMidiCC)
    displayPotCC(potName, midiScaled);
  else
    displayPotRampSpeed(potName, rampRaw);
}

inline void displayTapTempo(float bpm) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Tap Tempo"));
  display.setTextSize(3);
  display.setCursor(0, 18);
  display.print((int)bpm);
  display.setTextSize(1);
  display.setCursor(0, 52);
  display.print(F("BPM"));
  display.display();
}

inline void displayExpCalibrating(uint8_t secsLeft) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Exp Calibrate"));
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);
  if(secsLeft == 0) {
    display.setTextSize(2);
    display.setCursor(20, 26);
    display.print(F("Done!"));
  } else {
    display.setCursor(0, 12);
    display.print(F("Sweep pedal full range"));
    display.setTextSize(3);
    display.setCursor(50, 28);
    display.print(secsLeft);
    display.setTextSize(1);
    display.setCursor(72, 36);
    display.print('s');
  }
  display.display();
}

inline void displayExpInput(uint8_t ccNum, uint8_t midiVal) {
  undimDisplay();
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Exp In"));
  String ccStr = String(F("CC ")) + String(ccNum + 1);
  display.setCursor(128 - (int)ccStr.length() * 6, 0);
  display.print(ccStr);
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(0, 18);
  display.print(midiVal);
  int barW = (int)(midiVal * 128L / 127);
  if(barW > 0) display.fillRect(0, 52, barW, 10, SSD1306_WHITE);
  display.drawRect(0, 52, 128, 10, SSD1306_WHITE);
  display.display();
}

inline void displayPresetLoad(uint8_t idx) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Preset"));
  display.setTextSize(3);
  display.setCursor(0, 14);
  display.print(idx + 1);
  display.setTextSize(1);
  display.setCursor(0, 52);
  display.print(F("Loaded"));
  display.display();
}

inline void displayPresetSaved(uint8_t idx) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(true);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("Preset"));
  display.setTextSize(3);
  display.setCursor(0, 14);
  display.print(idx + 1);
  display.setTextSize(1);
  display.setCursor(0, 52);
  display.print(F("SAVED"));
  display.display();
}

// ── FS Cursor Edit Menu ───────────────────────────────────────────────────────

enum class FSEditRow : uint8_t {
  CATEGORY = 0,
  CC_TRIGGER,   // Mom / Latch / Single
  RAMP_SHAPE,   // Exp / Lin / Sine
  MOD_DIR,      // Normal / Inverted
  MOD_TRIG,     // Momentary / Latching
  LFO_WAVE,     // Sine / Tri / Square
  SCENE_UNIT,   // Helix / QC / Fractal / Kemper
  SCENE_STYLE,  // Snap / Scroll
  PRESET_DIR,   // Up / Down / Num
  NUMBER,       // CC# / PC# / Note# / Scene# / Key / Cmd / Preset# / Multi
  CC_HI,        // Hi value (CC Mom/Latch)
  CC_LO,        // Lo value (CC Mom/Latch)
  CC_VAL,       // Value (CC Single)
  VELOCITY,     // Note velocity
  KB_MOD,       // Keyboard modifier
  RAMP_UP,      // Up speed
  RAMP_DN,      // Down speed
  CHANNEL,      // Per-FS MIDI channel
};
static constexpr uint8_t MAX_FS_EDIT_ROWS = 18;

inline uint8_t buildFSEditRows(const FSButton &btn, FSEditRow *rows) {
  uint8_t n = 0;
  rows[n++] = FSEditRow::CATEGORY;
  uint8_t mi = btn.modeIndex;
  if(mi == 1) {
    rows[n++] = FSEditRow::NUMBER;
  } else if(mi >= 2 && mi <= 4) {
    rows[n++] = FSEditRow::CC_TRIGGER;
    rows[n++] = FSEditRow::NUMBER;
    if(mi == 4) rows[n++] = FSEditRow::CC_VAL;
    else { rows[n++] = FSEditRow::CC_HI; rows[n++] = FSEditRow::CC_LO; }
  } else if(mi == 5) {
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::VELOCITY;
  } else if(mi >= 6 && mi <= 17) {
    rows[n++] = FSEditRow::RAMP_SHAPE;
    rows[n++] = FSEditRow::MOD_DIR;
    rows[n++] = FSEditRow::MOD_TRIG;
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::RAMP_UP;
    rows[n++] = FSEditRow::RAMP_DN;
  } else if(mi >= 18 && mi <= 23) {
    rows[n++] = FSEditRow::LFO_WAVE;
    rows[n++] = FSEditRow::MOD_TRIG;
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::RAMP_UP;
    rows[n++] = FSEditRow::RAMP_DN;
  } else if(mi >= 24 && mi <= 27) {
    rows[n++] = FSEditRow::MOD_DIR;
    rows[n++] = FSEditRow::MOD_TRIG;
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::RAMP_UP;
    rows[n++] = FSEditRow::RAMP_DN;
  } else if(mi >= 28 && mi <= 31) {
    rows[n++] = FSEditRow::MOD_DIR;
    rows[n++] = FSEditRow::MOD_TRIG;
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::RAMP_UP;
    rows[n++] = FSEditRow::RAMP_DN;
  } else if(mi >= 32 && mi <= 39) {
    rows[n++] = FSEditRow::SCENE_UNIT;
    rows[n++] = FSEditRow::SCENE_STYLE;
    rows[n++] = FSEditRow::NUMBER;
  } else if(mi == 41) {
    rows[n++] = FSEditRow::NUMBER;
  } else if(mi == 42) {
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::KB_MOD;
    return n; // fsChannel stores modifier — skip CHANNEL
  } else if(mi == 43) {
    rows[n++] = FSEditRow::NUMBER;
  } else if(mi >= 44 && mi <= 46) {
    rows[n++] = FSEditRow::PRESET_DIR;
    if(mi == 46) rows[n++] = FSEditRow::NUMBER;
  }
  rows[n++] = FSEditRow::CHANNEL;
  return n;
}

inline const char* fsEditRowName(FSEditRow r, const FSButton &btn) {
  switch(r) {
    case FSEditRow::CATEGORY:    return "Mode";
    case FSEditRow::CC_TRIGGER:  return "Type";
    case FSEditRow::RAMP_SHAPE:  return "Shape";
    case FSEditRow::MOD_DIR:     return "Dir";
    case FSEditRow::MOD_TRIG:    return "Trig";
    case FSEditRow::LFO_WAVE:    return "Wave";
    case FSEditRow::SCENE_UNIT:  return "Unit";
    case FSEditRow::SCENE_STYLE: return "Style";
    case FSEditRow::PRESET_DIR:  return "Dir";
    case FSEditRow::NUMBER:
      if(btn.isNote)      return "Note#";
      if(btn.isPC)        return "PC#";
      if(btn.isScene)     return btn.isSceneScroll ? "Max" : "Scene#";
      if(btn.isKeyboard)  return "Key";
      if(btn.isSystem)    return "Cmd";
      if(btn.mode == FootswitchMode::PresetNum) return "Preset#";
      if(btn.mode == FootswitchMode::Multi)     return "Multi";
      return "CC#";
    case FSEditRow::CC_HI:    return "Hi";
    case FSEditRow::CC_LO:    return "Lo";
    case FSEditRow::CC_VAL:   return "Value";
    case FSEditRow::VELOCITY: return "Vel";
    case FSEditRow::KB_MOD:   return "Mod";
    case FSEditRow::RAMP_UP:  return "Up Spd";
    case FSEditRow::RAMP_DN:  return "Dn Spd";
    case FSEditRow::CHANNEL:  return "Ch";
    default: return "?";
  }
}

inline String fsEditRowValue(FSEditRow r, const FSButton &btn) {
  uint8_t mi = btn.modeIndex;
  switch(r) {
    case FSEditRow::CATEGORY: {
      uint8_t ci = categoryForModeIndex(mi);
      return String(modeCategories[ci].name);
    }
    case FSEditRow::CC_TRIGGER: {
      static const char *ts[] = {"Mom","Latch","Single"};
      uint8_t v = (mi >= 2 && mi <= 4) ? (mi - 2) : 0;
      return String(ts[v]);
    }
    case FSEditRow::RAMP_SHAPE: {
      static const char *ss[] = {"Exp","Lin","Sine"};
      uint8_t s = (mi >= 6 && mi <= 17) ? (mi-6)/4 : 0;
      return String(ss[s < 3 ? s : 0]);
    }
    case FSEditRow::MOD_DIR: {
      uint8_t d;
      if(mi >= 6  && mi <= 17) d = ((mi-6) /2)%2;
      else if(mi >= 24 && mi <= 27) d = ((mi-24)/2)%2;
      else                          d = ((mi-28)/2)%2;
      return String(d ? "Inv" : "Norm");
    }
    case FSEditRow::MOD_TRIG: {
      uint8_t t;
      if(mi >= 6  && mi <= 17) t = (mi-6) %2;
      else if(mi >= 18 && mi <= 23) t = (mi-18)%2;
      else if(mi >= 24 && mi <= 27) t = (mi-24)%2;
      else                          t = (mi-28)%2;
      return String(t ? "Latch" : "Mom");
    }
    case FSEditRow::LFO_WAVE: {
      static const char *ws[] = {"Sine","Tri","Sq"};
      uint8_t w = (mi >= 18 && mi <= 23) ? (mi-18)/2 : 0;
      return String(ws[w < 3 ? w : 0]);
    }
    case FSEditRow::SCENE_UNIT: {
      static const char *us[] = {"Helix","QC","Fractal","Kemper"};
      uint8_t u = (mi >= 32 && mi <= 39) ? (mi-32)/2 : 0;
      return String(us[u < 4 ? u : 0]);
    }
    case FSEditRow::SCENE_STYLE:
      return String((mi >= 32 && mi <= 39) && (mi-32)%2 ? "Scroll" : "Snap");
    case FSEditRow::PRESET_DIR: {
      static const char *pd[] = {"Up","Down","#"};
      uint8_t d = (mi >= 44 && mi <= 46) ? (mi-44) : 0;
      return String(pd[d < 3 ? d : 0]);
    }
    case FSEditRow::NUMBER: {
      if(btn.isModSwitch && btn.midiNumber == PB_SENTINEL) return String(F("PB"));
      if(btn.isNote) {
        static const char *nn[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        return String(nn[btn.midiNumber%12]) + String((int)(btn.midiNumber/12)-1);
      }
      if(btn.isPC)    return String(F("PC:"))  + String(btn.midiNumber+1);
      if(btn.isScene) return String(F("V:"))   + String(btn.midiNumber+1);
      if(btn.isKeyboard) {
        uint8_t ki = btn.midiNumber < NUM_HID_KEYS ? btn.midiNumber : 0;
        return String(hidKeys[ki].name);
      }
      if(btn.isSystem) {
        uint8_t si = btn.midiNumber < NUM_SYS_CMDS ? btn.midiNumber : 0;
        return String(systemCommands[si].name);
      }
      if(btn.mode == FootswitchMode::PresetNum) return String(btn.midiNumber+1);
      if(btn.mode == FootswitchMode::Multi) {
        uint8_t si = btn.midiNumber;
        return (si < MAX_MULTI_SCENES && multiScenes[si].name[0] != '\0')
          ? String(multiScenes[si].name) : String(F("?"));
      }
      return String(F("CC:")) + String(btn.midiNumber+1);
    }
    case FSEditRow::CC_HI:    return String(btn.ccHigh);
    case FSEditRow::CC_LO:    return String(btn.ccLow);
    case FSEditRow::CC_VAL:   return String(btn.ccHigh);
    case FSEditRow::VELOCITY: return String(btn.velocity);
    case FSEditRow::KB_MOD: {
      uint8_t m = (btn.fsChannel == 0xFF) ? 0 : btn.fsChannel;
      if(!m) return String(F("None"));
      String s;
      if(m & KEY_MOD_CTRL)  s += 'C';
      if(m & KEY_MOD_SHIFT) s += 'S';
      if(m & KEY_MOD_ALT)   s += 'A';
      if(m & KEY_MOD_GUI)   s += 'G';
      return s;
    }
    case FSEditRow::RAMP_UP:
    case FSEditRow::RAMP_DN: {
      uint32_t raw = (r == FSEditRow::RAMP_UP) ? btn.rampUpMs : btn.rampDownMs;
      if(raw & CLOCK_SYNC_FLAG) {
        uint8_t ni = (uint8_t)(raw & 0xFF);
        if(ni >= NUM_NOTE_VALUES) ni = NUM_NOTE_VALUES-1;
        return String(noteValueNames[ni]);
      }
      char buf[10];
      snprintf(buf, sizeof(buf), "%.2fs", raw/1000.0f);
      return String(buf);
    }
    case FSEditRow::CHANNEL:
      return (btn.fsChannel == 0xFF) ? String(F("Glb")) : String(btn.fsChannel+1);
    default: return String();
  }
}

// Sync loadActionEditBtn back to extra action or persistent load action.
// No-op for PRESS edits (fsEditExtraType < 0 and fsEditFSIdx < FS_LOAD_ACTION_IDX).
inline void _syncFSEditTarget(PedalState &pedal) {
  const FSButton &lb = pedal.loadActionEditBtn;
  if(pedal.fsEditExtraType >= 0 && pedal.fsEditFSIdx < FS_LOAD_ACTION_IDX) {
    FSAction &act = pedal.buttons[pedal.fsEditFSIdx].extraActions[(uint8_t)pedal.fsEditExtraType];
    act.modeIndex  = lb.modeIndex;  act.midiNumber = lb.midiNumber;
    act.fsChannel  = lb.fsChannel;  act.ccLow      = lb.ccLow;
    act.ccHigh     = lb.ccHigh;     act.velocity   = lb.velocity;
    act.rampUpMs   = lb.rampUpMs;   act.rampDownMs = lb.rampDownMs;
    act.enabled    = (lb.modeIndex != 0);
  } else if(pedal.fsEditFSIdx == FS_LOAD_ACTION_IDX) {
    FSActionPersisted &la = presets[activePreset].loadAction;
    la.enabled    = (lb.modeIndex != 0);  la.modeIndex  = lb.modeIndex;
    la.midiNumber = lb.midiNumber;        la.fsChannel  = lb.fsChannel;
    la.ccLow      = lb.ccLow;            la.ccHigh     = lb.ccHigh;
    la.velocity   = lb.velocity;         la.rampUpMs   = lb.rampUpMs;
    la.rampDownMs = lb.rampDownMs;       la._pad       = 0;
  }
}

inline void displayFSEditMenu(PedalState &pedal) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();

  const FSButton &btn = pedal.fsEditTarget();
  FSEditRow rows[MAX_FS_EDIT_ROWS];
  uint8_t rowCount = buildFSEditRows(btn, rows);
  if(pedal.fsEditCursor >= rowCount) pedal.fsEditCursor = rowCount - 1;

  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Title bar
  display.setCursor(0, 0);
  uint8_t fi = pedal.fsEditFSIdx;
  if(fi == FS_LOAD_ACTION_IDX) {
    display.print(F("Load Action"));
  } else {
    display.print(pedal.buttons[fi].name);
    if(pedal.fsEditExtraType == 0)      display.print(F(" Hold"));
    else if(pedal.fsEditExtraType == 1) display.print(F(" Dbl"));
    else if(pedal.fsEditExtraType == 2) display.print(F(" Rel"));
  }
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  // Scrolling window: show 5 rows
  int8_t start = (int8_t)pedal.fsEditCursor - 2;
  if(start < 0) start = 0;
  if(start > (int8_t)rowCount - 5) start = (rowCount >= 5) ? (int8_t)(rowCount - 5) : 0;

  for(uint8_t i = 0; i < 5; i++) {
    uint8_t ri = (uint8_t)start + i;
    if(ri >= rowCount) break;
    FSEditRow row = rows[ri];
    bool isSel  = (ri == pedal.fsEditCursor);
    bool isEdit = isSel && pedal.fsEditEditing;
    int y = 11 + (int)i * 10;

    display.setCursor(0, y);
    display.print(isSel ? '>' : ' ');
    display.print(fsEditRowName(row, btn));

    String val = fsEditRowValue(row, btn);
    if(val.length() == 0) continue;
    if(val.length() > 7) val = val.substring(0, 7);
    int vx = 128 - (int)val.length() * 6;

    if(isEdit) {
      // Highlight value being edited
      display.fillRect(vx - 1, y - 1, (int)val.length() * 6 + 2, 9, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(vx, y);
      display.print(val);
      display.setTextColor(SSD1306_WHITE);
    } else {
      display.setCursor(vx, y);
      display.print(val);
    }
  }

  display.setCursor(0, 56);
  display.print(pedal.fsEditEditing ? F("Turn=val  Press=OK") : F("Turn=sel  Press=edit"));
  display.display();
}
