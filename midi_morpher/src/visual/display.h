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
  if(btn.isNote) {
    static const char *noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
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

  uint8_t tidx = pedal.globalSettings.displayTimeoutIdx;
  if(tidx >= NUM_DISP_TIMEOUTS) tidx = DISP_TIMEOUT_DEF_IDX;
  uint32_t timeoutMs = DISP_TIMEOUT_MS[tidx];
  if(timeoutMs == 0) {
    if(displayDimmed) undimDisplay();
    return;
  }

  if(!displayDimmed && (now - lastInteraction) > timeoutMs) {
    displayDimmed = true;
    display.ssd1306_command(0xAE); // SSD1306_DISPLAYOFF — screen fully off
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
  if(button.isModSwitch) {
    display.print(button.isPressed ? "ACTIVE" : "OFF");
  } else if(button.isPC) {
    display.print(button.isPressed ? "SENT" : "");
  } else if(button.isNote) {
    display.print(button.isActivated ? "NOTE ON" : "NOTE OFF");
  } else if(button.isScene) {
    display.print(button.isPressed ? "SENT" : "");
  } else {
    // CC momentary or latching
    display.print(button.isActivated ? "ON  127" : "OFF   0");
  }

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

  if(button.isSystem) {
    // System/Transport: show the selected command name as big text
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

// Two-level mode selector: category (level 0) or variant within category (level 1).
inline void displayModeSelectScreen(const char *fsName, uint8_t catIdx, uint8_t varIdx, bool variantLevel) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(fsName);

  if(!variantLevel) {
    // ── Category select ──────────────────────────────────────────────────────
    display.setCursor(0, 14);
    display.print(F("Category:"));

    const char *catName = modeCategories[catIdx].name;
    display.setTextSize(2);
    display.setCursor(0, 30);
    display.print(catName);

    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Scroll / Press=OK"));
  } else {
    // ── Variant select ───────────────────────────────────────────────────────
    display.print(F("  "));
    display.print(modeCategories[catIdx].name);

    display.setCursor(0, 14);
    display.print(F("Mode:"));

    uint8_t modeIdx = modeCategories[catIdx].firstIdx + varIdx;
    const char *modeName = modes[modeIdx].name;
    bool big = strlen(modeName) <= 10;
    display.setTextSize(big ? 2 : 1);
    display.setCursor(0, 30);
    display.print(modeName);

    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Scroll / Press=Set"));
  }

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
