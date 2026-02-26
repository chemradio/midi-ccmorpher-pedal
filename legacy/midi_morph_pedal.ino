/*
 * MIDI Expression Morph Pedal - CORRECTED VERSION
 * Smooth MIDI CC ramping with configurable modes
 * Supports both USB MIDI and 5-pin DIN MIDI
 */

// Pin definitions
const int FOOTSWITCH_PIN = 2;
const int MODE_SWITCH_PIN = 3;      // HIGH = Momentary, LOW = Latching
const int DIRECTION_SWITCH_PIN = 4; // HIGH = Start Up, LOW = Start Down
const int CURVE_SWITCH_PIN = 5;     // HIGH = Linear, LOW = Exponential
const int RAMP_UP_POT_PIN = A0;
const int RAMP_DOWN_POT_PIN = A1;

// MIDI settings
const int MIDI_CHANNEL = 1;   // Change this if needed (1-16)
const int MIDI_CC_NUMBER = 4; // Change to your desired CC number

// State variables
bool footswitchState = false;
bool lastFootswitchState = false;
bool latchState = false;
int currentValue = 0;
int targetValue = 0;
int rampStartValue = 0; // NEW: Store where the ramp started from
bool isRamping = false;
bool rampDirectionUp = true; // true = up, false = down
unsigned long lastRampTime = 0;
int rampDuration = 1000; // milliseconds
int minRampDuration = 0;
int maxRampDuration = 1500;

// Debounce - reduced for better responsiveness
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 10; // Reduced from 50ms to 10ms

// Update rate limiting for smoother MIDI output
unsigned long lastMidiSendTime = 0;
const unsigned long midiSendInterval = 5; // Send MIDI every 5ms max (200 messages/sec)

void setup()
{
  // Initialize pins
  pinMode(FOOTSWITCH_PIN, INPUT_PULLUP);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(DIRECTION_SWITCH_PIN, INPUT_PULLUP);
  pinMode(CURVE_SWITCH_PIN, INPUT_PULLUP);

  // Start hardware serial for 5-pin MIDI (31250 baud is MIDI standard)
  Serial.begin(31250);

  // Initialize starting position based on direction switch
  if (digitalRead(DIRECTION_SWITCH_PIN) == HIGH)
  {
    currentValue = 0;
    rampDirectionUp = true;
  }
  else
  {
    currentValue = 127;
    rampDirectionUp = false;
  }

  // Send initial MIDI value so the receiving device knows the starting state
  sendMIDI(currentValue);
}

void loop()
{
  // Read the direction switch to update the preferred starting direction
  // This only affects which direction we ramp when the footswitch is pressed
  // It does NOT reset the current value
  rampDirectionUp = (digitalRead(DIRECTION_SWITCH_PIN) == HIGH);

  // Read footswitch with debounce
  bool reading = digitalRead(FOOTSWITCH_PIN) == LOW; // Active low (pressed = LOW)

  if (reading != lastFootswitchState)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != footswitchState)
    {
      footswitchState = reading;
      handleFootswitch(footswitchState);
    }
  }

  lastFootswitchState = reading;

  // Process ramping
  if (isRamping)
  {
    updateRamp();
  }
}

void handleFootswitch(bool pressed)
{
  bool isMomentaryMode = digitalRead(MODE_SWITCH_PIN) == HIGH;

  if (isMomentaryMode)
  {
    // Momentary mode: press starts ramp, release reverses
    if (pressed)
    {
      startRamp(rampDirectionUp);
    }
    else
    {
      // Reverse direction from current position
      startRamp(!rampDirectionUp);
    }
  }
  else
  {
    // Latching mode: each press toggles
    if (pressed)
    {
      latchState = !latchState;
      startRamp(latchState ? rampDirectionUp : !rampDirectionUp);
    }
  }
}

void startRamp(bool goingUp)
{
  // FIX: Store the actual starting value for this ramp
  rampStartValue = currentValue;

  // Determine target and base duration
  if (goingUp)
  {
    targetValue = 127;
    rampDuration = map(analogRead(RAMP_UP_POT_PIN), 0, 1023, minRampDuration, maxRampDuration);
  }
  else
  {
    targetValue = 0;
    rampDuration = map(analogRead(RAMP_DOWN_POT_PIN), 0, 1023, minRampDuration, maxRampDuration);
  }

  // FIX: Calculate proportional duration based on actual distance to travel
  int distance = abs(targetValue - rampStartValue);
  if (distance < 127)
  {
    rampDuration = (rampDuration * distance) / 127;
  }

  // Ensure minimum duration
  if (rampDuration < 100)
    rampDuration = 100;

  isRamping = true;
  lastRampTime = millis();
  rampDirectionUp = goingUp;

  // Send immediate MIDI message at the start of the ramp for instant feedback
  sendMIDI(currentValue);
  lastMidiSendTime = millis();
}

void updateRamp()
{
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - lastRampTime;

  if (elapsed >= rampDuration)
  {
    // Ramp complete
    currentValue = targetValue;
    isRamping = false;
    sendMIDI(currentValue);
    return;
  }

  // Only update at the specified interval to avoid MIDI flooding
  if (currentTime - lastMidiSendTime < midiSendInterval)
  {
    return; // Skip this update, too soon
  }

  // Calculate progress (0.0 to 1.0)
  float rampProgress = (float)elapsed / (float)rampDuration;

  // Apply curve
  float curvedProgress = rampProgress;
  bool isLinear = digitalRead(CURVE_SWITCH_PIN) == HIGH;

  if (!isLinear)
  {
    // Exponential curve (ease-in)
    curvedProgress = curvedProgress * curvedProgress;
  }

  // FIX: Calculate current value using the STORED start value and target
  // Simple formula: current = start + (distance * progress)
  int newValue = rampStartValue + (int)((targetValue - rampStartValue) * curvedProgress);
  newValue = constrain(newValue, 0, 127);

  if (newValue != currentValue)
  {
    currentValue = newValue;
    sendMIDI(currentValue);
    lastMidiSendTime = currentTime; // Record when we sent MIDI
  }
}
