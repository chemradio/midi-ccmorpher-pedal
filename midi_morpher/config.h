// config.h - Pin definitions and constants

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Footswitch pins
#define FS1_PIN 4
#define FS1_LED 5
#define FS2_PIN 6
#define FS2_LED 7
#define HS_PIN 15
#define HS_LED 16
#define EXTFS1_PIN 17
#define EXTFS1_LED 18
#define EXTFS2_PIN 8
#define EXTFS2_LED 3

#define NO_LED_PIN 255

// Encoder pins
#define ENC_A 9
#define ENC_B 10
#define ENC_BTN 11

// Analog inputs
#define POT1_PIN 1
#define POT2_PIN 2
#define EXP_IN 12

// Toggle switches
#define MS2_PIN 13  // Momentary/Latching
#define DIR_PIN 14  // Direction
#define LESW_PIN 21 // Linear/Exponential
#define LST_PIN 47  // Lock settings

// I2C Display
#define SDA_PIN 41
#define SCL_PIN 42

// Digital pot (X9C103S)
#define DIGIPOT_CS 38
#define DIGIPOT_INC 39
#define DIGIPOT_UD 40

// MIDI (Hardware Serial)
#define MIDI_TX 43
#define MIDI_RX 44

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

// Timing constants
#define DEBOUNCE_DELAY 80
#define DISPLAY_TIMEOUT 2000

// MIDI constants
#define NUM_FOOTSWITCHES 4
#define NUM_PROGRESS_LEDS 8

#endif
