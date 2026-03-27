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
#include "src/visual/neopx.h"
#include <USB.h>
#include <USBMIDI.h>

// initialize global state
USBMIDI midi;
PedalState pedal;
midiEventPacket_t midi_packet_in = {0, 0, 0, 0};
int8_t cin_to_midix_size[16] = {-1, -1, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1};

void setup() {
  // Serial.begin(115200); // Initialize serial for debugging
  Serial1.begin(31250, SERIAL_8N1, -1, MIDI_TX); // MIDI OUT
  // pinMode(MIDI_RX, INPUT_PULLUP);
  Serial2.begin(31250, SERIAL_8N1, MIDI_RX, -1); // MIDI IN
  USB.VID(0x303A);                               // Espressif VID
  USB.PID(0x8000);                               // Custom PID
  USB.productName("MIDI Morpher 1.0");
  USB.manufacturerName("Tim");
  USB.firmwareVersion(0x0100);
  midi.begin();
  USB.begin();
  delay(1000);
  if(!initDisplay()) {
    while(1)
      delay(1000);
  }
  digipotSetup();
  initToggles(pedal);
  pedal.initButtons();
  initEncoder();
  initEncoderButton();
  analogReadResolution(12);
  initAnalogPots();
  // hotswitch.init();
  loadState(pedal);
  initNeoPixel();
  showStartupScreen();
  delay(2000);
}

void loop() {
  // digipotLoop();
  pedal.modulator.update();
  for(auto &button : pedal.buttons) {
    button.handleFootswitch(pedal.midiChannel, pedal.modulator, displayFootswitchPress);
  }

  for(auto &tgl : toggles) {
    bool toggleChanged = handleToggleChange(tgl, pedal, displayLockedMessage);
    if(toggleChanged) {
      displayHomeScreen(pedal);
    }
  }
  for(auto &pot : analogPots) {
    handleAnalogPot(pot, pedal, displayPotValue, displayLockedMessage);
  }
  handleEncoder(pedal, displayEncoderTurn, displayLockedMessage);
  handleEncoderButton(pedal, encoderButtonFSModeChange, pedal.modulator, displayLockedMessage);

  updateNeoPixel(pedal.modulator.currentValue, analogPots);
  setDigipotFromMidi(pedal.modulator.currentValue);
  resetDisplayTimeout(pedal);
  // checkAndSaveState(pedal);

  // // MIDI THRU functionality
  // while (Serial2.available() > 0)
  // {
  //     uint8_t data = Serial2.read();
  //     // Direct Pass-through
  //     midi.write(data);
  //     Serial1.write(data);
  // }

  // DIN MIDI THROUGH - mirrors IDN input to DIN output and also USB (USB doesn't work)
  while(Serial2.available()) {
    // uint8_t b = Serial2.read();
    byte data = Serial2.read();
    Serial1.write(data);
    Serial1.flush();
    midi.write(data);
  }

  // USB MIDI IN - mirror to DIN output - WORKS!
  if(midi.readPacket(&midi_packet_in)) {
    midi_code_index_number_t code_index_num = MIDI_EP_HEADER_CIN_GET(midi_packet_in.header);
    int8_t midix_size = cin_to_midix_size[code_index_num];

    // We skip Misc and Cable Events for simplicity
    if(code_index_num >= 0x2) {
      for(int i = 0; i < midix_size; i++) {
        Serial1.write(((uint8_t *)&midi_packet_in)[i + 1]);
      }
      Serial1.flush();
    }
  }

  delay(10);
}
