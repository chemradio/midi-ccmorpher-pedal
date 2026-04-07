// display.h - OLED display functions
#pragma once
#include "../config.h"
#include "../footswitches/footswitchObject.h"
#include "../pedalState.h"
#include "../statePersistance.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// Display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum DisplayMode {
  DISPLAY_DEFAULT,
  DISPLAY_PARAM
};

// Temporary display text
String tempDisplayText = "";
unsigned long lastInteraction = 0;
const unsigned long displayTimeout = 2000;
DisplayMode displayMode = DISPLAY_DEFAULT;

bool initDisplay() {
  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    return false;
  }
  display.clearDisplay();
  display.dim(true);
  display.display();
  return true;
}

void showStartupScreen() {
  display.dim(false);
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
void displayLockedMessage(String whoSays = "") {
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

void displayHomeScreen(PedalState &pedal) {
  displayMode = DISPLAY_DEFAULT;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MIDI Morpher");
  display.setTextSize(1);
  display.print("Channel:");
  display.println(String(pedal.midiChannel + 1));
  display.println();
  display.print("Pots: ");
  display.println(pedal.getPotMode());
  display.println(pedal.modulator.latching ? "HotSwitch Latching" : "HotSwitch Momentary");
  display.println(pedal.settingsLocked ? "LOCKED" : "");
  display.display();
}

void resetDisplayTimeout(PedalState &pedal) {
  unsigned long now = millis();

  if(displayMode == DISPLAY_PARAM &&
     now - lastInteraction > displayTimeout) {
    lastInteraction = now;
    displayMode = DISPLAY_DEFAULT;
    displayHomeScreen(pedal);
  }
}

// Prints a MIDI note number as a note name+octave, e.g. 60 → "C4", 61 → "C#4".
// Middle C = 60 = C4.
static void _printNoteName(uint8_t note) {
  static const char *names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
  display.print(names[note % 12]);
  display.print((int)(note / 12) - 1);
}

// Helper used by both press and mode-change displays.
// Prints the mode name at size 2 if it fits in 128px, else size 1.
// Returns the y position of the next available row after the name.
static int _displayModeName(const char *modeName, int y) {
  bool big = strlen(modeName) <= 10;
  display.setTextSize(big ? 2 : 1);
  display.setCursor(0, y);
  display.print(modeName);
  return y + (big ? 18 : 10);
}

// Prints "CC: X", "PC: X", "Note: X" or scene info on one row.
static void _displayNumber(const FSButton &button, int y) {
  display.setTextSize(1);
  display.setCursor(0, y);
  if(button.isScene) {
    if(button.modMode.scenePickCC) {
      display.print("CC: ");
      display.print(button.modMode.sceneCC + button.midiNumber);
    } else {
      display.print("CC:");
      display.print(button.modMode.sceneCC);
      display.print(" Val:");
      display.print(button.midiNumber);
    }
  } else if(button.isPC) {
    display.print("PC: ");
    display.print(button.midiNumber + 1);
  } else if(button.isNote) {
    display.print("Note: ");
    display.print(button.midiNumber);
  } else {
    display.print("CC: ");
    display.print(button.midiNumber + 1);
  }
}

void displayFootswitchPress(FSButton &button) {
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

  // Row 3 — CC / PC / Note number
  _displayNumber(button, y);

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
void displayEncoderFSTurn(FSButton &button) {
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

  if(button.isNote) {
    // Note mode: show note name (C4, D#3 …) instead of raw number
    display.print("Note:");
    display.setTextSize(3);
    display.setCursor(0, 24);
    _printNoteName(button.midiNumber);
  } else {
    uint8_t displayVal;
    if(button.isScene) {
      if(button.modMode.scenePickCC) {
        display.print("Slot CC:");
        displayVal = button.modMode.sceneCC + button.midiNumber;
      } else {
        display.print("Scene:");
        displayVal = button.midiNumber; // 0-indexed, matches device expectation
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
void displayMidiChannel(uint8_t channel) {
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

void encoderButtonFSModeChange(FSButton &button) {
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

void displayPotRampSpeed(String potName, long rampMs) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(potName);
  display.println("");
  float seconds = rampMs / 1000.0f;
  display.print(seconds, 2);
  display.println("s");
  display.display();
}

void displayPotCC(String potName, uint8_t midiScaled) {
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

void displayPotValue(String potName, bool isMidiCC, uint8_t midiScaled, long rampMs = 0) {
  if(isMidiCC)
    displayPotCC(potName, midiScaled);
  else
    displayPotRampSpeed(potName, rampMs);
}

// //
// void displayToggleChange(String toggleName, bool state, bool rampDirection = false)
// {
//   return;
//   displayMode = DISPLAY_PARAM;
//   lastInteraction = millis();

//   display.clearDisplay();

//   display.setTextSize(2);
//   display.setCursor(0, 0);
//   display.println(toggleName);
//   display.println();
//   if (rampDirection)
//   {
//     display.println(state ? "UP" : "DOWN");
//   }
//   else
//   {
//     display.println(state ? "ON" : "OFF");
//   }
//   display.display();
// }

// void showSignal(String deviceName, long numSignal = 0, String strSignal = "")
// {
//   display.clearDisplay();
//   display.setTextSize(2);
//   display.setCursor(0, 0);
//   display.println(deviceName);
//   display.println(numSignal);
//   display.println(strSignal);
//   display.display();
// }
