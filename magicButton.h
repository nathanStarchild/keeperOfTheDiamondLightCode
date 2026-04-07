/*
 * MagicButton Class
 * 
 * Controls lights and sends messages using a single button with binary input
 * Short press = 1, Long press = 0
 * Builds a binary number from series of presses
 * 
 * Example: Short-Long-Short = 101 (binary) = 5 (decimal)
 */

#ifndef MAGIC_BUTTON_H
#define MAGIC_BUTTON_H

class MagicButton {
  public:
    // Constructor: takes button pin and optional pull-up mode
    MagicButton(uint8_t pin, bool usePullup = true);
    
    // Initialize button (call in setup())
    void begin();
    
    // Check button state and return accumulated value when sequence complete
    // Returns 0 if no sequence completed, otherwise returns the binary number
    // If getDuration=true, returns press duration in ms instead of binary value
    uint16_t checkButton(bool getDuration = false);
    
    // Reset the current sequence
    void reset();
    
    // Set thresholds (in milliseconds)
    void setShortPressThreshold(uint32_t ms);  // Max duration for "1" (default 300ms)
    void setSequenceTimeout(uint32_t ms);      // Timeout between presses (default 600ms)
    
    // Get current state info
    uint8_t getBitCount();                     // How many bits accumulated
    uint16_t getCurrentValue();                // Current accumulated value (even if not complete)
    bool isSequenceActive();                   // Is a sequence currently being entered
    
  private:
    uint8_t buttonPin;
    bool usePullup;
    
    // Button state tracking
    bool lastButtonState;
    bool currentButtonState;
    uint32_t pressStartTime;
    bool buttonPressed;
    
    // Sequence tracking
    uint16_t accumulatedValue;
    uint8_t bitCount;
  MilliTimer sequenceTimer;  // Direct member (not pointer) - initialized in constructor
    uint32_t shortPressThreshold;  // < this = 1, >= this = 0
    uint32_t sequenceTimeout;      // Time to wait before considering sequence complete
    
    // Helper functions
    void addBit(bool bit);
    void completeSequence();
    bool readButton();
};

// Constructor
// Note: Member initializer list (after ':' and before '{') constructs member objects
// BEFORE the constructor body runs. This is required for objects like MilliTimer that
// don't have a default constructor - they must be initialized with parameters.
// Syntax: Constructor(params) : memberObject(initValue) { body }
MagicButton::MagicButton(uint8_t pin, bool usePullup) 
  : sequenceTimer(600) {  // Initialize MilliTimer with 600ms timeout
  buttonPin = pin;
  this->usePullup = usePullup;
  lastButtonState = usePullup ? HIGH : LOW;
  currentButtonState = lastButtonState;
  pressStartTime = 0;
  buttonPressed = false;
  accumulatedValue = 0;
  bitCount = 0;
  sequenceActive = false;
  shortPressThreshold = 300;   // 300ms threshold
  sequenceTimeout = 600;       // 600ms timeout
  sequenceTimer.stopTimer();   // Start stopped
}

// Initialize button
void MagicButton::begin() {
  if (usePullup) {
    pinMode(buttonPin, INPUT_PULLUP);
  } else {
    pinMode(buttonPin, INPUT);
  }
  reset();
}

// Read button state (handles pullup logic)
bool MagicButton::readButton() {
  int reading = digitalRead(buttonPin);
  if (usePullup) {
    return (reading == LOW);  // Pressed when LOW with pullup
  } else {
    return (reading == HIGH); // Pressed when HIGH without pullup
  }
}

// Add a bit to the accumulated value
void MagicButton::addBit(bool bit) {
  if (bitCount < 16) {  // Limit to 16 bits for uint16_t
    accumulatedValue = (accumulatedValue << 1) | (bit ? 1 : 0);
    bitCount++;
    sequenceActive = true;
    sequenceTimer.startTimer();
  }
}

// Complete the sequence
void MagicButton::completeSequence() {
  sequenceActive = false;
  sequenceTimer.stopTimer();
}

// Check button and return value when sequence complete
uint16_t MagicButton::checkButton(bool getDuration) {
  currentButtonState = readButton();
  uint16_t returnValue = 0;
  
  // Detect button press (transition to pressed)
  if (currentButtonState && !lastButtonState && !buttonPressed) {
    // Button just pressed
    pressStartTime = millis();
    buttonPressed = true;
    Serial.println("pressed");
  }
  
  // Handle duration mode - return press duration in milliseconds
  if (getDuration) {
    // Detect button release and return duration
    if (!currentButtonState && lastButtonState && buttonPressed) {
      uint32_t pressDuration = millis() - pressStartTime;
      buttonPressed = false;
      Serial.print("MagicButton: Duration mode - press duration = ");
      Serial.print(pressDuration);
      Serial.println("ms");
      lastButtonState = currentButtonState;
      return pressDuration;
    }
    lastButtonState = currentButtonState;
    return 0;  // Still waiting for release
  }
  
  // Normal binary mode - detect button release and add bit
  if (!currentButtonState && lastButtonState && buttonPressed) {
    // Button just released - calculate duration and add bit
    uint32_t pressDuration = millis() - pressStartTime;
    
    if (pressDuration < shortPressThreshold) {
      // Short press = 1
      addBit(true);
      Serial.println("MagicButton: Short press (1)");
    } else {
      // Long press = 0
      addBit(false);
      Serial.println("MagicButton: Long press (0)");
    }
    
    Serial.print("MagicButton: Current value = ");
    Serial.print(accumulatedValue);
    Serial.print(" (");
    Serial.print(bitCount);
    Serial.println(" bits)");
    
    buttonPressed = false;
  }
  
  // Check for sequence timeout
  if (sequenceActive && !buttonPressed && sequenceTimer.isItTime()) {
    // Sequence complete!
    returnValue = accumulatedValue;
    Serial.print("MagicButton: Sequence complete! Value = ");
    Serial.println(returnValue);
    reset();
  }
  
  lastButtonState = currentButtonState;
  return returnValue;
}

// Reset sequence
void MagicButton::reset() {
  accumulatedValue = 0;
  bitCount = 0;
  sequenceActive = false;
  buttonPressed = false;
  sequenceTimer.stopTimer();
}

// Set short press threshold
void MagicButton::setShortPressThreshold(uint32_t ms) {
  shortPressThreshold = ms;
}

// Set sequence timeout
void MagicButton::setSequenceTimeout(uint32_t ms) {
  sequenceTimeout = ms;
  sequenceTimer.setInterval(ms);
}

// Get bit count
uint8_t MagicButton::getBitCount() {
  return bitCount;
}

// Get current value
uint16_t MagicButton::getCurrentValue() {
  return accumulatedValue;
}

// Check if sequence is active
bool MagicButton::isSequenceActive() {
  return sequenceActive;
}

#endif // MAGIC_BUTTON_H
