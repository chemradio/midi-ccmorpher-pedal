# MIDI Morpher

A MIDI controller pedal for musicians.

---

### A midi-controller pedal with 4 footswitches (FS) with MIDI Out.

The key standout feature of the pedal is ability to send modulated MIDI CC messages. The output is universal: mini-TRS MIDI Out, USB-MIDI and analog expression out.

Additionaly the pedal also supports all basic MIDI functionality like sending PC, CC and Note messages.

---

Any footswitch can be customized to control internal MIDI modulation engine with different types of modulation. Each footswitch can be momentary or latching and trigger different MIDI modulation/morphing functions.

Two onboard potentiometers control the speed of modulation. One for UP and one for DOWN directions. These also work for LFO and STEPPER modes. In LFO mode the raise is controled by the UP speed pot, the descent is controlled by DOWN speed pot.

Pedal's output MIDI Channel is selected by turning the encoder without pressing or holding any footswitches.

#### Modulator/Morpher types

1. RAMPER: sends continuous MIDI CC messages from 0 to 127 and back to 0.
    - momentary: on press send 0 to 127 ramp, on release - go back to 0
    - latching: on press send 0 to 127 ramp, on release - do nothing, on next press - go back to 0
2. RAMPER Inverted: same as RAMPER but the resting position is 127 instead of 0.
3. STEPPER: similar to RAMPER but goes in distinct steps to emulate quantized robotic feel.
4. STEPPER Inverted: same as STEPPER but the resting position is 127 instead of 0.
5. RANDOM STEPPER: same as STEPPER but step values are picked randomly to emulate robotic or glitchy feel. Instead of goind UP until reaching 127 the modulator does not stop - it keeps sending random CC values.
6. LFO: sends continuous MIDI CC messages from 0 to 127 and back to 0. Has three modes that change the wave type: triangle, sine and square.
    - momentary: on press start modulation, on release snap to 0 and stop modulation.
    - latching: on press start modulation, on release - do nothing, on next press - go back to 0 and stop modulation.

Modes are toggled by holding the footswitch and pressing encoder button at the same time. Turning the encoder while in Modulation mode selects the MIDI CC Number for modulation.

#### Basic functionality

1. MIDI PC: user can specify the PC number. Momentary only.
2. MIDI CC: user can spedify the CC number. Momentary or latching.
    - momentary: holding FS sends 127, releasing send 0
    - latching: each step alternates between 0 and 127
3. MIDI Note: user specifies the note. Momentary only. On press sends Note On, on release Note Off.
4. Scene/Snapshot: guitar modeller specific scene/snapshot selector. Allows to pick one CC to trigger specific set. Like sending MIDI CC69 with value 3 to trigger Snapshot 3 on Line6 Helix units.

Turning the encoder while in basic modes selects the PC, CC and NOTE numbers.

---

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
- Helix Snapshots (CC69 with values 0-7) encoder picks CC value
- Quad Cortex Scenes (CC43 with values 0-7) encoder picks CC value
- Fractal Scenes (CC34 with values 0-7) encoder picks CC value
- Kemper Slots (CC50-CC54 with value 1) encoder picks CC number instead of value in this mode

### Inputs, outputs, additional controls

- mini-TRS MIDI output and input. Input is used to serve as a MIDI Thru for mirroring/daisy-chaining other MIDI Controllers.
- Separate external expression pedal input and output. All modulation effects are sent to this output. The emulated resistance is achieved by internal digital potentiometer (AD5292-BRUZ-20). Expression pedal input for connecting external analog expression controller or pedal. If connected the pedal mirrors the expression pedal input to expression pedal output and by sending MIDI CC20 values.
- Additional 2 footswitches can be connected via external jack.
- LOCK switch can be engaged to prevent accidentaly changing encoder and pot values.

#### Internal components

- ESP32-S3-N16R8 Devboard is the brain
- SSD1306 OLED display
- Digital potentiometer AD5292-BRUZ-20 for expression output
-
