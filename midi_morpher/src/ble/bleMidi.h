#pragma once
// BLE MIDI — third transport alongside DIN (Serial1) and USB.
// Uses NimBLE-Arduino (>= 2.0) for the GATT server. Implements the standard
// Apple BLE-MIDI service so iOS/macOS/Windows/Linux/Android all recognize it
// without extra drivers.
//
// - TX: every MIDI message sent by the pedal is mirrored to the BLE
//   characteristic via notify(), wrapped in the BLE-MIDI timestamp header.
// - RX: incoming BLE-MIDI packets are parsed in the NimBLE task, the raw MIDI
//   bytes are pushed to a ring buffer, and drained from the main loop by
//   bleMidiPoll() which forwards them to DIN + USB (Thru behavior) and feeds
//   0xF8 clock bytes to midiClock — identical to the DIN-in handler.

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <USB.h>
#include <USBMIDI.h>

#include "../config.h"
#include "../globalSettings.h"

extern USBMIDI midi;
extern "C" bool tud_mounted(void);

// Trampoline implemented in midi_morpher.ino — taps received 0xF8 bytes into
// midiClock.receiveClock(). Declared extern here to avoid pulling in
// midiClock.h (which would create a cycle: midiClock.h → midiOut.h → bleMidi.h).
extern void bleMidiOnClockTick();

// ── Standard BLE MIDI UUIDs ───────────────────────────────────────────────────
#define BLE_MIDI_SERVICE_UUID "03B80E5A-EDE8-4B33-A751-6CE34EC4C700"
#define BLE_MIDI_CHAR_UUID    "7772E5DB-3868-4112-A1A9-F2669D106BF3"

// ── State ─────────────────────────────────────────────────────────────────────
inline NimBLEServer         *bleMidiServer = nullptr;
inline NimBLECharacteristic *bleMidiChar   = nullptr;
inline volatile bool         bleMidiConnected = false;

// Incoming-byte ring buffer. BLE task pushes parsed MIDI bytes in; main loop
// drains and forwards. Sized for a few buffered BLE packets.
#define BLE_MIDI_RX_BUF 512
inline uint8_t           bleMidiRxBuf[BLE_MIDI_RX_BUF];
inline volatile uint16_t bleMidiRxHead = 0;
inline volatile uint16_t bleMidiRxTail = 0;

inline void bleMidiRxPush(uint8_t b) {
  uint16_t next = (uint16_t)((bleMidiRxHead + 1) % BLE_MIDI_RX_BUF);
  if(next == bleMidiRxTail) return;  // full — drop byte
  bleMidiRxBuf[bleMidiRxHead] = b;
  bleMidiRxHead = next;
}

inline bool bleMidiRxPop(uint8_t &out) {
  if(bleMidiRxHead == bleMidiRxTail) return false;
  out = bleMidiRxBuf[bleMidiRxTail];
  bleMidiRxTail = (uint16_t)((bleMidiRxTail + 1) % BLE_MIDI_RX_BUF);
  return true;
}

// ── TX helpers ────────────────────────────────────────────────────────────────
// BLE-MIDI packet = header byte + timestamp byte + MIDI bytes.
//   header:    1 0 T T T T T T   (bits 12..7 of a 13-bit millisecond timestamp)
//   timestamp: 1 T T T T T T T   (bits  6..0 of the timestamp)

inline void bleMidiSendBytes(const uint8_t *midiBytes, uint8_t n) {
  if(!bleMidiConnected || !bleMidiChar) return;
  if(n == 0 || n > 3) return;
  uint32_t ts = millis() & 0x1FFF;
  uint8_t pkt[5];
  pkt[0] = 0x80 | ((uint8_t)(ts >> 7) & 0x3F);
  pkt[1] = 0x80 | ((uint8_t)ts & 0x7F);
  for(uint8_t i = 0; i < n; i++) pkt[2 + i] = midiBytes[i];
  bleMidiChar->setValue(pkt, n + 2);
  bleMidiChar->notify();
}

