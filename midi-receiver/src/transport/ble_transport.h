#pragma once

#ifndef CONFIG_BT_ENABLED
#  error "BLE transport requires a chip with Bluetooth (e.g. ESP32-S3). Use TRANSPORT_WIFI_UDP or TRANSPORT_ESPNOW for S2/S2-mini."
#endif

#include <NimBLEDevice.h>
#include "transport.h"

#define BLE_RX_SERVICE_UUID "03B80E5A-EDE8-4B33-A751-6CE34EC4C700"
#define BLE_RX_CHAR_UUID    "7772E5DB-3868-4112-A1A9-F2669D106BF3"

// ── BLE-MIDI packet parser ────────────────────────────────────────────────────
// Strips BLE-MIDI header/timestamp bytes and extracts raw MIDI bytes.
static void _bleParseMidi(const uint8_t *data, size_t len,
                           MidiDataCallback cb) {
  if (len < 3 || !cb) return;
  static bool inSysEx  = false;
  uint8_t runSt   = 0;
  uint8_t dataRem = 0;
  bool    wantTs  = true;

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

// ── Shared state (NimBLE callbacks run in the BLE task, not main loop) ────────
static MidiDataCallback     _bleCb          = nullptr;
static volatile bool        _bleFoundDevice = false;
static volatile bool        _bleScanDone    = false;
static NimBLEAddress        _bleFoundAddr;

class BleScanCallbacks : public NimBLEScanCallbacks {
public:
  void onResult(const NimBLEAdvertisedDevice *dev) override {
    if (!_bleFoundDevice && dev->getName() == "MIDI Morpher") {
      _bleFoundAddr   = dev->getAddress();
      _bleFoundDevice = true;
      NimBLEDevice::getScan()->stop();  // triggers onScanEnd
    }
  }
  void onScanEnd(const NimBLEScanResults &, int) override {
    _bleScanDone = true;
  }
};
static BleScanCallbacks _bleScanCbs;

static void _bleNotify(NimBLERemoteCharacteristic *,
                       uint8_t *data, size_t len, bool) {
  _bleParseMidi(data, len, _bleCb);
}

class BleClientCallbacks : public NimBLEClientCallbacks {
public:
  void onDisconnect(NimBLEClient *cl, int reason) override {
    Serial.printf("[BLE] Disconnected (reason %d), will rescan\n", reason);
  }
};
static BleClientCallbacks _bleClientCbs;

// ── BLE transport ─────────────────────────────────────────────────────────────

class BleTransport : public Transport {
public:
  void begin(MidiDataCallback cb) override {
    _bleCb          = cb;
    _connected      = false;
    _scanning       = false;
    _bleFoundDevice = false;
    _bleScanDone    = false;
    _lastAttempt    = 0;

    NimBLEDevice::init("");
    NimBLEDevice::setPower(9);

    NimBLEScan *scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(&_bleScanCbs, false);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);

    _startScan();
  }

  void loop() override {
    // Detect server-side disconnect
    if (_connected && _client && !_client->isConnected()) {
      _connected = false;
      Serial.println(F("[BLE] Lost connection, will rescan"));
    }

    // Scan finished — try to connect if device was spotted
    if (_scanning && _bleScanDone) {
      _scanning    = false;
      _bleScanDone = false;
      if (_bleFoundDevice) {
        _bleFoundDevice = false;
        _connect(_bleFoundAddr);
      } else {
        _lastAttempt = millis();  // wait before retry
      }
    }

    // Retry scan when idle and not connected
    if (!_connected && !_scanning && millis() - _lastAttempt > 5000) {
      _startScan();
    }
  }

  void stop() override {
    NimBLEDevice::getScan()->stop();
    if (_client && _client->isConnected()) _client->disconnect();
    NimBLEDevice::deinit(true);
    _connected      = false;
    _scanning       = false;
    _bleFoundDevice = false;
    _bleScanDone    = false;
  }

  bool connected() const override { return _connected; }

private:
  NimBLEClient *_client      = nullptr;
  bool          _connected   = false;
  bool          _scanning    = false;
  unsigned long _lastAttempt = 0;

  void _startScan() {
    _bleFoundDevice = false;
    _bleScanDone    = false;
    _scanning       = true;
    _lastAttempt    = millis();
    Serial.println(F("[BLE] Scanning for MIDI Morpher..."));
    NimBLEDevice::getScan()->start(5, false);  // 5-second async scan
  }

  void _connect(const NimBLEAddress &addr) {
    if (!_client) {
      _client = NimBLEDevice::createClient();
      _client->setClientCallbacks(&_bleClientCbs, false);
      _client->setConnectionParams(12, 12, 0, 150);
    }
    if (!_client->connect(addr)) {
      Serial.println(F("[BLE] Connect failed"));
      _lastAttempt = millis();
      return;
    }
    NimBLERemoteService *svc = _client->getService(BLE_RX_SERVICE_UUID);
    if (!svc) {
      Serial.println(F("[BLE] MIDI service not found"));
      _client->disconnect();
      _lastAttempt = millis();
      return;
    }
    NimBLERemoteCharacteristic *chr = svc->getCharacteristic(BLE_RX_CHAR_UUID);
    if (!chr || !chr->canNotify()) {
      Serial.println(F("[BLE] MIDI char not found or not notifiable"));
      _client->disconnect();
      _lastAttempt = millis();
      return;
    }
    chr->subscribe(true, _bleNotify);
    _connected = true;
    Serial.println(F("[BLE] Connected to MIDI Morpher"));
  }
};
