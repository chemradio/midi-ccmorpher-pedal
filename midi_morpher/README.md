MIDI Morpher. A MIDI controller pedal for musicians.

A midi-controller pedal with 4 footswitches (FS) with MIDI Out.

Basic functionality:

1. MIDI PC: user can specify the PC number. Momentary only.
2. MIDI CC: user can spedify the CC number. Momentary or latching.
    - momentary: holding FS sends 127, releasing send 0
    - latching: each step alternates between 0 and 127
3. MIDI Note: user specifies the note. Momentary only. On press sends Note On, on release Note Off.

The onboard encoder control selects the PC, CC and NOTE numbers.

MIDI-Morpher:
Footswitches can be used to control internal MIDI modulation engine in momentary or latching modes. Each footswitch can be customized to trigger various behavior of the modulator. Each footswitch can be momentary or latching and trigger different MIDI modulation/morphing functions.

Modulator/Morpher types:

1. RAMPER: sends continuous MIDI CC messages from 0 to 127 and back to 0.
    - momentary: on press send 0 to 127 ramp, on release - go back to 0
    - latching: on press send 0 to 127 ramp, on release - do nothing, on next press - go back to 0
2. RAMPER Inverted: same as RAMPER but the resting position is 127 instead of 0.
3. STEPPER: similar to RAMPER but goes in distinct steps to emulate quantized robotic feel.
4. STEPPER Inverted: same as STEPPER but the resting position is 127 instead of 0.
5. RANDOM STEPPER: same as STEPPER but step values are picked randomly to emulate robotic or glitchy feel.
6. LFO: sends continuous MIDI CC messages from 0 to 127 and back to 0. Has three modes that change the wave type: triangle, sinusoid and square.
    - momentary: on press start 0 to 127 MIDI LFO, on release snap to 0
    - latching: on press start 0 to 127 MIDI LFO, on release - do nothing, on next press - go back to 0

Modes are toggled by holding the footswitch and pressing encoder button. The onboard encoder control selects the MIDI CC Number for modulation.
Two onboard potentiometers control the speed of modulator - one for upward direction and the other is for downward. These also work for LFO nd RANDOM STEPPER modes. In LFO mode the raise is controled by the UP speed pot, the descent is controlled by DOWN speed pot.

Total modes list in order of appearance in menu:

- PC
- CC momentary
- CC latching
- NOTE
- RAMPER momentary
- RAMPER latching
- RAMPER inverted momentary
- RAMPER inverted latching
- LFO Sine-wave momentary
- LFO Sine-wave latching
- LFO Triangle-wave momentary
- LFO Triangle-wave latching
- LFO Square-wave momentary
- LFO Square-wave latching
- STEPPER momentary
- STEPPER latching
- STEPPER inverted momentary
- STEPPER inverted latching
- RANDOM STEPPER momentary
- RANDOM STEPPER latching
- RANDOM STEPPER inverted momentary
- RANDOM STEPPER inverted latching

The pedal works on ESP32-S3-N16R8 Devboard.

Other features:

- Expression pedal output via Digipot. All
- Expression pedal input for connecting external analog expression controller, pedal. If connected the pedal mirrors expression pedal input to
  and additional 2 footswitches via external jack. The pedal has both standard mini-TRS MIDI output and analog expression pedal output. In addition the pedal also has MIDI input which acts as a MIDI Thru for mirroring/daisy-chaining other MIDI Controllers. Additional expression pedal input is provided for manual control of the
