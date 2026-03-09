#include "encoder.h"
#include <Arduino.h>

// Define global variables
volatile int encoderPos = 0;
int lastEncoderPos = 0;
volatile uint8_t encoderLastEncoded = 0;
volatile int8_t encoderStepAccumulator = 0;

// Define ISR
void IRAM_ATTR encoderISR()
{
    uint8_t MSB = digitalRead(ENC_A);
    uint8_t LSB = digitalRead(ENC_B);
    uint8_t encoded = (MSB << 1) | LSB;
    uint8_t sum = (encoderLastEncoded << 2) | encoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
        encoderStepAccumulator++;

    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
        encoderStepAccumulator--;

    if (encoderStepAccumulator >= 4)
    {
        encoderPos++;
        encoderStepAccumulator = 0;
    }

    if (encoderStepAccumulator <= -4)
    {
        encoderPos--;
        encoderStepAccumulator = 0;
    }

    encoderLastEncoded = encoded;
}