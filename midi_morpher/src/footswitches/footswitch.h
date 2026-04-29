#pragma once
#include "modeTable.h"


struct FSButton {
  uint8_t pin;
  const char *name;

  bool ledState    = false;
  bool lastState   = HIGH;
  unsigned long lastDebounce = 0;
  unsigned long rampUpMs     = DEFAULT_RAMP_SPEED;
  unsigned long rampDownMs   = DEFAULT_RAMP_SPEED;
  bool isPressed   = false;
  bool isLatching  = false;
  bool isActivated = false;
  bool isPC        = true;
  bool isNote      = false;
  bool isScene     = false;
  bool isSceneScroll = false;
  bool isSystem    = false;
  bool isKeyboard  = false;
  uint8_t midiNumber = 0;
  uint8_t modeIndex  = 0;
  uint8_t scrollPos       = 0;
  uint8_t scrollLastSent  = 0;
  bool isModSwitch   = false;
  uint8_t fsChannel  = 0xFF;  // 0xFF=global; in Keyboard mode: KEY_MOD_* bitmask
  uint8_t ccLow      = 0;     // value sent on CC release / latch-off
  uint8_t ccHigh     = 127;   // value sent on CC press / latch-on
  uint8_t velocity   = 100;   // Note On velocity

  FootswitchMode mode = FootswitchMode::MomentaryPC;
  ModeInfo modMode = { FootswitchMode::MomentaryPC, false,true,false,false,false,false,
                       "PC",ModulationType::NOMODULATION,SHAPE_LINEAR,0,0,false,false,false,false };

  // Extra press actions: [0]=LONG [1]=DOUBLE [2]=RELEASE
  FSAction extraActions[FS_NUM_EXTRA];

  // Press state machine
  PressPhase    pressPhase  = PressPhase::IDLE;
  unsigned long pressStartMs = 0;
  unsigned long releaseMs    = 0;
  bool          longFired    = false;

  FSButton(uint8_t p, const char *n, uint8_t mN)
      : pin(p), name(n), midiNumber(mN) {}

  void init() {
    pinMode(pin, INPUT_PULLUP);
  }

  void _setLED(bool state) { ledState = state; }

  uint8_t effectiveChannel(uint8_t globalChannel) const {
    return (fsChannel == 0xFF) ? globalChannel : fsChannel;
  }

