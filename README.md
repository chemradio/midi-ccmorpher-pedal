### Imagine a pedal that ramps MIDI CC up and down with a press of a single footswitch. And a lot of possible variations around this idea.

# MIDI Expression Morph Pedal - Specification

## Overview

Compact stompbox that creates smooth transitions between two MIDI CC values with configurable ramping. Supports both MIDI and analog expression pedal output with expression pedal passthrough.
Hardware Components

1x Footswitch (main control)
1x Mode Switch (latching) - selects momentary or latching footswitch behavior
1x Direction Switch (latching) - sets starting ramp direction (up/down)
1x Curve Switch (latching) - selects ramp curve (linear/exponential)
2x Potentiometers - independent control of ramp-up and ramp-down speeds
1x Expression Pedal Input (1/4" jack, analog)
1x Expression Pedal Output (1/4" jack, analog)
MIDI Output (USB, via Arduino Uno)
Power: USB 5V (Arduino Uno)

## Operation Modes

### Momentary Mode

Press: Initiates ramp (0→127 or 127→0 based on direction switch)
Release: Initiates opposite ramp from current position
Interrupted ramps: If footswitch released before ramp completes, opposing ramp starts from current CC value and returns to starting point. Ramp time is interpolated proportionally (e.g., if original ramp reached 64/127 in 2 seconds with 4-second total time, return ramp takes 2 seconds to go from 64→0)

### Latching Mode

First press/release: Initiates one ramp
Second press/release: Initiates opposite ramp from current position
Interrupted ramps: If second activation occurs before first ramp completes, opposing ramp starts from current CC value. Ramp time is interpolated proportionally to match the distance traveled

## MIDI Output

Sends smooth CC value transitions from 0 to 127 (or reverse)
Ramp speed controlled by dedicated potentiometers (separate for up/down)
Curve shape: linear or exponential (switch-selectable)

## Analog Output

Mirrors MIDI output as analog expression pedal-compatible signal (resistive output via digital potentiometer, emulating standard 10kΩ expression pedal)
Expression pedal passthrough when connected to input jack

## Expression Pedal Passthrough Logic

When connected: Input signal mirrors to both analog output and MIDI output
During ramp: Expression pedal input ignored until ramp completes
After ramp: Expression pedal ignored until its position changes
No position change: Pedal remains ignored, footswitch control continues

### Core Use Case: Hands-free parameter automation for synths, effects, and MIDI gear with smooth, configurable transitions between two states.
