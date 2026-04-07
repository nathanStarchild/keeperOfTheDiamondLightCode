#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

// Special message types for network communication
// 9xx range reserved for system/protocol messages

#define PING_MSG_TYPE               999
#define PONG_MSG_TYPE               998
#define ROLE_MSG_TYPE               997
#define BRIDGE_ANNOUNCE_MSG_TYPE    996
#define NODE_COUNT_MSG_TYPE         995
#define TOTAL_LED_COUNT_MSG_TYPE    994
#define SWEEP_SPOT_MSG_TYPE         993

// ============================================================================
// Message Type Lookup Table for MagicButton Handler
// ============================================================================
// Centralized parameter definitions with PROGMEM storage (~2.5KB flash)
// Enables timeout handling and eliminates scattered if/switch chains

// Timeout behavior when user abandons parameter entry
enum TimeoutAction {
  TIMEOUT_RESET,   // Call reset() to return to default state
  TIMEOUT_RANDOM   // Generate random value within parameter's defined range
};

// Parameter input method
enum ParamInputType {
  INPUT_BINARY,    // Accumulated binary value from short/long button presses
  INPUT_DURATION   // Milliseconds of button hold duration
};

// Parameter definition
struct ParamDef {
  const char* name;        // Parameter name (e.g., "brightness", "hue")
  int16_t min;             // Minimum value (use int16_t to support negative like tail.pspeed)
  int16_t max;             // Maximum value
  int16_t defaultVal;      // Default/aesthetic value for timeout or reset
  ParamInputType inputType; // How this parameter is entered
};

// Message type definition
struct MessageTypeDef {
  uint8_t msgType;         // Message type number
  const char* name;        // Command name (e.g., "setBrightness", "reset")
  uint8_t paramCount;      // Number of parameters (0, 1, or 2)
  ParamDef param1;         // First parameter (ignored if paramCount < 1)
  ParamDef param2;         // Second parameter (ignored if paramCount < 2)
  bool needsSync;          // Whether to broadcast to mesh after executing
  TimeoutAction timeoutAction; // What to do on timeout
};

// Lookup table stored in flash memory
// Includes all message types used by magicButtonHandler
// Sorted by msgType for binary search
const PROGMEM MessageTypeDef MESSAGE_TYPES[] = {
  // 0-parameter messages
  {1,  "upset",           0, {}, {}, true,  TIMEOUT_RESET},
  {2,  "doubleRainbow",   0, {}, {}, true,  TIMEOUT_RESET},
  {3,  "tranquilityMode", 0, {}, {}, true,  TIMEOUT_RESET},
  {5,  "tripperTrapMode", 0, {}, {}, true,  TIMEOUT_RESET},
  {6,  "antsMode",        0, {}, {}, true,  TIMEOUT_RESET},
  {7,  "shootingStars",   0, {}, {}, true,  TIMEOUT_RESET},
  {8,  "toggleHouseLights", 0, {}, {}, true,  TIMEOUT_RESET},
  {9,  "launch",          0, {}, {}, true,  TIMEOUT_RESET},
  {14, "noiseTest",       0, {}, {}, true,  TIMEOUT_RESET},
  {15, "noiseFader",      0, {}, {}, true,  TIMEOUT_RESET},
  {20, "earthMode",       0, {}, {}, true,  TIMEOUT_RESET},
  {21, "fireMode",        0, {}, {}, true,  TIMEOUT_RESET},
  {22, "airMode",         0, {}, {}, true,  TIMEOUT_RESET},
  {23, "waterMode",       0, {}, {}, true,  TIMEOUT_RESET},
  {24, "metalMode",       0, {}, {}, true,  TIMEOUT_RESET},
  {28, "rippleGeddon",    0, {}, {}, true,  TIMEOUT_RESET},
  {29, "tailTime",        0, {}, {}, true,  TIMEOUT_RESET},
  {30, "rainbowNoise",    0, {}, {}, true,  TIMEOUT_RESET},
  {31, "flashGrid",       0, {}, {}, true,  TIMEOUT_RESET},
  {39, "rainbowSpiral",   0, {}, {}, true,  TIMEOUT_RESET},
  {40, "blender",         0, {}, {}, true,  TIMEOUT_RESET},
  {41, "enlightenment",   0, {}, {}, false, TIMEOUT_RESET}, // Special: web sends boolean, timeout resets
  {50, "zoomToColour",    0, {}, {}, true,  TIMEOUT_RESET},  // Special: 0-param input, sends val internally
  {51, "mg_noise_party",  0, {}, {}, true,  TIMEOUT_RESET},
  {52, "mg_blob",         0, {}, {}, true,  TIMEOUT_RESET},
  {53, "mg_random",       0, {}, {}, true,  TIMEOUT_RESET},
  {54, "enlightenmentAchieved", 0, {}, {}, true,  TIMEOUT_RESET},
  {55, "returnTimer",     0, {}, {}, true,  TIMEOUT_RESET},
  {56, "chillPill",       0, {}, {}, true,  TIMEOUT_RESET},
  {63, "nodeCounter",     0, {}, {}, true,  TIMEOUT_RESET},
  
  // 1-parameter messages
  {10, "setStepRate",     1, {"stepRate",     2,     8,   4, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {11, "setFadeRate",     1, {"fadeRate",    10,   240,  80, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {12, "setBrightness",   1, {"brightness",   5,   255, 100, INPUT_BINARY}, {}, true,  TIMEOUT_RESET},  // Intentional changes only
  {13, "setNRipples",     1, {"nRipples",     1,    10,  10, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {16, "setNoisePlength", 1, {"plength",      1,    30,   8, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {17, "setNoisePspeed",  1, {"pspeed",       1,     5,   1, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {18, "setNoiseFadePlength", 1, {"plength",  1,    33,  10, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {19, "setNoiseFadePspeed",  1, {"pspeed",   1,    10,   1, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {25, "setAirPlength",   1, {"plength",      1,    30,  10, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {26, "setAirPspeed",    1, {"pspeed",       1,    10,   3, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {32, "setFireDecay",    1, {"decay",        1,   255,  20, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {33, "setFirePspeed",   1, {"pspeed",       1,   255,  30, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {34, "setPalette",      1, {"paletteIndex", 0,    25,   0, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {48, "setSweepPlength", 1, {"plength",      0,     7,   3, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM},
  {60, "setSweepDuration", 1, {"plength",     0,     7,   3, INPUT_BINARY}, {}, true,  TIMEOUT_RANDOM}, // Special: plength + duration
};

const uint8_t MESSAGE_TYPES_COUNT = sizeof(MESSAGE_TYPES) / sizeof(MessageTypeDef);

// Binary search lookup helper - returns pointer to MessageTypeDef or nullptr if not found
const MessageTypeDef* getMessageTypeDef(uint8_t msgType) {
  int left = 0;
  int right = MESSAGE_TYPES_COUNT - 1;
  
  while (left <= right) {
    int mid = (left + right) / 2;
    uint8_t midMsgType = pgm_read_byte(&MESSAGE_TYPES[mid].msgType);
    
    if (midMsgType == msgType) {
      return &MESSAGE_TYPES[mid];
    } else if (midMsgType < msgType) {
      left = mid + 1;
    } else {
      right = mid - 1;
    }
  }
  
  return nullptr; // Not found
}

// Helper to read string from PROGMEM
void readProgmemString(const char* progmemStr, char* buffer, size_t bufferSize) {
  strncpy_P(buffer, progmemStr, bufferSize - 1);
  buffer[bufferSize - 1] = '\0';
}

#endif // MESSAGE_TYPES_H
