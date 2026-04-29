#pragma once
// ── FS Cursor Edit Menu ───────────────────────────────────────────────────────
// Depends on display, PedalState, FSButton, and mode table — always included
// from display.h after all those are declared.

enum class FSEditRow : uint8_t {
  CATEGORY = 0,
  CC_TRIGGER,   // Mom / Latch / Single
  RAMP_SHAPE,   // Exp / Lin / Sine
  MOD_DIR,      // Normal / Inverted
  MOD_TRIG,     // Momentary / Latching
  LFO_WAVE,     // Sine / Tri / Square
  SCENE_UNIT,   // Helix / QC / Fractal / Kemper
  SCENE_STYLE,  // Snap / Scroll
  PRESET_DIR,   // Up / Down / Num
  NUMBER,       // CC# / PC# / Note# / Scene# / Key / Cmd / Preset# / Multi
  CC_HI,        // Hi value (CC Mom/Latch)
  CC_LO,        // Lo value (CC Mom/Latch)
  CC_VAL,       // Value (CC Single)
  VELOCITY,     // Note velocity
  KB_MOD,       // Keyboard modifier
  RAMP_UP,      // Up speed
  RAMP_DN,      // Down speed
  CHANNEL,      // Per-FS MIDI channel
};
static constexpr uint8_t MAX_FS_EDIT_ROWS = 18;

inline uint8_t buildFSEditRows(const FSButton &btn, FSEditRow *rows) {
  uint8_t n = 0;
  rows[n++] = FSEditRow::CATEGORY;
  uint8_t mi = btn.modeIndex;
  if(mi == 1) {
    rows[n++] = FSEditRow::NUMBER;
  } else if(mi >= 2 && mi <= 4) {
    rows[n++] = FSEditRow::CC_TRIGGER;
    rows[n++] = FSEditRow::NUMBER;
    if(mi == 4) rows[n++] = FSEditRow::CC_VAL;
    else { rows[n++] = FSEditRow::CC_HI; rows[n++] = FSEditRow::CC_LO; }
  } else if(mi == 5) {
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::VELOCITY;
  } else if(mi >= 6 && mi <= 17) {
    rows[n++] = FSEditRow::RAMP_SHAPE;
    rows[n++] = FSEditRow::MOD_DIR;
    rows[n++] = FSEditRow::MOD_TRIG;
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::RAMP_UP;
    rows[n++] = FSEditRow::RAMP_DN;
  } else if(mi >= 18 && mi <= 23) {
    rows[n++] = FSEditRow::LFO_WAVE;
    rows[n++] = FSEditRow::MOD_TRIG;
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::RAMP_UP;
    rows[n++] = FSEditRow::RAMP_DN;
  } else if(mi >= 24 && mi <= 27) {
    rows[n++] = FSEditRow::MOD_DIR;
    rows[n++] = FSEditRow::MOD_TRIG;
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::RAMP_UP;
    rows[n++] = FSEditRow::RAMP_DN;
  } else if(mi >= 28 && mi <= 31) {
    rows[n++] = FSEditRow::MOD_DIR;
    rows[n++] = FSEditRow::MOD_TRIG;
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::RAMP_UP;
    rows[n++] = FSEditRow::RAMP_DN;
  } else if(mi >= 32 && mi <= 39) {
    rows[n++] = FSEditRow::SCENE_UNIT;
    rows[n++] = FSEditRow::SCENE_STYLE;
    rows[n++] = FSEditRow::NUMBER;
  } else if(mi == 41) {
    rows[n++] = FSEditRow::NUMBER;
  } else if(mi == 42) {
    rows[n++] = FSEditRow::NUMBER;
    rows[n++] = FSEditRow::KB_MOD;
    return n; // fsChannel stores modifier — skip CHANNEL
  } else if(mi == 43) {
    rows[n++] = FSEditRow::NUMBER;
  } else if(mi >= 44 && mi <= 46) {
    rows[n++] = FSEditRow::PRESET_DIR;
    if(mi == 46) rows[n++] = FSEditRow::NUMBER;
  }
  rows[n++] = FSEditRow::CHANNEL;
  return n;
}

