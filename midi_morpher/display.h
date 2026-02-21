// display.h - OLED display functions

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "settingsState.h"

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
}

void displayHomeScreen(MIDIMorpherSettingsState &settings)
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
  display.println(String(settings.midiChannel));
  display.println(settings.rampLinearCurve ? "Linear Ramp" : "Exponential Ramp");
  display.println(settings.rampDirectionUp ? "Ramp Direction UP" : "Ramp Direction DOWN");
  display.println(settings.hotSwitchLatching ? "HotSwitch Latching" : "HotSwitch Momentary");
  display.println(settings.settingsLocked ? "LOCKED" : "");
  display.display();
}

void resetDisplayTimeout(MIDIMorpherSettingsState &settings)
{
  unsigned long now = millis();

  if (displayMode == DISPLAY_PARAM &&
      now - lastInteraction > displayTimeout)
  {

    displayMode = DISPLAY_DEFAULT;
    displayHomeScreen(settings);
  }
}

//
void displayToggleChange(String toggleName, bool state, bool rampDirection = false)
{
  return;
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();

  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(toggleName);
  display.println();
  if (rampDirection)
  {
    display.println(state ? "UP" : "DOWN");
  }
  else
  {
    display.println(state ? "ON" : "OFF");
  }
  display.display();
}

void displayFootswitchPress(String fsName, String pcOrCc = "PC", uint8_t channel = 1, uint8_t midiValue = 0)
{
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(fsName);
  display.print(pcOrCc);
  display.println(String(midiValue + 1));
  display.println("Channel:" + String(channel));
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

void showSignal(String deviceName, long numSignal = 0, String strSignal = "")
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(deviceName);
  display.println(numSignal);
  display.println(strSignal);
  display.display();
}
#endif
