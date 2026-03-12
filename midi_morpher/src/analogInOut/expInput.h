#pragma once
#include "../config.h"
#include "../potentiometers/analogReadHelpers.h"

void initExpInput() {
  pinMode(EXP_IN, INPUT);
  uint16_t medianRead = readPotMedian(EXP_IN);
}

void handleExpInput() {
  readPotMedian
}