#ifndef FOOTSWITCH_OBJECT_H
#define FOOTSWITCH_OBJECT_H

struct FSButton
{
    uint8_t pin;
    uint8_t ledPin;
    const char *name;
    bool ledState = false;
    bool lastState = HIGH;
    unsigned long lastDebounce = 0;
    uint8_t lastCCValue = 0;

    bool activated = false;

    void init()
    {
        pinMode(pin, INPUT_PULLUP);
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW);
    }

    void setLED(bool state)
    {
        ledState = state;
        digitalWrite(ledPin, state ? HIGH : LOW);
    }
};

#endif