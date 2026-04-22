#pragma once
#include "../config.h"
#include "../midiOut.h"

// ── Note-value table ─────────────────────────────────────────────────────────
// Used for both the ramp-speed display and converting note indices to ms.
// Index matches what gets packed into rampUpMs/rampDownMs low byte when
// CLOCK_SYNC_FLAG is set. Order: shortest → longest.
inline const char * const noteValueNames[NUM_NOTE_VALUES] = {
  "1/32T", "1/32",  "1/16T", "1/32.", "1/16",  "1/8T", "1/16.", "1/8",
  "1/4T",  "1/8.",  "1/4",   "1/2T",  "1/4.",  "1/2",  "1/2.",  "1/1",  "2/1"
};

// Multiplier relative to a quarter note. 1.0 = 1/4 note.
inline const float noteValueMul[NUM_NOTE_VALUES] = {
  1.0f/12.0f,  // 1/32T
  1.0f/8.0f,   // 1/32
  1.0f/6.0f,   // 1/16T
  3.0f/16.0f,  // 1/32.
  1.0f/4.0f,   // 1/16
  1.0f/3.0f,   // 1/8T
  3.0f/8.0f,   // 1/16.
  1.0f/2.0f,   // 1/8
  2.0f/3.0f,   // 1/4T
  3.0f/4.0f,   // 1/8.
  1.0f,        // 1/4
  4.0f/3.0f,   // 1/2T
  3.0f/2.0f,   // 1/4.
  2.0f,        // 1/2
  3.0f,        // 1/2.
  4.0f,        // 1/1
  8.0f         // 2/1
};

// ── MidiClock ────────────────────────────────────────────────────────────────
// Internal/external MIDI clock generator and tap tempo tracker.
// 24 pulses per quarter note. Tempo LED pulses on the beat (50 ms flash).
struct MidiClock {
  float         bpm            = DEFAULT_BPM;
  bool          externalSync   = false;
  bool          ledEnabled     = true;
  bool          clockGenerate  = true;
  bool          clockOutput    = true;
  unsigned long tickIntervalUs = (unsigned long)(60000000.0f / (DEFAULT_BPM * 24.0f));
  unsigned long lastTickUs     = 0;
  unsigned long lastExternalMs = 0;
  unsigned long lastExtPulseUs = 0;
  uint8_t       pulseCount     = 0;   // 0–23, advances each 0xF8
  unsigned long ledOnMs        = 0;

  // Tap tempo averaging window (last N taps).
  static constexpr uint8_t TAP_HISTORY = 4;
  unsigned long tapHistory[TAP_HISTORY] = {0};
  uint8_t       tapIdx   = 0;
  uint8_t       tapCount = 0;

  void setBpm(float b) {
    if(b < BPM_MIN) b = BPM_MIN;
    if(b > BPM_MAX) b = BPM_MAX;
    bpm = b;
    tickIntervalUs = (unsigned long)(60000000.0f / (bpm * 24.0f));
  }

  // Advance beat counter and flash the tempo LED on the downbeat.
  void onPulse() {
    if(pulseCount == 0 && ledEnabled) {
      digitalWrite(TEMPO_LED_PIN, HIGH);
      ledOnMs = millis();
      if(ledOnMs == 0) ledOnMs = 1;   // 0 is the "LED off" sentinel
    }
    if(++pulseCount >= 24) pulseCount = 0;
  }

  // Call once per main loop iteration.
  void tick() {
    unsigned long nowUs = micros();
    unsigned long nowMs = millis();

    // Revert to internal clock if no external pulses for N ms.
    if(externalSync && (nowMs - lastExternalMs) > CLOCK_SYNC_TIMEOUT_MS) {
      externalSync = false;
      pulseCount   = 0;
      lastTickUs   = nowUs;
    }

    // Internal clock generator — only runs when not externally synced.
    if(!externalSync) {
      if(nowUs - lastTickUs >= tickIntervalUs) {
        lastTickUs += tickIntervalUs;
        // If we fell far behind (blocking display/i2c call), resync.
        if(nowUs - lastTickUs > tickIntervalUs * 4) lastTickUs = nowUs;
        if(clockGenerate && clockOutput) sendClockTick();
        onPulse();
      }
    }

    // Tempo LED auto-off after 50 ms.
    if(ledOnMs != 0 && (nowMs - ledOnMs) > 50) {
      digitalWrite(TEMPO_LED_PIN, LOW); // always off — ledEnabled only gates HIGH
      ledOnMs = 0;
    }
  }

  // External 0xF8 pulse received (from DIN in or USB in).
  // Switches to external sync and estimates BPM from pulse spacing.
  void receiveClock() {
    unsigned long nowMs = millis();
    unsigned long nowUs = micros();

    if(!externalSync) {
      externalSync    = true;
      pulseCount      = 0;
      lastExtPulseUs  = 0;       // reset BPM estimate on (re)sync
    }

    if(lastExtPulseUs != 0) {
      unsigned long delta = nowUs - lastExtPulseUs;
      if(delta > 0) {
        float newBpm = 60000000.0f / (delta * 24.0f);
        if(newBpm >= BPM_MIN && newBpm <= BPM_MAX) {
          // EMA smoothing — tracks fast enough without jitter.
          bpm = bpm * 0.9f + newBpm * 0.1f;
        }
      }
    }
    lastExtPulseUs = nowUs;
    lastExternalMs = nowMs;
    onPulse();
  }

  // Tap tempo — call on each TapTempo footswitch press.
  void receiveTap() {
    unsigned long now = millis();

    // Reset if taps are too far apart.
    if(tapCount > 0) {
      unsigned long lastTap = tapHistory[(tapIdx + TAP_HISTORY - 1) % TAP_HISTORY];
      if((now - lastTap) > 2000) {
        tapCount = 0;
        tapIdx   = 0;
      }
    }
    tapHistory[tapIdx] = now;
    tapIdx = (tapIdx + 1) % TAP_HISTORY;
    if(tapCount < TAP_HISTORY) tapCount++;

    if(tapCount >= 2) {
      unsigned long oldest = tapHistory[(tapIdx + TAP_HISTORY - tapCount) % TAP_HISTORY];
      unsigned long newest = tapHistory[(tapIdx + TAP_HISTORY - 1) % TAP_HISTORY];
      unsigned long span   = newest - oldest;
      if(span > 0) {
        float newBpm = 60000.0f * (float)(tapCount - 1) / (float)span;
        if(newBpm >= BPM_MIN && newBpm <= BPM_MAX) {
          setBpm(newBpm);
          externalSync = false;
          pulseCount   = 0;
          lastTickUs   = micros();
        }
      }
    }
  }

  // Convert CLOCK_SYNC_FLAG | noteIdx → milliseconds at current BPM.
  // A quarter note at 120 BPM = 500 ms.
  unsigned long syncToMs(uint32_t rampRaw) const {
    uint8_t idx = rampRaw & 0xFF;
    if(idx >= NUM_NOTE_VALUES) idx = NUM_NOTE_VALUES - 1;
    float quarterMs = 60000.0f / bpm;
    float ms        = quarterMs * noteValueMul[idx];
    if(ms < 1.0f)    ms = 1.0f;
    if(ms > 30000.0f) ms = 30000.0f;
    return (unsigned long)ms;
  }
};

inline MidiClock midiClock;
