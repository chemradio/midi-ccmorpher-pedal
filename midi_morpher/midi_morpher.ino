#include "src/analogInOut/expInput.h"
#include "src/ble/bleMidi.h"
#include "src/clock/midiClock.h"
#include "src/config.h"
#include "src/controls/encoder.h"
#include "src/controls/encoderButton.h"
#include "src/controls/pots.h"
#include "src/menu/mainMenu.h"
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

// Trampoline declared extern in bleMidi.h — defined here so bleMidi.h doesn't
// need to include midiClock.h (which would create a cycle via midiOut.h).
void bleMidiOnClockTick() { midiClock.receiveClock(); }

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

  initExpInput();
  pedal.initButtons();
  initEncoder();
  initEncoderButton();
  analogReadResolution(12);
  initAnalogPots();

  // Preset button and activity LED
  pinMode(PRESET_BTN_PIN,  INPUT_PULLUP);
  pinMode(ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(ACTIVITY_LED_PIN, LOW);
  pinMode(TEMPO_LED_PIN, OUTPUT);
  digitalWrite(TEMPO_LED_PIN, LOW);

  // Load presets + global settings, then apply global settings to live state
  loadAllPresets(pedal);
  loadGlobalSettings(pedal);
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
  midiClock.tick();
  pedal.modulator.update();

  // Process footswitches, tracking which was last pressed for the activity LED.
  for(int i = 0; i < (int)pedal.buttons.size(); i++) {
    bool wasPressedBefore = pedal.buttons[i].isPressed;
    pedal.buttons[i].handleFootswitch(pedal.midiChannel, pedal.modulator, displayFootswitchPress);
    if(pedal.buttons[i].isPressed && !wasPressedBefore) {
      pedal.lastActiveFSIndex = i;
      // Tap tempo press: show the updated BPM after receiveTap() has run.
      if(pedal.buttons[i].mode == FootswitchMode::TapTempo) {
        displayTapTempo(midiClock.bpm);
      }
    }
  }

  // Keep the live modulator's ramp times synced when the active button's
  // rampUpMs / rampDownMs are clock-locked and BPM has drifted.
  if(pedal.lastActiveFSIndex >= 0) {
    FSButton &ab = pedal.buttons[pedal.lastActiveFSIndex];
    if(ab.isModSwitch) {
      if(ab.rampUpMs   & CLOCK_SYNC_FLAG)
        pedal.modulator.rampUpTimeMs   = midiClock.syncToMs(ab.rampUpMs);
      if(ab.rampDownMs & CLOCK_SYNC_FLAG)
        pedal.modulator.rampDownTimeMs = midiClock.syncToMs(ab.rampDownMs);
    }
  }

  for(auto &pot : analogPots) {
    handleAnalogPot(pot, pedal, displayPotValue, displayLockedMessage);
  }

  handleEncoder(pedal, displayEncoderFSTurn, displayMidiChannel, displayLockedMessage, displayFSChannel, displayTapTempo, displayModeSelectScreen);
  handleEncoderButton(pedal, encoderButtonFSModeChange, pedal.modulator, displayLockedMessage, displayFSChannel, displayModeSelectScreen);

  // Preset button (short press = next preset, long press = save)
  handlePresetButton(pedal);

  // Drive LEDs: FS LEDs show active preset; activity LED shows last-pressed FS state.
  updatePresetLEDs(pedal);
  updateActivityLed(pedal);

  updateNeoPixel(pedal.modulator.currentValue, analogPots, pedal.globalSettings.neoPixelEnabled);
  handleExpInput(pedal);
  resetDisplayTimeout(pedal);

  handleWebServer(pedal);

  // BLE MIDI IN — drain parsed bytes, forward per routing flags.
  bleMidiPoll(pedal.globalSettings.routingFlags);

  // DIN MIDI THRU — DIN→DIN always on; DIN→USB and DIN→BLE gated by routing flags.
  static uint8_t dinRunStatus = 0;
  static uint8_t dinMsgBuf[3] = {0, 0, 0};
  static uint8_t dinMsgLen    = 0;
  static uint8_t dinMsgNeed   = 0;
  const uint8_t  rf           = pedal.globalSettings.routingFlags;
  while(Serial2.available()) {
    byte data = Serial2.read();
    Serial1.write(data);  // DIN→DIN always
    Serial1.flush();

    if(data == 0xF8) midiClock.receiveClock();

    if(data >= 0xF8) {
      if(rf & ROUTE_DIN_USB) { midiEventPacket_t pkt = {0x0F, data, 0, 0}; midi.writePacket(&pkt); }
      if(rf & ROUTE_DIN_BLE) { uint8_t rt = data; bleMidiSendBytes(&rt, 1); }
      continue;
    }

    if(data & 0x80) {
      if(data >= 0xF0) { dinRunStatus = 0; dinMsgLen = 0; continue; }
      dinRunStatus = data;
      dinMsgBuf[0] = data;
      dinMsgLen    = 1;
      uint8_t hi   = data & 0xF0;
      dinMsgNeed   = (hi == 0xC0 || hi == 0xD0) ? 2 : 3;
    } else {
      if(dinRunStatus == 0) continue;
      if(dinMsgLen == 0) {
        dinMsgBuf[0] = dinRunStatus;
        dinMsgLen    = 1;
        uint8_t hi   = dinRunStatus & 0xF0;
        dinMsgNeed   = (hi == 0xC0 || hi == 0xD0) ? 2 : 3;
      }
      dinMsgBuf[dinMsgLen++] = data;
    }

    if(dinMsgLen >= dinMsgNeed) {
      if(rf & ROUTE_DIN_USB) {
        uint8_t cin = (dinMsgBuf[0] & 0xF0) >> 4;
        midiEventPacket_t pkt = { cin, dinMsgBuf[0], dinMsgBuf[1], dinMsgBuf[2] };
        midi.writePacket(&pkt);
      }
      if(rf & ROUTE_DIN_BLE) bleMidiSendBytes(dinMsgBuf, dinMsgNeed);
      dinMsgLen = 0;
    }
  }

  // USB MIDI IN — USB→DIN and USB→BLE gated by routing flags.
  if(midi.readPacket(&midi_packet_in)) {
    midi_code_index_number_t code_index_num = MIDI_EP_HEADER_CIN_GET(midi_packet_in.header);
    int8_t midix_size = cin_to_midix_size[code_index_num];
    if(code_index_num == 0x0F && ((uint8_t *)&midi_packet_in)[1] == 0xF8) {
      midiClock.receiveClock();
    }
    if(code_index_num >= 0x2) {
      uint8_t *midiBytes = ((uint8_t *)&midi_packet_in) + 1;
      if(rf & ROUTE_USB_DIN) {
        for(int i = 0; i < midix_size; i++) Serial1.write(midiBytes[i]);
        Serial1.flush();
      }
      if(midix_size > 0 && midix_size <= 3 && (rf & ROUTE_USB_BLE)) {
        bleMidiSendBytes(midiBytes, (uint8_t)midix_size);
      }
    }
  }

  delay(10);
}
