#pragma once

// BLE transport is only available on chips with Bluetooth hardware.
// Attempting to compile on ESP32-S2 / S2-mini will produce a clear error.
#ifndef CONFIG_BT_ENABLED
#  error "BLE transport requires a chip with Bluetooth (e.g. ESP32-S3). Use TRANSPORT_WIFI_UDP or TRANSPORT_ESPNOW for S2/S2-mini."
#endif

#include <NimBLEDevice.h>
#include "transport.h"

// Standard Apple BLE-MIDI UUIDs
#define BLE_RX_SERVICE_UUID "03B80E5A-EDE8-4B33-A751-6CE34EC4C700"
#define BLE_RX_CHAR_UUID    "7772E5DB-3868-4112-A1A9-F2669D106BF3"

// ── BLE-MIDI packet parser ────────────────────────────────────────────────────
// Strips BLE-MIDI header/timestamp bytes and extracts raw MIDI bytes.
// Mirrors the logic in bleMidi.h on the Morpher side.
static void _bleParseMidi(const uint8_t *data, size_t len,
                           MidiDataCallback cb) {
  if (len < 3 || !cb) return;
  static bool inSysEx  = false;
  uint8_t runSt   = 0;
  uint8_t dataRem = 0;
  bool    wantTs  = true;

  // Collect parsed bytes and fire callback per complete message.
  uint8_t msg[3];
  uint8_t msgLen = 0;

  auto flush = [&]() {
    if (msgLen > 0) { cb(msg, msgLen); msgLen = 0; }
  };
  auto push = [&](uint8_t b) {
    if (msgLen < 3) msg[msgLen++] = b;
  };

  size_t i = 1;  // skip packet header byte
  while (i < len) {
    uint8_t b = data[i++];

    if (b >= 0xF8) {
      flush();
      uint8_t rt[1] = {b};
      cb(rt, 1);
      continue;
    }

    if (inSysEx) {
      if (b == 0xF7) {
        push(b); flush(); inSysEx = false; wantTs = true;
      } else if ((b & 0x80) && i < len && data[i] == 0xF7) {
        continue;  // timestamp before F7
      } else if (!(b & 0x80)) {
        push(b);
      }
      continue;
    }

    if (wantTs) {
      if (b & 0x80) { wantTs = false; }
      continue;
    }

    if (b & 0x80) {
      flush();
      if (b == 0xF0) {
        push(b); inSysEx = true; runSt = 0; continue;
      }
      if (b >= 0xF0) {
        uint8_t sc[1] = {b}; cb(sc, 1);
        runSt = 0; dataRem = 0; wantTs = true; continue;
      }
      runSt = b;
      push(b);
      uint8_t hi = b & 0xF0;
      dataRem = (hi == 0xC0 || hi == 0xD0) ? 1 : 2;
    } else {
      if (!runSt) continue;
      push(b);
      if (dataRem == 0) {
        uint8_t hi = runSt & 0xF0;
        dataRem = (hi == 0xC0 || hi == 0xD0) ? 1 : 2;
      }
      if (--dataRem == 0) { flush(); wantTs = true; }
    }
  }
  flush();
}

// ── NimBLE client transport ───────────────────────────────────────────────────

static MidiDataCallback _bleCb = nullptr;

class BleNotifyCallbacks : public NimBLEClientCallbacks {
public:
  void onDisconnect(NimBLEClient *cl, int reason) override {
    Serial.printf("[BLE] Disconnected (reason %d), will retry\n", reason);
  }
};
static BleNotifyCallbacks _bleCallbacks;

static void _bleNotify(NimBLERemoteCharacteristic *,
                       uint8_t *data, size_t len, bool) {
  _bleParseMidi(data, len, _bleCb);
}

class BleTransport : public Transport {
public:
  void begin(MidiDataCallback cb) override {
    _bleCb     = cb;
    _connected = false;
    NimBLEDevice::init("");
    NimBLEDevice::setPower(9);
    Serial.println(F("[BLE] Scanning for MIDI Morpher..."));
    _startScan();
  }

  void loop() override {
    if (_connected && !_client->isConnected()) {
      _connected = false;
      Serial.println(F("[BLE] Connection dropped, rescanning..."));
    }
    if (!_connected && millis() - _lastAttempt > 5000) {
      _lastAttempt = millis();
      _startScan();
    }
  }

  void stop() override {
    if (_client && _client->isConnected()) _client->disconnect();
    NimBLEDevice::deinit(true);
    _connected = false;
  }

  bool connected() const override { return _connected; }

private:
  NimBLEClient *_client      = nullptr;
  bool          _connected   = false;
  unsigned long _lastAttempt = 0;

  void _startScan() {
    NimBLEScan *scan = NimBLEDevice::getScan();
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);
    // Blocking scan for up to 5 s; non-blocking variant requires callbacks
    // and complicates the reconnect loop — keep it simple here.
    NimBLEScanResults results = scan->start(5, false);
    for (int i = 0; i < results.getCount(); i++) {
      NimBLEAdvertisedDevice dev = results.getDevice(i);
      if (dev.getName() == "MIDI Morpher") {
        scan->stop();
        _connect(dev.getAddress());
        return;
      }
    }
    Serial.println(F("[BLE] MIDI Morpher not found, will retry"));
  }

  void _connect(const NimBLEAddress &addr) {
    if (!_client) {
      _client = NimBLEDevice::createClient();
      _client->setClientCallbacks(&_bleCallbacks, false);
      _client->setConnectionParams(12, 12, 0, 150);
    }
    if (!_client->connect(addr)) {
      Serial.println(F("[BLE] Connect failed"));
      return;
    }
    NimBLERemoteService *svc = _client->getService(BLE_RX_SERVICE_UUID);
    if (!svc) {
      Serial.println(F("[BLE] MIDI service not found"));
      _client->disconnect();
      return;
    }
    NimBLERemoteCharacteristic *chr = svc->getCharacteristic(BLE_RX_CHAR_UUID);
    if (!chr || !chr->canNotify()) {
      Serial.println(F("[BLE] MIDI char not found or not notifiable"));
      _client->disconnect();
      return;
    }
    chr->subscribe(true, _bleNotify);
    _connected = true;
    Serial.println(F("[BLE] Connected to MIDI Morpher"));
  }
};
