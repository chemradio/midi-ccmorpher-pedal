#pragma once
#include "midiOut.h"
#include "config.h"

inline constexpr unsigned long LONG_PRESS_TIMEOUT = 2000;

// enum for footswitch mode
enum class FootswitchMode
{
    MomentaryPC,
    LatchingCC,
    MomentaryCC
};

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
    bool isLongPressActivated = false;
    bool isPC = true;
    uint8_t midiNumber = 0;
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
        if (reading)
        {
            return;
        }

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
        if (!reading)
        {
            _setLED(true);
            if (!isActivated)
            {
                if (displayCallback)
                {
                    displayCallback(*this);
                }
                sendMIDI(midiChannel, isPC, midiNumber, isPressed ? 127 : 0);
            }
            isActivated = true;
        }
        else
        {
            _setLED(false);
            isActivated = false;
            if (displayCallback)
            {
                displayCallback(*this);
            }
        }
    }

    void handleFootswitch(uint8_t midiChannel, void (*displayCallback)(FSButton &) = nullptr)
    {
        // debounce
        if ((millis() - lastDebounce) < DEBOUNCE_DELAY)
        {
            return;
        }

        bool reading = digitalRead(pin);
        isPressed = (reading == LOW);

        if (reading != lastState)
        {
            lastDebounce = millis();
            lastState = reading;

            if (isLatching)
                _handleLatching(midiChannel, reading, displayCallback);
            else
                _handleMomentary(midiChannel, reading, displayCallback);
        }
    }

    const char *toggleFootswitchMode()
    {
        // three modes total: momentary PC, latching CC, momentary CC, need to cycle through
        switch (mode)
        {
        case FootswitchMode::MomentaryPC:
            mode = FootswitchMode::LatchingCC;
            isLatching = true;
            isPC = false;
            return "latchCC";
        case FootswitchMode::LatchingCC:
            mode = FootswitchMode::MomentaryCC;
            isLatching = false;
            isPC = false;
            return "momCC";
        case FootswitchMode::MomentaryCC:
            mode = FootswitchMode::MomentaryPC;
            isLatching = false;
            isPC = true;
            return "momPC";
        }
        return "unknown";
    }
};
