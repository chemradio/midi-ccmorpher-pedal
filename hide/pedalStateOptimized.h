#ifndef PEDAL_STATE_H
#define PEDAL_STATE_H

#include <stdint.h>
#include <array>

// -------------------- Enums --------------------

enum class RampState : uint8_t
{
    Idle,
    Up,
    Down
};

enum class MidiMode : uint8_t
{
    ProgramChange,
    ControlChange
};

// -------------------- Switch --------------------

struct SwitchConfig
{
    uint8_t value = 0;
    MidiMode mode = MidiMode::ProgramChange;
};

struct SwitchRuntime
{
    bool pressed = false;
    bool activated = false;
};

// -------------------- Configuration --------------------

struct PedalConfig
{
    uint8_t midiChannel = 1;

    bool rampLinear = true;
    bool rampDirectionUp = true;
    bool hotSwitchLatching = false;
    bool settingsLocked = false;

    float rampMinSeconds = 0.1f;
    float rampMaxSeconds = 5.0f;
    float rampCurrentSeconds = 1.0f;

    static constexpr size_t SwitchCount = 6;
    std::array<SwitchConfig, SwitchCount> switches;
};

// -------------------- Runtime --------------------

struct PedalRuntime
{
    RampState rampState = RampState::Idle;
    uint8_t rampProgress = 0; // 0–127 MIDI

    std::array<SwitchRuntime, PedalConfig::SwitchCount> switches;
};

// -------------------- Unified State --------------------

struct PedalState
{
    PedalConfig config;
    PedalRuntime runtime;

    // ---- Derived helpers ----

    inline bool isRamping() const
    {
        return runtime.rampState != RampState::Idle;
    }

    inline bool isSwitchActive(size_t index) const
    {
        return runtime.switches[index].latched;
    }
};

#endif