#include "src/analogInOut/digipot.h"
#include "src/config.h"
#include "src/controls/encoder.h"
#include "src/controls/encoderButton.h"
#include "src/controls/pots.h"
#include "src/controls/toggles.h"
#include "src/midiOut.h"
#include "src/pedalState.h"
#include "src/statePersistance.h"
#include "src/visual/display.h"
#include "src/presets.h"
#include "src/visual/neopx.h"
#include "src/wifi/webServer.h"
#include <USB.h>
#include <USBMIDI.h>

// initialize global state
USBMIDI midi;
PedalState pedal;
midiEventPacket_t midi_packet_in = {0, 0, 0, 0};
int8_t cin_to_midix_size[16] = {-1, -1, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1};

void setup() {
  // Serial.begin(115200);
  Serial1.begin(31250, SERIAL_8N1, -1, MIDI_TX);
  Serial2.begin(31250, SERIAL_8N1, MIDI_RX, -1);
  USB.VID(0x303A);
  USB.PID(0x8000);
  USB.productName("MIDI Morpher 1.0");
  USB.manufacturerName("Tim");
  USB.firmwareVersion(0x0100);
  midi.begin();
  USB.begin();
  delay(1000);

  if(!initDisplay()) {
    while(1) delay(1000);
  }

  digipotSetup();
  initToggles(pedal);
  pedal.initButtons();
  initEncoder();
  initEncoderButton();
  analogReadResolution(12);
  initAnalogPots();

  // Preset button and activity LED
  pinMode(PRESET_BTN_PIN,  INPUT_PULLUP);
  pinMode(ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(ACTIVITY_LED_PIN, LOW);

  // Load all presets from NVS and apply the active one
  loadAllPresets(pedal);

  initWebServer(pedal);
  initNeoPixel();
  showStartupScreen();
  delay(2000);

  // Show the home screen immediately after startup
  displayHomeScreen(pedal);
  updatePresetLEDs(pedal);
}

void loop() {
  pedal.modulator.update();

  // Process footswitches, tracking which was last pressed for the activity LED.
  for(int i = 0; i < (int)pedal.buttons.size(); i++) {
    bool wasPressedBefore = pedal.buttons[i].isPressed;
    pedal.buttons[i].handleFootswitch(pedal.midiChannel, pedal.modulator, displayFootswitchPress);
    if(pedal.buttons[i].isPressed && !wasPressedBefore) {
      pedal.lastActiveFSIndex = i;
    }
  }

  for(auto &tgl : toggles) {
    bool toggleChanged = handleToggleChange(tgl, pedal, displayLockedMessage, displayLockChange);
    if(toggleChanged) {
      displayHomeScreen(pedal);
    }
  }

  for(auto &pot : analogPots) {
    handleAnalogPot(pot, pedal, displayPotValue, displayLockedMessage);
  }

  handleEncoder(pedal, displayEncoderFSTurn, displayMidiChannel, displayLockedMessage, displayFSChannel);
  handleEncoderButton(pedal, encoderButtonFSModeChange, pedal.modulator, displayLockedMessage, displayFSChannel);

  // Preset button (short press = next preset, long press = save)
  handlePresetButton(pedal);

  // Drive LEDs: FS LEDs show active preset; activity LED shows last-pressed FS state.
  updatePresetLEDs(pedal);
  updateActivityLed(pedal);

  updateNeoPixel(pedal.modulator.currentValue, analogPots);
  setDigipotFromMidi(pedal.modulator.currentValue);
  resetDisplayTimeout(pedal);

  handleWebServer(pedal);

  // DIN MIDI THRU — mirrors DIN input to DIN output
  while(Serial2.available()) {
    byte data = Serial2.read();
    Serial1.write(data);
    Serial1.flush();
    midi.write(data);
  }

  // USB MIDI IN — mirror to DIN output
  if(midi.readPacket(&midi_packet_in)) {
    midi_code_index_number_t code_index_num = MIDI_EP_HEADER_CIN_GET(midi_packet_in.header);
    int8_t midix_size = cin_to_midix_size[code_index_num];
    if(code_index_num >= 0x2) {
      for(int i = 0; i < midix_size; i++) {
        Serial1.write(((uint8_t *)&midi_packet_in)[i + 1]);
      }
      Serial1.flush();
    }
  }

  delay(10);
}
