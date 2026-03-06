# MIDI Morpher Pedal

## Overview

The pedal was first designed to replace the need for an expression pedal for musical gear. A single footswitch can send continuous MIDI CC ramps emulating a foot on a expression pedal. The ramp is fully configurable - the curve type can be set to linear or exponential, the ramp up and ramp down times can be adjusted with onboard controls. The footswitch can also be momentary (ramp up when pressed, ramp down on release) or latching.

Further down the road it grew in functionality. Two extra footswitches were added for convenience. Both can be independently reconfigured to serve as a MIDI PC trigger, MIDI CC sender with optional latching functionality.

I didn't want to complicate the setup process by introducing wireless interfaces. Just using onboard controls user can change footswitch modes, latching, MIDI Channel, MIDI CC and PC numbers, etc.

The project is based on a knockoff ESP32-S3-N16R8 DevBoard. In case someone want's to try it all of the connections are described in config.h file.

## MIDI Output

Both DIN and USB MIDI works fine. MIDI Input is to be added in later versions.

## Analog Output

Still in development: analog output jack mirrors MIDI output as analog expression pedal-compatible signal (resistive output via digital potentiometer, emulating standard 10kΩ expression pedal)
Expression pedal passthrough when connected to input jack
