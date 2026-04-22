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

inline void displayHomeScreen(PedalState &pedal) {
  displayMode = DISPLAY_DEFAULT;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // ── Status bar ─────────────────────────────────────────────────────────────
  // Layout (128px @ 6px/char): Ch:NN  P:N*  BPM[E]  LOCK
  display.setCursor(0, 0);
  display.print(F("Ch:"));
  display.print(pedal.midiChannel + 1);

  display.setCursor(32, 0);
  display.print(F("P:"));
  display.print(activePreset + 1);
  if(presetDirty)
    display.print('*');

  display.setCursor(62, 0);
  display.print((int)midiClock.bpm);
  if(midiClock.externalSync)
    display.print(F("E"));

  if(pedal.settingsLocked) {
    display.setCursor(104, 0);
    display.print(F("LOCK"));
  }

  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  // ── Footswitch rows ────────────────────────────────────────────────────────
  for(int i = 0; i < DISPLAY_FS_ROWS; i++) {
    const FSButton &btn = pedal.buttons[i];
    int y = 12 + i * 13;

    display.setCursor(0, y);
    display.print(i + 1);
    display.print(' ');

    // Mode name truncated to 8 chars to leave room for the optional channel indicator
    const char *name = btn.modMode.name;
    for(uint8_t c = 0; c < 8 && name[c] != '\0'; c++)
      display.print(name[c]);

    // Parameter right-aligned to display edge
    String numStr = _buttonNumStr(btn);
    int numX = 128 - (int)numStr.length() * 6;

    // Per-FS channel override: show channel number just left of the parameter
    if(btn.fsChannel != 0xFF) {
      String chStr = String(btn.fsChannel + 1);
      display.setCursor(numX - (int)chStr.length() * 6 - 6, y);
      display.print(chStr);
    }

    display.setCursor(numX, y);
    display.print(numStr);
  }

  display.display();
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
    uint32_t revertMs = (pedal.menuState != MenuState::NONE) ? 5000 : 3000;
    if((now - lastInteraction) > revertMs) {
      pedal.menuState       = MenuState::NONE;
      pedal.inModeSelect    = false;
      pedal.inChannelSelect = false;
      displayHomeScreen(pedal);
    }
    return;
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

// Prints a MIDI note number as a note name+octave, e.g. 60 → "C4", 61 → "C#4".
// Middle C = 60 = C4.
inline static void _printNoteName(uint8_t note) {
  static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  display.print(names[note % 12]);
  display.print((int)(note / 12) - 1);
}

// Helper used by both press and mode-change displays.
// Prints the mode name at size 2 if it fits in 128px, else size 1.
// Returns the y position of the next available row after the name.
inline static int _displayModeName(const char *modeName, int y) {
  bool big = strlen(modeName) <= 10;
  display.setTextSize(big ? 2 : 1);
  display.setCursor(0, y);
  display.print(modeName);
  return y + (big ? 18 : 10);
}

// Prints "CC: X", "PC: X", "Note: X" or scene info on one row.
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
  } else {
    display.print("CC: ");
    display.print(button.midiNumber + 1);
  }
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
  uint8_t totalDepth = cat.autoSelect ? 1 : (isMultiCat2 ? 2 : (cat.subGroupCount > 0 ? 3 : (isCCCat ? 4 : 2)));

  // ── Inverted header bar ────────────────────────────────────────────────────
  display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 1);
  display.print(fsName);
  // Depth indicator right-aligned
  char depth[5];
  snprintf(depth, sizeof(depth), "%d/%d", level + 1, totalDepth);
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
      noteLine = (cat.subGroupCount > 0) ? "3 steps" : (isCCCat ? "4 steps" : "2 steps");
  } else if(level == 1) {
    bool isMultiCat = (cat.firstIdx < NUM_MODES &&
                       modes[cat.firstIdx].mode == FootswitchMode::Multi);
    if(isMultiCat) {
      label = "Multi:";
      uint8_t si = idx1;
      value = (si < MAX_MULTI_SCENES && multiScenes[si].name[0] != '\0')
              ? multiScenes[si].name : "?";
    } else if(cat.subGroupCount > 0) {
      label    = (cat.subGroupCount == 3) ? "Wave:" : "Direction:";
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
    // CC Hi value
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 12);
    display.print(F("Hi (press):"));
    display.setTextSize(3);
    display.setCursor(0, 26);
    display.print(idx1);
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Turn=value  Press=OK"));
    display.display();
    return;
  } else {
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
