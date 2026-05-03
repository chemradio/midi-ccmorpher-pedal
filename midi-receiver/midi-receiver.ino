// MIDI Morpher — Wireless MIDI Receiver
// Target boards: ESP32-S3, ESP32-S2, ESP32-S2-mini
//
// Transport is selected at runtime by pressing the BOOT button (GPIO 0).
// Cycles: WiFi UDP → BLE (S3 only) → ESP-NOW → …
// Selection is persisted to NVS so it survives reboot.
//
// Output: USB MIDI (native USB) + DIN MIDI (Serial1 TX on MIDI_TX_PIN)

#include "config.h"
#include "src/midi_output.h"
#include "src/transport_manager.h"

USBMIDI usbMidi;

static TransportManager transport;

static void onMidi(const uint8_t *data, size_t len) {
  midiOutBytes(data, len);
}

void setup() {
  Serial.begin(115200);

  USB.VID(0x303A);
  USB.PID(0x8001);
  USB.productName("MIDI Receiver");
  USB.manufacturerName("Tim");
  usbMidi.begin();
  USB.begin();
  delay(500);

  midiOutInit();
  transport.begin(onMidi);
}

void loop() {
  transport.loop();
  delay(1);
}
