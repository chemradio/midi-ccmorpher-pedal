#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include "transport.h"
#include "../../config.h"

// Magic bytes used for pairing handshake with the Morpher.
static const uint8_t _rxPairReq[4] = {'M','M','P','R'};
static const uint8_t _rxPairAck[4] = {'M','M','P','A'};

static MidiDataCallback _espNowRxCb    = nullptr;
static volatile bool    _espNowAckRecv = false;

static void _espNowRxRecv(const esp_now_recv_info_t *info,
                           const uint8_t *data, int len) {
  if (len < 1) return;
  if (len == 4 && memcmp(data, _rxPairAck, 4) == 0) { _espNowAckRecv = true; return; }
  if (len == 4 && memcmp(data, _rxPairReq, 4) == 0) return;  // ignore our own bcast loopback
  if (_espNowRxCb)
    _espNowRxCb(data, (size_t)len);
}

class EspNowTransport : public Transport {
public:
  void begin(MidiDataCallback cb) override {
    _espNowRxCb    = cb;
    _connected     = false;
    _espNowAckRecv = false;

    // STA mode required for ESP-NOW; set channel to match Morpher's AP.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    // Fix the channel so ESP-NOW packets arrive on the right channel.
    esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
      Serial.println(F("[ESPNOW] Init failed"));
      return;
    }
    esp_now_register_recv_cb(_espNowRxRecv);

    // Add Morpher broadcast as a send peer so we can transmit pair requests.
    static const uint8_t bcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    esp_now_peer_info_t p = {};
    memcpy(p.peer_addr, bcast, 6);
    p.channel = ESPNOW_WIFI_CHANNEL;
    p.encrypt = false;
    p.ifidx   = WIFI_IF_STA;
    esp_now_add_peer(&p);

    _sendPairRequest();
    Serial.println(F("[ESPNOW] Listening for MIDI Morpher broadcasts"));
  }

  void loop() override {
    // Promote to "connected" only when we've seen the Morpher's MMPA ACK.
    if (!_connected && _espNowAckRecv) {
      _connected = true;
      Serial.println(F("[ESPNOW] Paired with MIDI Morpher"));
    }
    // Re-send pair request periodically so the Morpher registers this device
    // as a unicast peer even after a Morpher reboot.
    if (millis() - _lastPair > 30000) {
      _sendPairRequest();
    }
  }

  void stop() override {
    esp_now_deinit();
    WiFi.disconnect(true, true);
    _connected = false;
  }

  bool connected() const override { return _connected; }

private:
  bool          _connected = false;
  unsigned long _lastPair  = 0;

  void _sendPairRequest() {
    static const uint8_t bcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    esp_now_send(bcast, _rxPairReq, 4);
    _lastPair = millis();
  }
};
