#include "../config.h"
#include "Arduino.h"
#include "X9C10X.h"

X9C10X pot(10000); //  10 KΩ  (ALT-234)

uint8_t direction = LOW;
uint8_t step = 1;
uint8_t prevMidiValue = 0;

void digipotSetup() {
  pot.begin(DIGIPOT_INC, DIGIPOT_UD, DIGIPOT_CS); //  pulse, direction, select
  pot.setPosition(0);
}

void digipotLoop() {
  for(uint8_t i = 0; i < 100; i++) {
    pot.incr();
    delay(200);
  }

  for(uint8_t i = 0; i < 100; i++) {
    pot.decr();
    delay(200);
  }
}

void setDigipotFromMidi(uint8_t midiValue) {
  if(midiValue == prevMidiValue)
    return;
  // Clamp to valid MIDI range
  if(midiValue > 127)
    midiValue = 127;

  // Map MIDI (0-127) to digipot position (0-99)
  uint8_t targetPosition = map(midiValue, 0, 127, 0, 99);

  // Set the position
  pot.setPosition(targetPosition);
}