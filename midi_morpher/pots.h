#ifndef POTS_H
#define POTS_H
#include "config.h"
#include "pedalState.h"

inline const uint16_t potDeadband = 10;

struct AnalogPot
{
    uint8_t pin;
    const char *name;
    uint16_t lastValue;
    uint16_t lastValueMidi;
    float filtered = 0;
};

inline AnalogPot analogPots[] = {
    {POT1_PIN, "UP Speed", 0, 0},
    {POT2_PIN, "Down Speed", 0, 0}};

inline void initAnalogPots()
{
    for (int i = 0; i < 2; i++)
    {
        pinMode(analogPots[i].pin, INPUT);
    }
}

inline void handleAnalogPot(AnalogPot &pot, void (*displayCallback)(String, uint16_t, uint8_t))
{
    const float alpha = 0.18f;

    uint16_t raw = analogRead(pot.pin);

    // exponential smoothing
    pot.filtered += alpha * (raw - pot.filtered);
    uint16_t smooth = (uint16_t)pot.filtered;

    // deadband check
    if (abs((int)smooth - (int)pot.lastValue) > potDeadband)
    {
        pot.lastValue = smooth;

        // clamp to ADC range
        if (smooth > 4095)
            smooth = 4095;

        // precise 0–127 scaling
        uint8_t scaled = (smooth * 128UL) / 4096;

        displayCallback(
            String(pot.name),
            smooth,
            scaled);
    }
}
// inline void handleAnalogPot(AnalogPot &pot, SignalCallback cb)
// {
//     const float alpha = 0.18f; // smaller = smoother, larger = faster

//     uint16_t raw = analogRead(pot.pin);

//     // exponential smoothing
//     pot.filtered += alpha * (raw - pot.filtered);

//     uint16_t smooth = (uint16_t)pot.filtered;

//     // deadband against filtered value
//     if (abs((int)smooth - (int)pot.lastValue) > potDeadband)
//     {
//         pot.lastValue = smooth;

//         cb(
//             String(pot.name),
//             smooth,
//             String(map(smooth, 0, 4095, 0, 127)));
//     }
// }

// inline void handleAnalogPot(AnalogPot &pot, SignalCallback cb)
// {
//     uint16_t value = analogRead(pot.pin);

//     if (abs(value - pot.lastValue) > potDeadband)
//     {
//         pot.lastValue = value;
//         cb(String(pot.name), value, String(map(value, 0, 4095, 0, 127)));
//     }
// }

#endif