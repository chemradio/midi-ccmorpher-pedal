/*
 * MIDI Expression Morph Pedal
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
const int MIDI_CHANNEL = 1;    // Change this if needed (1-16)
const int MIDI_CC_NUMBER = 11; // Change to your desired CC number

// State variables
bool footswitchState = false;
bool lastFootswitchState = false;
bool latchState = false;
int currentValue = 0;
int targetValue = 0;
bool isRamping = false;
bool rampDirectionUp = true; // true = up, false = down
unsigned long lastRampTime = 0;
float rampProgress = 0.0;
int rampDuration = 1000; // milliseconds

// Debounce
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

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
}

void loop()
{
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
    // Latching mode: each press/release toggles
    if (pressed)
    {
      latchState = !latchState;
      startRamp(latchState ? rampDirectionUp : !rampDirectionUp);
    }
  }
}

void startRamp(bool goingUp)
{
  int startValue = currentValue;

  // Determine target and duration
  if (goingUp)
  {
    targetValue = 127;
    rampDuration = map(analogRead(RAMP_UP_POT_PIN), 0, 1023, 100, 10000);
  }
  else
  {
    targetValue = 0;
    rampDuration = map(analogRead(RAMP_DOWN_POT_PIN), 0, 1023, 100, 10000);
  }

  // Calculate proportional duration if interrupted mid-ramp
  int distance = abs(targetValue - startValue);
  if (distance < 127)
  {
    rampDuration = (rampDuration * distance) / 127;
  }

  // Ensure minimum duration
  if (rampDuration < 100)
    rampDuration = 100;

  isRamping = true;
  rampProgress = 0.0;
  lastRampTime = millis();
  rampDirectionUp = goingUp;
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

  // Calculate progress (0.0 to 1.0)
  rampProgress = (float)elapsed / (float)rampDuration;

  // Apply curve
  float curvedProgress = rampProgress;
  bool isLinear = digitalRead(CURVE_SWITCH_PIN) == HIGH;

  if (!isLinear)
  {
    // Exponential curve
    curvedProgress = curvedProgress * curvedProgress;
  }

  // Calculate current value
  int startValue = rampDirectionUp ? 0 : 127;
  int range = rampDirectionUp ? 127 : -127;

  // Account for interrupted ramps
  if (rampDirectionUp && currentValue > 0)
  {
    startValue = currentValue - (int)((currentValue) * (1.0 - rampProgress));
  }
  else if (!rampDirectionUp && currentValue < 127)
  {
    startValue = currentValue + (int)((127 - currentValue) * (1.0 - rampProgress));
  }

  int newValue = startValue + (int)(range * curvedProgress);
  newValue = constrain(newValue, 0, 127);

  if (newValue != currentValue)
  {
    currentValue = newValue;
    sendMIDI(currentValue);
  }
}

void sendMIDI(int value)
{
  // Send MIDI CC message over 5-pin DIN (and USB if connected)
  // Format: Status byte, CC number, Value
  byte status = 0xB0 | ((MIDI_CHANNEL - 1) & 0x0F); // Control Change
  Serial.write(status);
  Serial.write(MIDI_CC_NUMBER & 0x7F);
  Serial.write(value & 0x7F);
}