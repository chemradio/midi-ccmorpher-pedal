#pragma once
#include "../config.h"
#include "../pedalState.h"
#include "../midiCCModulator.h"
#include "footswitchObject.h"

FSButton hotswitch = FSButton(HS_PIN, HS_LED, "Hotswitch", 0);
