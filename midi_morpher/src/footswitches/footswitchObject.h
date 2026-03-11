#pragma once
#include "../midiOut.h"
#include "../config.h"
#include "../midiCCModulator.h"
#include "../sharedTypes.h"
inline constexpr unsigned long LONG_PRESS_TIMEOUT = 2000;

// there are two options:
// 1. adding latching toggle to every switch
// 2. latching is programmed from "MODE SELECT" menu via encoder
// this kills the "latching trick workflow" to invert the modulation
// Below are the modulation modes of footswitches:

// enum for footswitch mode
enum class FootswitchMode
{
    MomentaryPC,
    LatchingCC,
    MomentaryCC,

    RamperUpMomentary,
    RamperUpLatching,
    RamperDownMomentary,
    RamerDownLatching,

    StepperUpMomentary,
    StepperUpLatching,
    StepperDownMomentary,
    StepperDownLatching,

    LfoMomentary,
    LfoLatching,

    RandomStepperMomentary,
    RandomStepperLatching,
};

struct ModeInfo
{
    FootswitchMode mode;
    bool isLatching;
    bool isPC;
    const char *name;
    ModulationType modMode;
};

static constexpr ModeInfo modes[] = {
    {FootswitchMode::MomentaryPC, false, true, "PC", ModulationType::NOMODULATION},
    {FootswitchMode::MomentaryCC, false, false, "CC", ModulationType::NOMODULATION},
    {FootswitchMode::LatchingCC, true, false, "CC L", ModulationType::NOMODULATION},
    //
    {FootswitchMode::RamperUpMomentary, false, false, "RampUp", ModulationType::RAMPER},
    {FootswitchMode::RamperUpLatching, true, false, "RampUp L", ModulationType::RAMPER},
    {FootswitchMode::RamperDownMomentary, false, false, "RampDown", ModulationType::RAMPER},
    {FootswitchMode::RamerDownLatching, true, false, "RampDown L", ModulationType::RAMPER},
    //
    {FootswitchMode::StepperUpMomentary, false, false, "StepUp", ModulationType::STEPPER},
    {FootswitchMode::StepperUpLatching, true, false, "StepUp L", ModulationType::STEPPER},
    {FootswitchMode::StepperDownMomentary, false, false, "StepDown", ModulationType::STEPPER},
    {FootswitchMode::StepperDownLatching, true, false, "StepDown L", ModulationType::STEPPER},
    //
    {FootswitchMode::LfoMomentary, false, false, "LFO", ModulationType::LFO},
    {FootswitchMode::LfoLatching, true, false, "LFO L", ModulationType::LFO},
    //
    {FootswitchMode::RandomStepperMomentary, false, false, "Random", ModulationType::RANDOM},
    {FootswitchMode::RandomStepperLatching, true, false, "Random L", ModulationType::RANDOM}};

struct FSButton
{
    // unique fields
    uint8_t pin;
    uint8_t ledPin;
    const char *name;

    // defaults
    bool ledState = false;
    bool lastState = HIGH;
    unsigned long lastDebounce = 0;
    unsigned long holdTime = 0;
    uint8_t lastCCValue = 0;
    bool isPressed = false;
    bool isLatching = false;
    bool isActivated = false;
    bool isPC = true;
    uint8_t midiNumber = 0;
    uint8_t modeIndex = 0;
    bool isModSwitch = false;

    FootswitchMode mode = FootswitchMode::MomentaryPC;

    // constructor
    FSButton(uint8_t p, uint8_t lp, const char *n, uint8_t mN) : pin(p), ledPin(lp), name(n), midiNumber(mN) {}

    // shared methods
    void init()
    {
        pinMode(pin, INPUT_PULLUP);
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW);
    }

    void _setLED(bool state)
    {
        ledState = state;
        digitalWrite(ledPin, state ? HIGH : LOW);
    }

    void _handleLatching(uint8_t midiChannel, bool reading, void (*displayCallback)(FSButton &) = nullptr)
    {
        if (isPC || reading)
        {
            return;
        }
        isActivated = !isActivated;
        _setLED(isActivated);
        if (displayCallback)
        {
            displayCallback(*this);
        }
        sendMIDI(midiChannel, isPC, midiNumber, isActivated ? 127 : 0);
    }

    void _handleMomentary(uint8_t midiChannel, bool reading, void (*displayCallback)(FSButton &) = nullptr)
    {
        if (isPC)
        {
            if (!reading)
            {
                _setLED(true);
                sendMIDI(midiChannel, true, midiNumber);
                if (displayCallback)
                {
                    displayCallback(*this);
                }
            }
            else
            {
                _setLED(false);
                if (displayCallback)
                {
                    displayCallback(*this);
                }
            }
        }
        else if (!isPC) // cc
        {
            if (!reading)
            {
                _setLED(true);
                isActivated = true;
                sendMIDI(midiChannel, isPC, midiNumber, 127);
                if (displayCallback)
                {
                    displayCallback(*this);
                }
            }
            else
            {
                _setLED(false);
                isActivated = false;
                sendMIDI(midiChannel, isPC, midiNumber, 0);
                if (displayCallback)
                {
                    displayCallback(*this);
                }
            }
        }
    }

    bool _handleDebounce()
    {
        // debounce
        if ((millis() - lastDebounce) < DEBOUNCE_DELAY)
        {
            return false;
        }
        return true;
    }

    bool readState()
    {
        if (!_handleDebounce())
            return lastState;

        bool reading = digitalRead(pin);
        return (reading == LOW);
    }

    void handleFootswitch(uint8_t midiChannel, MidiCCModulator &modulator, void (*displayCallback)(FSButton &) = nullptr)
    {
        if (!_handleDebounce())
            return;
        bool reading = digitalRead(pin);
        isPressed = (reading == LOW);

        if (reading != lastState)
        {
            lastDebounce = millis();
            lastState = reading;

            if (isModSwitch)
            {
                // every hotswitch has it's own params for modulation
                // it requires reconfiguring the modulator each time
                modulator.modType = mode.modType;
                modulator.latching = isLatching;

                if (isPressed)
                {
                    modulator.press();
                }
                else
                {
                    modulator.release();
                }
                return;
            }
            else
            {
                if (isLatching)
                    _handleLatching(midiChannel, reading, displayCallback);
                else
                    _handleMomentary(midiChannel, reading, displayCallback);
            }
        }
    }

    const char *toggleFootswitchMode()
    {
        modeIndex = (modeIndex + 1) % 15;

        mode = modes[modeIndex].mode;
        isLatching = modes[modeIndex].isLatching;
        isPC = modes[modeIndex].isPC;

        return modes[modeIndex].name;

        // // three modes total: momentary PC, latching CC, momentary CC, need to cycle through
        // switch (mode)
        // {
        // case FootswitchMode::MomentaryPC:
        //     mode = FootswitchMode::LatchingCC;
        //     isLatching = true;
        //     isPC = false;
        //     return "latchCC";
        // case FootswitchMode::LatchingCC:
        //     mode = FootswitchMode::MomentaryCC;
        //     isLatching = false;
        //     isPC = false;
        //     return "momCC";
        // case FootswitchMode::MomentaryCC:
        //     mode = FootswitchMode::MomentaryPC;
        //     isLatching = false;
        //     isPC = true;
        //     return "momPC";
        // }
        // return "unknown";
    }
};
