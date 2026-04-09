#pragma once
#include "../config.h"
#include <Arduino.h>
#include <SPI.h>

// AD5292-BRUZ-20: 1024-position SPI digipot
// 16-bit transfer: bits[15:10] = command, bits[9:0] = data

inline SPIClass digipotSPI(HSPI);

inline void ad5292Write(uint8_t cmd, uint16_t data) {
  uint16_t word = ((uint16_t)(cmd & 0x3F) << 10) | (data & 0x3FF);
  digitalWrite(DIGIPOT_CS, LOW);
  digipotSPI.transfer16(word);
  digitalWrite(DIGIPOT_CS, HIGH);
}

inline void digipotSetup() {
  pinMode(DIGIPOT_CS, OUTPUT);
  digitalWrite(DIGIPOT_CS, HIGH);
  digipotSPI.begin(DIGIPOT_SCK, -1, DIGIPOT_MOSI, DIGIPOT_CS);
  digipotSPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));

  // Unlock RDAC write protect (control register bit D1 = 1)
  ad5292Write(0b000111, 0b10);
  // Set wiper to 0 on startup
  ad5292Write(0b000001, 0);
}

inline uint8_t prevMidiValue = 255; // force write on first call

inline void setDigipotFromMidi(uint8_t midiValue) {
  if (midiValue == prevMidiValue) return;
  prevMidiValue = midiValue;
  if (midiValue > 127) midiValue = 127;
  uint16_t position = map(midiValue, 0, 127, 0, 1023);
  ad5292Write(0b000001, position);
}