// SysEx: data must include leading 0xF0 and trailing 0xF7. BLE-MIDI rules
// require a timestamp byte before 0xF0 and before 0xF7.
inline void bleMidiSendSysEx(const uint8_t *data, uint8_t n) {
  if(!bleMidiConnected || !bleMidiChar) return;
  if(n < 2 || n > 28) return;  // header+ts+data+ts ≤ 32 bytes
  uint32_t ts = millis() & 0x1FFF;
  uint8_t tsHi = 0x80 | ((uint8_t)(ts >> 7) & 0x3F);
  uint8_t tsLo = 0x80 | ((uint8_t)ts & 0x7F);
  uint8_t pkt[32];
  uint8_t o = 0;
  pkt[o++] = tsHi;                         // header
  pkt[o++] = tsLo;                         // timestamp for F0
  for(uint8_t i = 0; i < n - 1; i++)
    pkt[o++] = data[i];                    // F0 + body
  pkt[o++] = tsLo;                         // timestamp for F7
  pkt[o++] = data[n - 1];                  // F7
  bleMidiChar->setValue(pkt, o);
  bleMidiChar->notify();
}

// ── RX callback — parses BLE-MIDI, pushes raw MIDI bytes to the ring ────────
// Called in the NimBLE task context. Uses a state machine to correctly
// distinguish BLE-MIDI timestamp bytes from MIDI status bytes, handles
// running status within a packet, and passes SysEx through intact.
inline void bleMidiParseIncoming(const uint8_t *data, size_t len) {
  if(len < 3) return;                       // header + ts + ≥1 MIDI byte

  static bool inSysEx = false;              // persists across BLE packets (SysEx can span)
  uint8_t runSt   = 0;                      // running status (per-packet)
  uint8_t dataRem = 0;                      // data bytes still needed for current msg
  bool    wantTs  = true;                    // expecting a timestamp byte next

  size_t i = 1;                              // skip header (data[0])
  while(i < len) {
    uint8_t b = data[i++];

    // System realtime (0xF8–0xFF): pass through in any state.
    if(b >= 0xF8) { bleMidiRxPush(b); continue; }

    // ── Inside SysEx ──────────────────────────────────────────────────────────
    if(inSysEx) {
      if(b == 0xF7)               { bleMidiRxPush(b); inSysEx = false; wantTs = true; continue; }
      if((b & 0x80) && i < len
         && data[i] == 0xF7)        continue;          // timestamp before F7 — skip
      if(!(b & 0x80))               bleMidiRxPush(b);  // SysEx data byte
      continue;
    }

    // ── Timestamp consumption ─────────────────────────────────────────────────
    if(wantTs) {
      if(b & 0x80) { wantTs = false; continue; }       // skip timestamp
      continue;                                         // bit-7-clear when expecting ts → malformed
    }

    // ── MIDI status or data ───────────────────────────────────────────────────
    if(b & 0x80) {
      if(b == 0xF0)  { bleMidiRxPush(b); inSysEx = true; runSt = 0; continue; }
      if(b >= 0xF0)  { bleMidiRxPush(b); runSt = 0; dataRem = 0; wantTs = true; continue; }
      // Channel voice status byte
      runSt = b;
      bleMidiRxPush(b);
      uint8_t hi = b & 0xF0;
      dataRem = (hi == 0xC0 || hi == 0xD0) ? 1 : 2;
    } else {
      // Data byte (may be running-status)
      if(runSt == 0) continue;                          // orphaned — skip
      bleMidiRxPush(b);
      if(dataRem == 0) {                                // running-status: re-derive count
        uint8_t hi = runSt & 0xF0;
        dataRem = (hi == 0xC0 || hi == 0xD0) ? 1 : 2;
      }
      if(--dataRem == 0) wantTs = true;                 // message complete
    }
  }
}

