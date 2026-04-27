// Only include config.h — NOT encoder.h. encoder.h pulls in mainMenu.h →
// display.h, which would duplicate every static/inline function body in this TU.
#include "../config.h"
#include <Arduino.h>

volatile int encoderPos = 0;
int lastEncoderPos = 0;
volatile uint8_t encoderLastEncoded = 0;
volatile int8_t encoderStepAccumulator = 0;

void IRAM_ATTR encoderISR()
{
    uint8_t MSB = digitalRead(ENC_A);
    uint8_t LSB = digitalRead(ENC_B);
    uint8_t encoded = (MSB << 1) | LSB;
    uint8_t sum = (encoderLastEncoded << 2) | encoded;

    int8_t step = 0;
    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) step =  1;
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) step = -1;

    if (step != 0) {
        // Reset residual count when direction reverses so the first detent in
        // the new direction is not eaten by leftover counts from the previous one.
        if ((step > 0 && encoderStepAccumulator < 0) ||
            (step < 0 && encoderStepAccumulator > 0))
            encoderStepAccumulator = 0;

        encoderStepAccumulator += step;

        if (encoderStepAccumulator >= 4) {
            encoderPos++;
            encoderStepAccumulator = 0;
        } else if (encoderStepAccumulator <= -4) {
            encoderPos--;
            encoderStepAccumulator = 0;
        }
    }

    encoderLastEncoded = encoded;
}
