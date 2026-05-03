#pragma once
#include <esp_now.h>
#include <WiFi.h>

static bool          _espNowActive           = false;
static const uint8_t _espNowBcast[6]         = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
// Magic sent by receivers that want to register as a unicast peer.
static const uint8_t _espNowPairMagic[4]     = {'M','M','P','R'};
// ACK sent back to confirm pairing.
static const uint8_t _espNowPairAck[4]       = {'M','M','P','A'};

// Receive callback — always active when ESP-NOW is enabled.
// On pairing request: add sender as unicast peer + send ACK.
static void _espNowOnRecv(const esp_now_recv_info_t *info,
                          const uint8_t *data, int len) {
  if (len < 4 || memcmp(data, _espNowPairMagic, 4) != 0) return;
  if (!esp_now_is_peer_exist(info->src_addr)) {
    esp_now_peer_info_t p = {};
    memcpy(p.peer_addr, info->src_addr, 6);
    p.channel = 0;   // inherit current WiFi channel
    p.encrypt = false;
    p.ifidx   = WIFI_IF_AP;
    esp_now_add_peer(&p);
  }
  esp_now_send(info->src_addr, _espNowPairAck, 4);
}

inline void espNowInit() {
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(_espNowOnRecv);
  // Add broadcast peer so we can send without prior pairing.
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, _espNowBcast, 6);
  p.channel = 0;
  p.encrypt = false;
  p.ifidx   = WIFI_IF_AP;
  esp_now_add_peer(&p);
  _espNowActive = true;
}

inline void espNowDeinit() {
  esp_now_deinit();
  _espNowActive = false;
}

inline bool espNowActive() { return _espNowActive; }

// Broadcast raw MIDI bytes. Also reaches any unicast-paired receivers because
// broadcast is received by all ESP-NOW nodes on the same channel.
inline void espNowSend(const uint8_t *bytes, size_t len) {
  if (!_espNowActive || len == 0 || len > 250) return;
  esp_now_send(_espNowBcast, bytes, len);
}
