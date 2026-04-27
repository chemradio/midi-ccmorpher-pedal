#include "src/analogInOut/expInput.h"
#include "src/ble/bleMidi.h"
#include "src/clock/clockSync.h"
#include "src/clock/midiClock.h"
#include "src/config.h"
#include "src/controls/encoder.h"
#include "src/controls/encoderButton.h"
#include "src/controls/pots.h"
#include "src/hidKeyboard.h"
#include "src/menu/mainMenu.h"
#include "src/midi/midiRouter.h"
#include "src/midiOut.h"
#include "src/pedalState.h"
#include "src/presets.h"
#include "src/statePersistance.h"
#include "src/visual/display.h"
#include "src/visual/neopx.h"
#include "src/wifi/webServer.h"
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBMIDI.h>

// initialize global state
USBMIDI midi;
USBHIDKeyboard hidKeyboard;
PedalState pedal;

// Trampoline declared extern in bleMidlmi.h — defined here so bleMidi.h doesn't
// need to include midiClock.h (which would create a cycle via midiOut.h).
void bleMidiOnClockTick() { midiClock.receiveClock(); }

void setup() {
  Serial1.begin(31250, SERIAL_8N1, -1, MIDI_TX);
  Serial2.begin(31250, SERIAL_8N1, MIDI_RX, -1);
  USB.VID(0x303A);
  USB.PID(0x8000);
  USB.productName("MIDI Morpher 1.0");
  USB.manufacturerName("Tim");
  USB.firmwareVersion(0x0100);
  hidKeyboard.begin();
  midi.begin();
  USB.begin();
  delay(1000);

  if(!initDisplay()) {
    while(1)
      delay(1000);
  }

  initExpInput();
  pedal.initButtons();
  initEncoder();
  initEncoderButton();
  analogReadResolution(12);
  initAnalogPots();

  // Preset button and activity LED
  pinMode(PRESET_BTN_PIN, INPUT_PULLUP);
  pinMode(ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(ACTIVITY_LED_PIN, LOW);
  pinMode(TEMPO_LED_PIN, OUTPUT);
  digitalWrite(TEMPO_LED_PIN, LOW);

  // Load presets + global settings, then apply global settings to live state
  SPIFFS.begin(true);
  loadAllPresets(pedal);
  loadGlobalSettings(pedal);
  loadMultiScenes();
  loadExpCalibration(pedal);
  applyGlobalSettings(pedal);

  initWebServer(pedal);
  initBleMidi();
  initNeoPixel();
  showStartupScreen();
  delay(2000);

  // Show the home screen immediately after startup
  displayHomeScreen(pedal);
  updatePresetLEDs(pedal);
}

void loop() {
  midiClock.ledEnabled = pedal.globalSettings.tempoLedEnabled;
  midiClock.clockGenerate = pedal.globalSettings.clockGenerate;
  midiClock.clockOutput = pedal.globalSettings.clockOutput;
  midiClock.tick();
  for(auto &mod : pedal.modulators)
    mod.update();

  // Sync display state before any input handlers run so encoder/button logic
  // always sees consistent state (inModeSelect cleared when display reverts).
  resetDisplayTimeout(pedal);

  // Process footswitches, tracking which was last pressed for the activity LED.
  for(int i = 0; i < (int)pedal.buttons.size(); i++) {
    bool wasPressedBefore = pedal.buttons[i].isPressed;
    bool prevNeedsDraw = g_homeFSNeedsDraw;

    pedal.buttons[i].handleFootswitch(pedal.midiChannel, pedal.modForFS(i), displayFSUpdateHome);

    // If the callback fired for this button, record its index.
    if(g_homeFSNeedsDraw && !prevNeedsDraw)
      g_homeFSIdx = i;

    if(pedal.buttons[i].isPressed && !wasPressedBefore) {
      pedal.lastActiveFSIndex = i;
      if(pedal.buttons[i].mode == FootswitchMode::TapTempo) {
        g_homeFSNeedsDraw = false; // tap tempo gets its own screen
        displayTapTempo(midiClock.bpm);
      }
      // Any FS press exits config / menu and returns to home screen
      if(pedal.inModeSelect || pedal.inActionSelect ||
         pedal.inChannelSelect || pedal.inFSEdit ||
         pedal.menuState != MenuState::NONE) {
        pedal.inModeSelect = false;
        pedal.inActionSelect = false;
        pedal.inChannelSelect = false;
        pedal.inFSEdit = false;
        pedal.fsEditEditing = false;
        pedal.modeSelectFromActionSelect = false;
        pedal.menuState = MenuState::NONE;
        g_homeFSNeedsDraw = false;
        displayHomeScreen(pedal);
        lastInteraction = millis();
      }
    }
  }

  // Preset navigation requested by a preset-nav footswitch
  if(presetNavDirect >= 0) {
    applyPreset((uint8_t)presetNavDirect, pedal);
    displayPresetLoad(activePreset);
    presetNavDirect = -1;
  } else if(presetNavRequest != 0) {
    uint8_t next = (presetNavRequest > 0)
                       ? (activePreset + 1) % NUM_PRESETS
                       : (activePreset == 0 ? NUM_PRESETS - 1 : activePreset - 1);
    applyPreset(next, pedal);
    displayPresetLoad(activePreset);
    presetNavRequest = 0;
  }

  // Redraw home screen if a FS fired and we're not in a param screen.
  if(g_homeFSNeedsDraw && displayMode == DISPLAY_DEFAULT) {
    g_homeFSNeedsDraw = false;
    displayHomeScreen(pedal);
  }

  // Safety net: release all HID keys if no keyboard-mode FS is physically held.
  // Prevents stuck keys from missed release edges or USB re-enumeration.
  {
    bool anyKbPressed = false;
    for(const auto &b : pedal.buttons)
      if(b.isKeyboard && b.isPressed) {
        anyKbPressed = true;
        break;
      }
    if(!anyKbPressed && tud_mounted())
      hidKeyboard.releaseAll();
  }

  syncClockRamps(pedal);

  for(auto &pot : analogPots) {
    handleAnalogPot(pot, pedal, displayPotValue, displayLockedMessage);
  }

  handleEncoder(pedal, displayEncoderFSTurn, displayMidiChannel, displayLockedMessage, displayFSChannel, displayTapTempo, displayModeSelectScreen, displayActionSelect);
  handleEncoderButton(pedal, encoderButtonFSModeChange, displayLockedMessage, displayFSChannel, displayModeSelectScreen, displayActionSelect, []() {
    triggerPresetSaveBlink();
    displayPresetSaved(activePreset);
  });

  // Preset button (short press = next preset, long press = save)
  handlePresetButton(pedal);

  // Combined footswitch actions for preset navigation
  handleCombinedActions(pedal);

  // Drive LEDs: FS LEDs show active preset; activity LED shows last-pressed FS state.
  updatePresetLEDs(pedal);
  updateActivityLed(pedal);

  uint16_t neoVal = (pedal.lastActiveFSIndex >= 0)
                        ? pedal.modForFS(pedal.lastActiveFSIndex).currentValue
                        : 0;
  updateNeoPixel(neoVal, analogPots, pedal.globalSettings.neoPixelEnabled);
  handleExpInput(pedal);

  handleWebServer(pedal);

  // BLE MIDI IN — drain parsed bytes, forward per routing flags.
  bleMidiPoll(pedal.globalSettings.routingFlags);

  handleMidiRouting(pedal);

  delay(10);
}
