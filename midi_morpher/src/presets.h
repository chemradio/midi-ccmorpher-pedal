#pragma once
// Preset button handling and LED update functions.
// Included after statePersistence.h, pedalState.h, and display.h in the .ino.

// ── Preset LED blink state ────────────────────────────────────────────────────
// Slow blink on the active preset LED when presetDirty signals unsaved changes
// (except in conservative mode).
// Fast blink burst on save to confirm the write.
#define PRESET_DIRTY_BLINK_MS 700 // full period (on + off)
#define PRESET_SAVE_BLINK_MS 80   // full period during save confirmation
#define PRESET_SAVE_BLINK_DUR 800 // total duration of save confirmation burst

inline unsigned long presetSaveBlinkUntil = 0;

inline void triggerPresetSaveBlink() {
  presetSaveBlinkUntil = millis() + PRESET_SAVE_BLINK_DUR;
}

// ── Combined footswitch actions for preset navigation ───────────────────────────
// FS1 + FS2 simultaneous press → preset down (previous preset)
// FS3 + FS4 simultaneous press → preset up (next preset)
// FS1 + FS4 held → save preset (long-press, like preset button long-press)

inline void handleCombinedActions(PedalState &pedal) {
  static uint8_t prevCombinedState = 0;
  static unsigned long lastCombinedMs = 0;
  static unsigned long combinedHoldStart = 0;
  static bool combinedHoldFired = false;

  uint8_t combined = 0;
  if(pedal.buttons[0].isPressed && pedal.buttons[1].isPressed)
    combined |= 0x01;
  if(pedal.buttons[2].isPressed && pedal.buttons[3].isPressed)
    combined |= 0x02;
  if(pedal.buttons[0].isPressed && pedal.buttons[3].isPressed)
    combined |= 0x04;

  unsigned long now = millis();
  if(now - lastCombinedMs < 300) {
    if((combined & 0x04) && !(prevCombinedState & 0x04)) {
      combinedHoldStart = now;
      combinedHoldFired = false;
    } else if((prevCombinedState & 0x04) && !(combined & 0x04)) {
      combinedHoldFired = false;
    }
    prevCombinedState = combined;
    return;
  }

  uint8_t lim = max((uint8_t)1, pedal.globalSettings.presetCount);
  if((combined & 0x01) && !(prevCombinedState & 0x01)) {
    uint8_t next = (activePreset == 0) ? (lim - 1) : (activePreset - 1);
    applyPreset(next, pedal);
    displayPresetLoad(activePreset);
    lastCombinedMs = now;
  } else if((combined & 0x02) && !(prevCombinedState & 0x02)) {
    uint8_t next = (activePreset + 1) % lim;
    applyPreset(next, pedal);
    displayPresetLoad(activePreset);
    lastCombinedMs = now;
  } else if((combined & 0x04) && !(prevCombinedState & 0x04)) {
    combinedHoldStart = now;
    combinedHoldFired = false;
  } else if((prevCombinedState & 0x04) && !(combined & 0x04)) {
    combinedHoldFired = false;
  }

  if((combined & 0x04) && !combinedHoldFired && (now - combinedHoldStart) >= FS_LONG_MS) {
    if(!pedal.settingsLocked) {
      saveCurrentPreset(pedal);
      triggerPresetSaveBlink();
      displayPresetSaved(activePreset);
    }
    combinedHoldFired = true;
    lastCombinedMs = now;
  }

  prevCombinedState = combined;
}
