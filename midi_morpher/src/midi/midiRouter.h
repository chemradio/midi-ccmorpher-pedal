#pragma once
#include <USBMIDI.h>
#include "../ble/bleMidi.h"
#include "../clock/midiClock.h"
#include "../globalSettings.h"
#include "../pedalState.h"

extern USBMIDI midi;

static midiEventPacket_t _midi_packet_in = {0, 0, 0, 0};
static const int8_t _cin_to_midix_size[16] = {-1, -1, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1};

inline void handleMidiRouting(PedalState &pedal) {
  const uint8_t rf = pedal.globalSettings.routingFlags;

  // DIN MIDI THRU — DIN→DIN always on; DIN→USB and DIN→BLE gated by routing flags.
  static uint8_t dinRunStatus = 0;
  static uint8_t dinMsgBuf[3] = {0, 0, 0};
  static uint8_t dinMsgLen    = 0;
  static uint8_t dinMsgNeed   = 0;

  while(Serial2.available()) {
    byte data = Serial2.read();
    bool isClockByte = (data == 0xF8);

    if(!isClockByte || midiClock.clockOutput) {
      Serial1.write(data);
    }

    if(isClockByte) midiClock.receiveClock();

    if(data >= 0xF8) {
      if(!isClockByte || midiClock.clockOutput) {
        if(rf & ROUTE_DIN_USB) { midiEventPacket_t pkt = {0x0F, data, 0, 0}; midi.writePacket(&pkt); }
        if(rf & ROUTE_DIN_BLE) { uint8_t rt = data; bleMidiSendBytes(&rt, 1); }
      }
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
  // Drain all queued packets per loop iteration so sustained host streams
  // aren't bottlenecked by the main-loop tick rate.
  while(midi.readPacket(&_midi_packet_in)) {
    midi_code_index_number_t code_index_num = MIDI_EP_HEADER_CIN_GET(_midi_packet_in.header);
    int8_t midix_size = _cin_to_midix_size[code_index_num];
    bool isClockPkt = (code_index_num == 0x0F && ((uint8_t *)&_midi_packet_in)[1] == 0xF8);
    if(isClockPkt) midiClock.receiveClock();
    if(code_index_num >= 0x2) {
      if(!isClockPkt || midiClock.clockOutput) {
        uint8_t *midiBytes = ((uint8_t *)&_midi_packet_in) + 1;
        if(rf & ROUTE_USB_DIN) {
          for(int i = 0; i < midix_size; i++) Serial1.write(midiBytes[i]);
        }
        if(midix_size > 0 && midix_size <= 3 && (rf & ROUTE_USB_BLE)) {
          bleMidiSendBytes(midiBytes, (uint8_t)midix_size);
        }
      }
    }
  }
}