inline const char* fsEditRowName(FSEditRow r, const FSButton &btn) {
  switch(r) {
    case FSEditRow::CATEGORY:    return "Mode";
    case FSEditRow::CC_TRIGGER:  return "Type";
    case FSEditRow::RAMP_SHAPE:  return "Shape";
    case FSEditRow::MOD_DIR:     return "Dir";
    case FSEditRow::MOD_TRIG:    return "Trig";
    case FSEditRow::LFO_WAVE:    return "Wave";
    case FSEditRow::SCENE_UNIT:  return "Unit";
    case FSEditRow::SCENE_STYLE: return "Style";
    case FSEditRow::PRESET_DIR:  return "Dir";
    case FSEditRow::NUMBER:
      if(btn.isNote)      return "Note#";
      if(btn.isPC)        return "PC#";
      if(btn.isScene)     return btn.isSceneScroll ? "Max" : "Scene#";
      if(btn.isKeyboard)  return "Key";
      if(btn.isSystem)    return "Cmd";
      if(btn.mode == FootswitchMode::PresetNum) return "Preset#";
      if(btn.mode == FootswitchMode::Multi)     return "Multi";
      return "CC#";
    case FSEditRow::CC_HI:    return "Hi";
    case FSEditRow::CC_LO:    return "Lo";
    case FSEditRow::CC_VAL:   return "Value";
    case FSEditRow::VELOCITY: return "Vel";
    case FSEditRow::KB_MOD:   return "Mod";
    case FSEditRow::RAMP_UP:  return "Up Spd";
    case FSEditRow::RAMP_DN:  return "Dn Spd";
    case FSEditRow::CHANNEL:  return "Ch";
    default: return "?";
  }
}

