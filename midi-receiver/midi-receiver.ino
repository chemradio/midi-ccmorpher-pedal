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

#if MIDI_ACTIVITY_LED_PIN >= 0
static volatile bool  _midiActivity    = false;
static unsigned long  _midiActivityEnd = 0;
#endif

static void onMidi(const uint8_t *data, size_t len) {
  midiOutBytes(data, len);
#if MIDI_ACTIVITY_LED_PIN >= 0
  _midiActivity = true;
#endif
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

#if MIDI_ACTIVITY_LED_PIN >= 0
  pinMode(MIDI_ACTIVITY_LED_PIN, OUTPUT);
  digitalWrite(MIDI_ACTIVITY_LED_PIN, LOW);
#endif

  midiOutInit();
  transport.begin(onMidi);
}

void loop() {
  transport.loop();

#if MIDI_ACTIVITY_LED_PIN >= 0
  if (_midiActivity) {
    _midiActivity    = false;
    _midiActivityEnd = millis() + MIDI_ACTIVITY_LED_MS;
    digitalWrite(MIDI_ACTIVITY_LED_PIN, HIGH);
  } else if (_midiActivityEnd && millis() >= _midiActivityEnd) {
    _midiActivityEnd = 0;
    digitalWrite(MIDI_ACTIVITY_LED_PIN, LOW);
  }
#endif

  delay(1);
}