// ── NimBLE callback classes ───────────────────────────────────────────────────
class BleMidiCharCallbacks : public NimBLECharacteristicCallbacks {
public:
  void onWrite(NimBLECharacteristic *chr, NimBLEConnInfo &) override {
    std::string v = chr->getValue();
    bleMidiParseIncoming((const uint8_t *)v.data(), v.length());
  }
};

class BleMidiServerCallbacks : public NimBLEServerCallbacks {
public:
  void onConnect(NimBLEServer *srv, NimBLEConnInfo &info) override {
    bleMidiConnected = true;
    // Request 7.5–15 ms connection interval for low-latency MIDI.
    // (min 6×1.25=7.5ms, max 12×1.25=15ms, latency 0, timeout 2s)
    srv->updateConnParams(info.getConnHandle(), 6, 12, 0, 200);
  }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override {
    bleMidiConnected = false;
    NimBLEDevice::startAdvertising();
  }
};

// ── Init ──────────────────────────────────────────────────────────────────────
inline void initBleMidi() {
  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setPower(9);              // +9 dBm (NimBLE 2.x takes dBm, not enum)

  bleMidiServer = NimBLEDevice::createServer();
  bleMidiServer->setCallbacks(new BleMidiServerCallbacks());

  NimBLEService *svc = bleMidiServer->createService(BLE_MIDI_SERVICE_UUID);
  bleMidiChar = svc->createCharacteristic(
      BLE_MIDI_CHAR_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY);
  bleMidiChar->setCallbacks(new BleMidiCharCallbacks());
  svc->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(BLE_MIDI_SERVICE_UUID);
  adv->setName(BLE_DEVICE_NAME);
  adv->enableScanResponse(true);
  adv->start();
}

// ── Main-loop drain — forwards received bytes to DIN + USB + clock ──────────
// routingFlags gates BLE→DIN (ROUTE_BLE_DIN) and BLE→USB (ROUTE_BLE_USB).
inline void bleMidiPoll(uint8_t routingFlags) {
  static uint8_t runStatus = 0;
  static uint8_t msgBuf[3] = {0, 0, 0};
  static uint8_t msgLen    = 0;
  static uint8_t msgNeed   = 0;

  const bool toDin = (routingFlags & ROUTE_BLE_DIN) != 0;
  const bool toUsb = (routingFlags & ROUTE_BLE_USB) != 0;

  uint8_t b;
  while(bleMidiRxPop(b)) {
    if(toDin) Serial1.write(b);

    if(b == 0xF8) bleMidiOnClockTick();

    if(b >= 0xF8) {
      if(toUsb && tud_mounted()) {
        midiEventPacket_t pkt = {0x0F, b, 0, 0};
        midi.writePacket(&pkt);
      }
      continue;
    }

    if(b & 0x80) {
      if(b >= 0xF0) { runStatus = 0; msgLen = 0; continue; }
      runStatus = b;
      msgBuf[0] = b;
      msgLen    = 1;
      uint8_t hi = b & 0xF0;
      msgNeed   = (hi == 0xC0 || hi == 0xD0) ? 2 : 3;
    } else {
      if(runStatus == 0) continue;
      if(msgLen == 0) {
        msgBuf[0] = runStatus;
        msgLen    = 1;
        uint8_t hi = runStatus & 0xF0;
        msgNeed   = (hi == 0xC0 || hi == 0xD0) ? 2 : 3;
      }
      msgBuf[msgLen++] = b;
    }

    if(msgLen >= msgNeed) {
      if(toUsb && tud_mounted()) {
        uint8_t cin = (msgBuf[0] & 0xF0) >> 4;
        midiEventPacket_t pkt = { cin, msgBuf[0], msgBuf[1], msgBuf[2] };
        midi.writePacket(&pkt);
      }
      msgLen = 0;
    }
  }
  if(toDin) Serial1.flush();
}
