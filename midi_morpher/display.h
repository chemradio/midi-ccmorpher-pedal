// display.h - OLED display functions
#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "pedalState.h"
#include "footswitchObject.h"

// Display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum DisplayMode
{
  DISPLAY_DEFAULT,
  DISPLAY_PARAM
};

// Temporary display text
String tempDisplayText = "";
unsigned long lastInteraction = 0;
const unsigned long displayTimeout = 2000;
DisplayMode displayMode = DISPLAY_DEFAULT;

bool initDisplay()
{
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    return false;
  }
  display.clearDisplay();
  display.display();
  return true;
}

void showStartupScreen()
{
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
void displayLockedMessage()
{
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(F("Settings"));
  display.println(F("LOCKED"));
  display.display();
  displayMode = DISPLAY_PARAM;
}

void displayHomeScreen(PedalState &pedal)
{
  displayMode = DISPLAY_DEFAULT;
  lastInteraction = millis();
  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MIDI Morpher");
  display.println();
  display.setTextSize(1);
  display.print("MIDI CH:");
  display.println(String(pedal.midiChannel + 1));
  display.println(pedal.rampLinearCurve ? "Linear Ramp" : "Exponential Ramp");
  display.println(pedal.rampDirectionUp ? "Ramp Direction UP" : "Ramp Direction DOWN");
  display.println(pedal.hotSwitchLatching ? "HotSwitch Latching" : "HotSwitch Momentary");
  display.println(pedal.settingsLocked ? "LOCKED" : "");
  display.display();
}

void resetDisplayTimeout(PedalState &pedal)
{
  unsigned long now = millis();

  if (displayMode == DISPLAY_PARAM &&
      now - lastInteraction > displayTimeout)
  {
    lastInteraction = now;
    displayMode = DISPLAY_DEFAULT;
    displayHomeScreen(pedal);
  }
}

void displayFootswitchPress(FSButton &button)
{
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);

  display.println(button.name);
  display.print(button.isPC ? "PC: " : "CC: ");
  display.println(String(button.midiNumber + 1));
  if (!button.isPC)
  {
    display.print("Value: ");
    display.println(String(button.isActivated ? 127 : 0));
  }
  display.display();
}

void displayEncoderTurn(String fsName, bool isPC, uint8_t value)
{
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(fsName);
  display.print(isPC ? "PC: " : "CC: ");
  display.println(String(value + 1));
  display.display();
}

void encoderButtonFSModeChange(String fsName, String newMode, bool isPC, uint8_t midiNumber)
{
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(fsName);
  display.print("New Mode: ");
  display.println(newMode);
  //
  display.print("MIDI Num: ");
  display.println(String(midiNumber + 1));
  display.display();
}

void displayPotValue(String potName, uint16_t potValue, uint8_t scaledValue)
{
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(potName);
  display.print("Raw: ");
  display.println(potValue);
  display.print("Scaled: ");
  display.println(scaledValue);
  display.display();
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
