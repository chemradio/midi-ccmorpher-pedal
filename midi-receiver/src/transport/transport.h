#pragma once
#include <Arduino.h>

// Callback type: called with raw MIDI bytes whenever a complete packet arrives.
typedef void (*MidiDataCallback)(const uint8_t *data, size_t len);

class Transport {
public:
  virtual ~Transport() {}
  virtual void begin(MidiDataCallback cb) = 0;
  virtual void loop()                     = 0;
  virtual void stop()                     = 0;
  virtual bool connected()  const         = 0;
};