inline String fsEditRowValue(FSEditRow r, const FSButton &btn) {
  uint8_t mi = btn.modeIndex;
  switch(r) {
    case FSEditRow::CATEGORY: {
      uint8_t ci = categoryForModeIndex(mi);
      return String(modeCategories[ci].name);
    }
    case FSEditRow::CC_TRIGGER: {
      static const char *ts[] = {"Mom","Latch","Single"};
      uint8_t v = (mi >= 2 && mi <= 4) ? (mi - 2) : 0;
      return String(ts[v]);
    }
    case FSEditRow::RAMP_SHAPE: {
      static const char *ss[] = {"Exp","Lin","Sine"};
      uint8_t s = (mi >= 6 && mi <= 17) ? (mi-6)/4 : 0;
      return String(ss[s < 3 ? s : 0]);
    }
    case FSEditRow::MOD_DIR: {
      uint8_t d;
      if(mi >= 6  && mi <= 17) d = ((mi-6) /2)%2;
      else if(mi >= 24 && mi <= 27) d = ((mi-24)/2)%2;
      else                          d = ((mi-28)/2)%2;
      return String(d ? "Inv" : "Norm");
    }
    case FSEditRow::MOD_TRIG: {
      uint8_t t;
      if(mi >= 6  && mi <= 17) t = (mi-6) %2;
      else if(mi >= 18 && mi <= 23) t = (mi-18)%2;
      else if(mi >= 24 && mi <= 27) t = (mi-24)%2;
      else                          t = (mi-28)%2;
      return String(t ? "Latch" : "Mom");
    }
    case FSEditRow::LFO_WAVE: {
      static const char *ws[] = {"Sine","Tri","Sq"};
      uint8_t w = (mi >= 18 && mi <= 23) ? (mi-18)/2 : 0;
      return String(ws[w < 3 ? w : 0]);
    }
    case FSEditRow::SCENE_UNIT: {
      static const char *us[] = {"Helix","QC","Fractal","Kemper"};
      uint8_t u = (mi >= 32 && mi <= 39) ? (mi-32)/2 : 0;
      return String(us[u < 4 ? u : 0]);
    }
    case FSEditRow::SCENE_STYLE:
      return String((mi >= 32 && mi <= 39) && (mi-32)%2 ? "Scroll" : "Snap");
    case FSEditRow::PRESET_DIR: {
      static const char *pd[] = {"Up","Down","#"};
      uint8_t d = (mi >= 44 && mi <= 46) ? (mi-44) : 0;
      return String(pd[d < 3 ? d : 0]);
    }
    case FSEditRow::NUMBER: {
      if(btn.isModSwitch && btn.midiNumber == PB_SENTINEL) return String(F("PB"));
      if(btn.isNote) {
        static const char *nn[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        return String(nn[btn.midiNumber%12]) + String((int)(btn.midiNumber/12)-1);
      }
      if(btn.isPC)    return String(F("PC:"))  + String(btn.midiNumber+1);
      if(btn.isScene) return String(F("V:"))   + String(btn.midiNumber+1);
      if(btn.isKeyboard) {
        uint8_t ki = btn.midiNumber < NUM_HID_KEYS ? btn.midiNumber : 0;
        return String(hidKeys[ki].name);
      }
      if(btn.isSystem) {
        uint8_t si = btn.midiNumber < NUM_SYS_CMDS ? btn.midiNumber : 0;
        return String(systemCommands[si].name);
      }
      if(btn.mode == FootswitchMode::PresetNum) return String(btn.midiNumber+1);
      if(btn.mode == FootswitchMode::Multi) {
        uint8_t si = btn.midiNumber;
        return (si < MAX_MULTI_SCENES && multiScenes[si].name[0] != '\0')
          ? String(multiScenes[si].name) : String(F("?"));
      }
      return String(F("CC:")) + String(btn.midiNumber+1);
    }
    case FSEditRow::CC_HI:    return String(btn.ccHigh);
    case FSEditRow::CC_LO:    return String(btn.ccLow);
    case FSEditRow::CC_VAL:   return String(btn.ccHigh);
    case FSEditRow::VELOCITY: return String(btn.velocity);
    case FSEditRow::KB_MOD: {
      uint8_t m = (btn.fsChannel == 0xFF) ? 0 : btn.fsChannel;
      if(!m) return String(F("None"));
      String s;
      if(m & KEY_MOD_CTRL)  s += 'C';
      if(m & KEY_MOD_SHIFT) s += 'S';
      if(m & KEY_MOD_ALT)   s += 'A';
      if(m & KEY_MOD_GUI)   s += 'G';
      return s;
    }
    case FSEditRow::RAMP_UP:
    case FSEditRow::RAMP_DN: {
      uint32_t raw = (r == FSEditRow::RAMP_UP) ? btn.rampUpMs : btn.rampDownMs;
      if(raw & CLOCK_SYNC_FLAG) {
        uint8_t ni = (uint8_t)(raw & 0xFF);
        if(ni >= NUM_NOTE_VALUES) ni = NUM_NOTE_VALUES-1;
        return String(noteValueNames[ni]);
      }
      char buf[10];
      snprintf(buf, sizeof(buf), "%.2fs", raw/1000.0f);
      return String(buf);
    }
    case FSEditRow::CHANNEL:
      return (btn.fsChannel == 0xFF) ? String(F("Glb")) : String(btn.fsChannel+1);
    default: return String();
  }
}

// Sync loadActionEditBtn back to extra action or persistent load action.
// No-op for PRESS edits (fsEditExtraType < 0 and fsEditFSIdx < FS_LOAD_ACTION_IDX).
inline void _syncFSEditTarget(PedalState &pedal) {
  const FSButton &lb = pedal.loadActionEditBtn;
  if(pedal.fsEditExtraType >= 0 && pedal.fsEditFSIdx < FS_LOAD_ACTION_IDX) {
    FSAction &act = pedal.buttons[pedal.fsEditFSIdx].extraActions[(uint8_t)pedal.fsEditExtraType];
    act.modeIndex  = lb.modeIndex;  act.midiNumber = lb.midiNumber;
    act.fsChannel  = lb.fsChannel;  act.ccLow      = lb.ccLow;
    act.ccHigh     = lb.ccHigh;     act.velocity   = lb.velocity;
    act.rampUpMs   = lb.rampUpMs;   act.rampDownMs = lb.rampDownMs;
    act.enabled    = (lb.modeIndex != 0);
  } else if(pedal.fsEditFSIdx == FS_LOAD_ACTION_IDX) {
    FSActionPersisted &la = pedal.liveLoadAction;
    la.enabled    = (lb.modeIndex != 0);  la.modeIndex  = lb.modeIndex;
    la.midiNumber = lb.midiNumber;        la.fsChannel  = lb.fsChannel;
    la.ccLow      = lb.ccLow;            la.ccHigh     = lb.ccHigh;
    la.velocity   = lb.velocity;         la.rampUpMs   = lb.rampUpMs;
    la.rampDownMs = lb.rampDownMs;       la._pad       = 0;
    markStateDirty();
  }
}

inline void displayFSEditMenu(PedalState &pedal) {
  displayMode = DISPLAY_PARAM;
  lastInteraction = millis();

  const FSButton &btn = pedal.fsEditTarget();
  FSEditRow rows[MAX_FS_EDIT_ROWS];
  uint8_t rowCount = buildFSEditRows(btn, rows);
  if(pedal.fsEditCursor >= rowCount) pedal.fsEditCursor = rowCount - 1;

  display.clearDisplay();
  display.invertDisplay(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Title bar
  display.setCursor(0, 0);
  uint8_t fi = pedal.fsEditFSIdx;
  if(fi == FS_LOAD_ACTION_IDX) {
    display.print(F("Load Action"));
  } else {
    display.print(pedal.buttons[fi].name);
    if(pedal.fsEditExtraType == 0)      display.print(F(" Hold"));
    else if(pedal.fsEditExtraType == 1) display.print(F(" Dbl"));
    else if(pedal.fsEditExtraType == 2) display.print(F(" Rel"));
  }
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  // Scrolling window: show 5 rows
  int8_t start = (int8_t)pedal.fsEditCursor - 2;
  if(start < 0) start = 0;
  if(start > (int8_t)rowCount - 5) start = (rowCount >= 5) ? (int8_t)(rowCount - 5) : 0;

  for(uint8_t i = 0; i < 5; i++) {
    uint8_t ri = (uint8_t)start + i;
    if(ri >= rowCount) break;
    FSEditRow row = rows[ri];
    bool isSel  = (ri == pedal.fsEditCursor);
    bool isEdit = isSel && pedal.fsEditEditing;
    int y = 11 + (int)i * 10;

    display.setCursor(0, y);
    display.print(isSel ? '>' : ' ');
    display.print(fsEditRowName(row, btn));

    String val = fsEditRowValue(row, btn);
    if(val.length() == 0) continue;
    if(val.length() > 7) val = val.substring(0, 7);
    int vx = 128 - (int)val.length() * 6;

    if(isEdit) {
      // Highlight value being edited
      display.fillRect(vx - 1, y - 1, (int)val.length() * 6 + 2, 9, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(vx, y);
      display.print(val);
      display.setTextColor(SSD1306_WHITE);
    } else {
      display.setCursor(vx, y);
      display.print(val);
    }
  }

  display.setCursor(0, 56);
  display.print(pedal.fsEditEditing ? F("Turn=val  Press=OK") : F("Turn=sel  Press=edit"));
  display.display();
}