  void _applyPressState(bool pressed, uint8_t ch, MidiCCModulator &modulator,
                        void (*displayCallback)(FSButton &)) {
    if(mode == FootswitchMode::NoAction) return;
    if(mode == FootswitchMode::TapTempo) {
      if(pressed) midiClock.receiveTap();
      _setLED(pressed);
      return;
    }

    if(mode == FootswitchMode::PresetUp || mode == FootswitchMode::PresetDown) {
      if(pressed) presetNavRequest = (mode == FootswitchMode::PresetUp) ? +1 : -1;
      _setLED(pressed);
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(mode == FootswitchMode::PresetNum) {
      if(pressed) presetNavDirect = (int8_t)(midiNumber < NUM_PRESETS ? midiNumber : NUM_PRESETS - 1);
      _setLED(pressed);
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(isKeyboard) {
      if(pressed) sendKeyDown(midiNumber, fsChannel == 0xFF ? 0 : fsChannel);
      else        sendKeyUp();
      isActivated = pressed;
      _setLED(pressed);
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(isSystem) {
      _setLED(pressed);
      if(pressed) sendSystemCommand(midiNumber);
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(mode == FootswitchMode::Multi) {
      _setLED(pressed);
      uint8_t si = midiNumber;
      if(si < MAX_MULTI_SCENES && multiScenes[si].name[0] != '\0') {
        const MultiScene &sc = multiScenes[si];
        for(uint8_t s = 0; s < sc.count && s < MAX_MULTI_SUBCMDS; s++) {
          const MultiSubCmd &sub = sc.subCmds[s];
          if(sub.modeIndex >= NUM_MODES) continue;
          const ModeInfo &mi = modes[sub.modeIndex];
          uint8_t eff = (sub.channel == 0xFF) ? ch : sub.channel;
          if(mi.isPC)        { if(pressed) sendMIDI(eff, true, sub.midiNumber); }
          else if(mi.isNote) { sendNote(eff, sub.midiNumber, pressed); }
          else if(mi.isScene) {
            if(pressed) {
              if(mi.scenePickCC) sendMIDI(eff, false, mi.sceneCC + sub.midiNumber, 1);
              else               sendMIDI(eff, false, mi.sceneCC, sub.midiNumber);
            }
          }
          else if(mi.isSystem) { if(pressed) sendSystemCommand(sub.midiNumber); }
          else { sendMIDI(eff, false, sub.midiNumber, pressed ? 127 : 0); }
        }
      }
      if(displayCallback) displayCallback(*this);
      return;
    }

    if(isModSwitch) {
      modulator.modType        = getModulationType(mode);
      modulator.latching       = isLatching;
      modulator.rampShape      = modMode.shape;
      if(modMode.isInverted)   modulator.restingHigh = true;
      else                     modulator.restingHigh = false;
      ModDest newDest = (midiNumber == PB_SENTINEL) ? DEST_PITCHBEND : DEST_CC;
      if(modulator.destType != newDest) {
        modulator.destType     = newDest;
        modulator.currentValue = modulator.restVal();
        modulator.lastEmittedCC = 0xFF;
        modulator.lastEmittedPB = 0xFFFF;
      }
      modulator.destType       = newDest;
      modulator.rampUpTimeMs   = (rampUpMs  & CLOCK_SYNC_FLAG)
                                   ? midiClock.syncToMs(rampUpMs)
                                   : (unsigned long)rampUpMs;
      modulator.rampDownTimeMs = (rampDownMs & CLOCK_SYNC_FLAG)
                                   ? midiClock.syncToMs(rampDownMs)
                                   : (unsigned long)rampDownMs;
      modulator.midiChannel    = ch;
      modulator.midiCCNumber   = (newDest == DEST_PITCHBEND) ? 0 : midiNumber;
      pressed ? modulator.press() : modulator.release();
      _setLED(modulator.isActivated);
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isPC) {
      _setLED(pressed);
      if(pressed) sendMIDI(ch, true, midiNumber);
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isNote) {
      isActivated = pressed;
      _setLED(pressed);
      sendNote(ch, midiNumber, pressed, velocity);
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isScene) {
      _setLED(pressed);
      if(pressed) {
        if(isSceneScroll) {
          if(scrollPos > midiNumber) scrollPos = 0;
          if(modMode.scenePickCC) {
            sendMIDI(ch, false, modMode.sceneCC + scrollPos, 1);
          } else {
            sendMIDI(ch, false, modMode.sceneCC, scrollPos);
          }
          scrollLastSent = scrollPos;
          scrollPos = (scrollPos >= midiNumber) ? 0 : scrollPos + 1;
        } else if(modMode.scenePickCC) {
          sendMIDI(ch, false, modMode.sceneCC + midiNumber, 1);
        } else {
          sendMIDI(ch, false, modMode.sceneCC, midiNumber);
        }
      }
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(mode == FootswitchMode::SingleCC) {
      isActivated = pressed;
      _setLED(pressed);
      if(pressed) sendMIDI(ch, false, midiNumber, ccHigh);
      if(displayCallback) displayCallback(*this);
      return;
    }
    if(isLatching) {
      if(!pressed) return;
      isActivated = !isActivated;
      _setLED(isActivated);
      sendMIDI(ch, false, midiNumber, isActivated ? ccHigh : ccLow);
      if(displayCallback) displayCallback(*this);
      return;
    }
    isActivated = pressed;
    _setLED(pressed);
    sendMIDI(ch, false, midiNumber, pressed ? ccHigh : ccLow);
    if(displayCallback) displayCallback(*this);
  }

  // Fire the RELEASE extra action.
  // Always clears switchPressed first so press() is never blocked by prior state
  // (e.g. primary-mode modulation or an earlier release that left it set).
  void _fireReleaseAction(uint8_t ch, MidiCCModulator &modulator,
                          void (*displayCallback)(FSButton &)) {
    modulator.switchPressed = false;
    _applyExtraAction(2, true, ch, modulator, displayCallback);
    modulator.switchPressed = false;  // clear after so the next release also fires
  }

  void _applyExtraAction(uint8_t t, bool pressed, uint8_t ch,
                         MidiCCModulator &modulator,
                         void (*displayCallback)(FSButton &)) {
    if(t >= FS_NUM_EXTRA || !extraActions[t].enabled) return;
    const FSAction &act = extraActions[t];
    if(act.modeIndex >= NUM_MODES) return;
    const ModeInfo &mi = modes[act.modeIndex];
    uint8_t ach = (act.fsChannel == 0xFF) ? ch : act.fsChannel;

    // Save primary config
    uint8_t  sv_mi=modeIndex; FootswitchMode sv_mo=mode; ModeInfo sv_mm=modMode;
    uint8_t  sv_mn=midiNumber, sv_fc=fsChannel, sv_cl=ccLow, sv_cH=ccHigh, sv_vel=velocity;
    uint32_t sv_ru=rampUpMs, sv_rd=rampDownMs;
    bool sv_lat=isLatching, sv_pc=isPC, sv_nt=isNote, sv_ms=isModSwitch;
    bool sv_sc=isScene, sv_ss=isSceneScroll, sv_sy=isSystem, sv_kb=isKeyboard;

    modeIndex=act.modeIndex; midiNumber=act.midiNumber; fsChannel=act.fsChannel;
    ccLow=act.ccLow; ccHigh=act.ccHigh; velocity=act.velocity;
    rampUpMs=act.rampUpMs; rampDownMs=act.rampDownMs;
    mode=mi.mode; modMode=mi;
    isLatching=mi.isLatching; isPC=mi.isPC; isNote=mi.isNote;
    isModSwitch=mi.isModSwitch; isScene=mi.isScene; isSceneScroll=mi.isSceneScroll;
    isSystem=mi.isSystem; isKeyboard=mi.isKeyboard;

    _applyPressState(pressed, ach, modulator, displayCallback);

    modeIndex=sv_mi; mode=sv_mo; modMode=sv_mm;
    midiNumber=sv_mn; fsChannel=sv_fc; ccLow=sv_cl; ccHigh=sv_cH; velocity=sv_vel;
    rampUpMs=sv_ru; rampDownMs=sv_rd;
    isLatching=sv_lat; isPC=sv_pc; isNote=sv_nt; isModSwitch=sv_ms;
    isScene=sv_sc; isSceneScroll=sv_ss; isSystem=sv_sy; isKeyboard=sv_kb;
  }

  void handleFootswitch(uint8_t midiChannel, MidiCCModulator &modulator,
                        void (*displayCallback)(FSButton &) = nullptr) {
    unsigned long now = millis();
    const uint8_t ch  = effectiveChannel(midiChannel);
    bool hasLong    = extraActions[0].enabled && extraActions[0].modeIndex != 0;
    bool hasDouble  = extraActions[1].enabled && extraActions[1].modeIndex != 0;
    bool hasRelease = extraActions[2].enabled && extraActions[2].modeIndex != 0;

    // Debounced edge detection
    bool edgeDetected = false;
    if((now - lastDebounce) >= DEBOUNCE_DELAY) {
      bool reading = digitalRead(pin);
      if(reading != lastState) {
        lastDebounce = now;
        lastState    = reading;
        isPressed    = (reading == LOW);
        edgeDetected = true;
      }
    }

    if(edgeDetected) {
      if(isPressed) {
        // Press edge
        if(pressPhase == PressPhase::PENDING_DBL) {
          // Second press within double-tap window → fire DOUBLE
          pressPhase = PressPhase::DBL_ACTIVE;
          _applyExtraAction(1, true, ch, modulator, displayCallback);
        } else {
          pressStartMs = now;
          longFired    = false;
          if(!hasDouble && !hasLong) {
            _applyPressState(true, ch, modulator, displayCallback);
            pressPhase = PressPhase::PRESS_ACTIVE;
          } else {
            pressPhase = PressPhase::WAIT;
          }
        }
      } else {
        // Release edge
        switch(pressPhase) {
          case PressPhase::WAIT:
            if(hasDouble) {
              // Released before long or double — start double-tap window
              releaseMs  = now;
              pressPhase = PressPhase::PENDING_DBL;
            } else {
              // HOLD configured but released before timeout — fire PRESS now
              _applyPressState(true, ch, modulator, displayCallback);
              _applyPressState(false, ch, modulator, displayCallback);
              if(hasRelease) _fireReleaseAction(ch, modulator, displayCallback);
              pressPhase = PressPhase::IDLE;
            }
            break;
          case PressPhase::PRESS_ACTIVE:
            _applyPressState(false, ch, modulator, displayCallback);
            if(hasRelease) _fireReleaseAction(ch, modulator, displayCallback);
            pressPhase = PressPhase::IDLE;
            break;
          case PressPhase::LONG_ACTIVE:
            _applyExtraAction(0, false, ch, modulator, displayCallback);
            longFired  = false;
            pressPhase = PressPhase::IDLE;
            break;
          case PressPhase::DBL_ACTIVE:
            _applyExtraAction(1, false, ch, modulator, displayCallback);
            pressPhase = PressPhase::IDLE;
            break;
          default:
            pressPhase = PressPhase::IDLE;
            break;
        }
      }
    }

    // Long press detection (every tick while held)
    if(hasLong && !longFired && isPressed &&
       (pressPhase == PressPhase::WAIT || pressPhase == PressPhase::PRESS_ACTIVE)) {
      if((now - pressStartMs) >= FS_LONG_MS) {
        if(pressPhase == PressPhase::PRESS_ACTIVE)
          _applyPressState(false, ch, modulator, displayCallback);
        _applyExtraAction(0, true, ch, modulator, displayCallback);
        longFired  = true;
        pressPhase = PressPhase::LONG_ACTIVE;
      }
    }

    // Double-tap window timeout → treat as single PRESS
    if(pressPhase == PressPhase::PENDING_DBL && (now - releaseMs) >= FS_DBL_WIN_MS) {
      _applyPressState(true, ch, modulator, displayCallback);
      _applyPressState(false, ch, modulator, displayCallback);
      if(hasRelease) _fireReleaseAction(ch, modulator, displayCallback);
      pressPhase = PressPhase::IDLE;
    }
  }

  void simulatePress(bool pressed, uint8_t midiChannel, MidiCCModulator &modulator) {
    isPressed = pressed;
    _applyPressState(pressed, effectiveChannel(midiChannel), modulator, nullptr);
  }

  const char *toggleFootswitchMode(MidiCCModulator &modulator) {
    modeIndex = (modeIndex + 1) % NUM_MODES;
    const ModeInfo &m = modes[modeIndex];
    // fsChannel transition: entering keyboard resets to no-modifiers;
    // leaving keyboard restores global-channel sentinel
    if(!isKeyboard && m.isKeyboard) fsChannel = 0x00;
    else if(isKeyboard && !m.isKeyboard) fsChannel = 0xFF;
    mode        = m.mode;
    isModSwitch = m.isModSwitch;
    isLatching  = m.isLatching;
    isPC        = m.isPC;
    isNote      = m.isNote;
    isScene     = m.isScene;
    isSceneScroll = m.isSceneScroll;
    isSystem    = m.isSystem;
    isKeyboard  = m.isKeyboard;
    scrollPos      = 0;
    scrollLastSent = 0;
    modMode     = m;
    if(m.isInverted) modulator.setRestingHigh();
    else             modulator.setRestingLow();
    modulator.rampShape = m.shape;
    modulator.reset();
    isActivated = false;
    _setLED(false);
    return m.name;
  }
};

// ── Mode property helpers ─────────────────────────────────────────────────────

// Returns true if this mode needs a value-entry submenu (level 4) in mode select.
inline bool modeNeedsValueEntry(uint8_t modeIdx) {
  if(modeIdx >= NUM_MODES) return false;
  FootswitchMode m = modes[modeIdx].mode;
  return m != FootswitchMode::NoAction
      && m != FootswitchMode::TapTempo
      && m != FootswitchMode::PresetUp
      && m != FootswitchMode::PresetDown
      && m != FootswitchMode::Multi;
}

// Returns true if this mode also needs a velocity entry (level 5) — Note mode only.
inline bool modeNeedsVelocity(uint8_t modeIdx) {
  if(modeIdx >= NUM_MODES) return false;
  return modes[modeIdx].isNote;
}

// Returns true if this mode shows the Up/Down speed submenus (levels 6/7).
inline bool modeNeedsRampEntry(uint8_t modeIdx) {
  if(modeIdx >= NUM_MODES) return false;
  return modes[modeIdx].isModSwitch;
}

// ── Free mode-application helpers ────────────────────────────────────────────

inline void applyModeFlags(FSButton &btn, uint8_t idx) {
  if(idx >= NUM_MODES) return;
  const ModeInfo &m = modes[idx];
  // fsChannel is NOT touched here — caller (applyPreset) provides it from NVS.
  // Only user-initiated paths (applyModeIndex, toggleFootswitchMode) reset it.
  btn.modeIndex      = idx;
  btn.mode           = m.mode;
  btn.isModSwitch    = m.isModSwitch;
  btn.isLatching     = m.isLatching;
  btn.isPC           = m.isPC;
  btn.isNote         = m.isNote;
  btn.isScene        = m.isScene;
  btn.isSceneScroll  = m.isSceneScroll;
  btn.isSystem       = m.isSystem;
  btn.isKeyboard     = m.isKeyboard;
  btn.scrollPos      = 0;
  btn.scrollLastSent = 0;
  btn.modMode        = m;
  btn.isActivated    = false;
  btn.ledState       = false;
  if(!m.isModSwitch && !m.isKeyboard && btn.midiNumber == PB_SENTINEL)
    btn.midiNumber = 0;
}

inline void applyModeIndex(FSButton &btn, uint8_t idx, MidiCCModulator *mod = nullptr) {
  // User-initiated mode change: transition fsChannel (keyboard ↔ MIDI modes)
  const ModeInfo &nm = modes[idx];
  if(!btn.isKeyboard && nm.isKeyboard) btn.fsChannel = 0x00;
  else if(btn.isKeyboard && !nm.isKeyboard) btn.fsChannel = 0xFF;
  applyModeFlags(btn, idx);
  if(mod) {
    const ModeInfo &m = modes[idx];
    if(m.isInverted) mod->setRestingHigh();
    else             mod->setRestingLow();
    mod->rampShape = m.shape;
    mod->reset();
  }
}
