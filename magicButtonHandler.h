/*
 * MagicButtonHandler Class
 * 
 * Processes MagicButton input and executes lighting commands
 * Handles multi-value commands with a state machine
 * Sends messages to mesh network and executes locally
 */

#ifndef MAGIC_BUTTON_HANDLER_H
#define MAGIC_BUTTON_HANDLER_H

#include "magicButton.h"
#include "messageTypes.h"

// Forward declarations for global message handling
extern void processWSMessage();
extern DynamicJsonDocument wsMsg;
extern String wsMsgString;
extern bool outbox;
extern void nextPalette();
extern void next_mg_palette();
extern uint8_t paletteCycleIndex;
extern painlessMesh mesh;
extern String meshMsgString;
extern uint64_t pendingStartTime;

class MagicButtonHandler {
  public:
    // Constructor
    MagicButtonHandler(MagicButton* button);
    
    // Main update function - call this in loop()
    void update();
    
    // Configuration
    void enableLocalExecution(bool enable);   // Execute commands locally (default: true)
    void enableNetworkBroadcast(bool enable); // Send to mesh network (default: true)
    void enableSerialDebug(bool enable);      // Print debug info (default: true)
    
    // State info
    bool isWaitingForValue();
    uint8_t getCurrentMsgType();
    void reset();
    
  private:
    MagicButton* magicButton;
    
    // State machine
    enum State {
      WAITING_FOR_COMMAND,
      WAITING_FOR_VALUE_1,
      WAITING_FOR_VALUE_2,
      WAITING_FOR_DURATION
    };
    
    State currentState;
    uint8_t pendingMsgType;
    uint16_t pendingValue1;
    uint16_t pendingValue2;
    
    // Timeout handling
    MilliTimer parameterTimeout;
    static const uint32_t PARAMETER_TIMEOUT_MS = 2000;  // 2 seconds to enter parameter
    
    // Configuration flags
    bool localExecutionEnabled;
    bool networkBroadcastEnabled;
    bool serialDebugEnabled;
    
    // Helper functions
    uint8_t getParameterCount(uint8_t msgType);
    void executeCommand(uint8_t msgType, uint16_t val1 = 0, uint16_t val2 = 0);
    void buildMessage(uint8_t msgType, uint16_t val1 = 0, uint16_t val2 = 0);
    bool needsSync(uint8_t msgType);
    const char* getCommandName(uint8_t msgType);
    void handleTimeout();
    int16_t generateRandomInRange(int16_t min, int16_t max);
};

// Constructor
MagicButtonHandler::MagicButtonHandler(MagicButton* button) 
  : parameterTimeout(PARAMETER_TIMEOUT_MS) {
  magicButton = button;
  currentState = WAITING_FOR_COMMAND;
  pendingMsgType = 0;
  pendingValue1 = 0;
  pendingValue2 = 0;
  localExecutionEnabled = true;
  networkBroadcastEnabled = true;
  serialDebugEnabled = debugging;
  parameterTimeout.stopTimer();  // Don't start until waiting for parameter
}

// Get number of parameters needed for a message type
uint8_t MagicButtonHandler::getParameterCount(uint8_t msgType) {
  const MessageTypeDef* msgDef = getMessageTypeDef(msgType);
  
  if (msgDef != nullptr) {
    return pgm_read_byte(&msgDef->paramCount);
  }
  
  // Legacy/special commands not in lookup table
  // msgType 4 (nextPalette) - 0 params, but converts to setPalette
  if (msgType == 4) return 0;
  
  // Unknown command
  return 255;  // Invalid
}

// Check if message type needs synchronized execution
bool MagicButtonHandler::needsSync(uint8_t msgType) {
  const MessageTypeDef* msgDef = getMessageTypeDef(msgType);
  
  if (msgDef != nullptr) {
    return pgm_read_byte(&msgDef->needsSync);
  }
  
  return false;  // Default to no sync for unknown messages
}

// Get human-readable command name for debugging
const char* MagicButtonHandler::getCommandName(uint8_t msgType) {
  const MessageTypeDef* msgDef = getMessageTypeDef(msgType);
  
  if (msgDef != nullptr) {
    // Read pointer from PROGMEM (32-bit on both ESP8266 and ESP32)
    return (const char*)pgm_read_dword(&msgDef->name);
  }
  
  // Legacy commands not in lookup table
  if (msgType == 4) return "nextPalette";
  
  return "unknown";
}

// Build message and populate global variables
void MagicButtonHandler::buildMessage(uint8_t msgType, uint16_t val1, uint16_t val2) {
  DynamicJsonDocument doc(256);
  doc["msgType"] = msgType;
  
  uint8_t paramCount = getParameterCount(msgType);
  // msgType 50 is special - it's 0-param from user input but sends a val
  if (paramCount >= 1 || msgType == 50) {
    doc["val"] = val1;
  }
  if (paramCount >= 2) {
    doc["val2"] = val2;  // For future 2-param commands
  }
  
  // msgType 60 (sweep with duration) - special case
  if (msgType == 60) {
    doc["plength"] = val1;
    doc["duration"] = val2;
  }
  
  // Add startTime for synchronized execution on specific commands
  // Messages 9 (launch), 50 (zoomToColour), 54 (enlightenmentAchieved)
  if (needsSync(msgType)) {
    uint64_t startTime = mesh.getNodeTime() + 600000;  // 600ms = 600,000 microseconds
    doc["startTime"] = startTime;
    if (serialDebugEnabled) {
      Serial.printf("MagicButtonHandler: Added startTime (600ms from now)\n");
    }
  }
  
  // Populate wsMsg and wsMsgString (used for both local and network)
  wsMsg = doc;
  wsMsgString = "";
  serializeJson(doc, wsMsgString);
  
  if (serialDebugEnabled) {
    Serial.printf("MagicButtonHandler: Built message: %s\n", wsMsgString.c_str());
  }
}

// Execute the complete command
void MagicButtonHandler::executeCommand(uint8_t msgType, uint16_t val1, uint16_t val2) {
  bool skipLocalExecution = false;
  
  // Special handling for msgType 4 (nextPalette) - cycle locally then send setPalette to sync mesh
  if (msgType == 4) {
    nextPalette();
    val1 = paletteCycleIndex;
    msgType = 34;  // Change to setPalette message
    skipLocalExecution = true;  // Already cycled locally, don't execute again
    if (serialDebugEnabled) {
      Serial.printf("MagicButtonHandler: Called nextPalette(), sending setPalette with index=%d\n", val1);
    }
  }
  
  // Special handling for msgType 50 (zoomToColour) - cycles palette and uses paletteCycleIndex
  if (msgType == 50) {

    if (serialDebugEnabled) {
        Serial.printf("MagicButtonHandler: Palette was %d\n", paletteCycleIndex);
    }
    next_mg_palette();
    if(serialDebugEnabled) {
        Serial.printf("MagicButtonHandler: Called next_mg_palette(), new palette is %d\n", paletteCycleIndex);
    }
    val1 = paletteCycleIndex;
    if (serialDebugEnabled) {
      Serial.printf("MagicButtonHandler: Called next_mg_palette(), using paletteCycleIndex=%d\n", val1);
    }
  }
  
  if (serialDebugEnabled) {
    Serial.printf("MagicButtonHandler: Executing command %d (%s)", msgType, getCommandName(msgType));
    uint8_t paramCount = getParameterCount(msgType);
    if (paramCount >= 1 || msgType == 50 || msgType == 34) {  // msgType 50 and 34 may have values
      Serial.printf(" with val=%d", val1);
    }
    if (paramCount >= 2) {
      Serial.printf(", val2=%d", val2);
    }
    Serial.println();
  }
  
  // Build the message
  buildMessage(msgType, val1, val2);
    
  // Set outbox flag for network broadcast
  if (networkBroadcastEnabled) {
    outbox = true;
    if (serialDebugEnabled) {
      Serial.println("MagicButtonHandler: Set outbox flag for network broadcast");
    }
  }
  
  // Execute locally
  if (localExecutionEnabled && !skipLocalExecution) {
    if (needsSync(msgType) && wsMsg.containsKey("startTime")) {
      // For synchronized messages, schedule local execution at the same time
      uint64_t startTime = wsMsg["startTime"].as<uint64_t>();
      meshMsgString = wsMsgString;
      pendingStartTime = startTime;
      if (serialDebugEnabled) {
        Serial.println("MagicButtonHandler: Scheduled local execution at startTime");
      }
    } else {
      // Execute immediately for non-synchronized messages
      processWSMessage();
    }
  }
}

// Main update function
void MagicButtonHandler::update() {
  // Check for parameter entry timeout
  if (currentState != WAITING_FOR_COMMAND && parameterTimeout.isItTime()) {
    handleTimeout();
    return;
  }
  
  // Check if we're waiting for duration input
  if (currentState == WAITING_FOR_DURATION) {
    MagicButton::ButtonState state = magicButton->getState(true);  // Duration mode
    if (state == MagicButton::ACTIVE) {
      // User is pressing button - stop parameter timeout
      parameterTimeout.stopTimer();
    } else if (state == MagicButton::COMPLETE) {
      // Got the duration!
      uint16_t duration = magicButton->getValue();
      if (serialDebugEnabled) {
        Serial.printf("MagicButtonHandler: Got duration=%dms\n", duration);
      }
      executeCommand(pendingMsgType, pendingValue1, duration);
      reset();
    }
    return;  // Keep waiting for duration
  }
  
  // Normal binary input mode
  MagicButton::ButtonState state = magicButton->getState();
  
  if (state == MagicButton::ACTIVE) {
    // User is actively entering a sequence - stop parameter timeout
    parameterTimeout.stopTimer();
    return;
  } else if (state == MagicButton::IDLE) {
    return;  // No input yet
  }
  
  // State is COMPLETE - get the value
  uint16_t value = magicButton->getValue();
  
  // State machine
  switch (currentState) {
    case WAITING_FOR_COMMAND:
      {
        pendingMsgType = value;
        uint8_t paramCount = getParameterCount(pendingMsgType);
        
        if (paramCount == 255) {
          // Unknown or unsupported command
          if (serialDebugEnabled) {
            Serial.printf("MagicButtonHandler: Unknown/unsupported msgType %d\n", pendingMsgType);
          }
          reset();
          return;
        }
        
        if (paramCount == 0) {
          // Execute immediately
          executeCommand(pendingMsgType);
          reset();
        } else if (paramCount == 1) {
          // Wait for value
          currentState = WAITING_FOR_VALUE_1;
          parameterTimeout.startTimer();
          if (serialDebugEnabled) {
            Serial.printf("MagicButtonHandler: Command %d (%s) needs 1 value. Waiting...\n", 
                         pendingMsgType, getCommandName(pendingMsgType));
          }
        } else if (paramCount == 2) {
          // Wait for first value
          currentState = WAITING_FOR_VALUE_1;
          parameterTimeout.startTimer();
          if (serialDebugEnabled) {
            Serial.printf("MagicButtonHandler: Command %d (%s) needs 2 values. Waiting...\n", 
                         pendingMsgType, getCommandName(pendingMsgType));
          }
        }
      }
      break;
      
    case WAITING_FOR_VALUE_1:
      {
        pendingValue1 = value;
        uint8_t paramCount = getParameterCount(pendingMsgType);
        
        if (paramCount == 1) {
          // Check if this is msgType 60 (needs duration hold)
          if (pendingMsgType == 60) {
            currentState = WAITING_FOR_DURATION;
            parameterTimeout.startTimer();
            if (serialDebugEnabled) {
              Serial.printf("MagicButtonHandler: Got plength=%d, now waiting for duration hold...\n", pendingValue1);
            }
          } else {
            // Execute with one value
            executeCommand(pendingMsgType, pendingValue1);
            reset();
          }
        } else if (paramCount == 2) {
          // Wait for second value
          currentState = WAITING_FOR_VALUE_2;
          parameterTimeout.startTimer();
          if (serialDebugEnabled) {
            Serial.printf("MagicButtonHandler: Got value1=%d, waiting for value2...\n", pendingValue1);
          }
        }
      }
      break;
      
    case WAITING_FOR_VALUE_2:
      pendingValue2 = value;
      executeCommand(pendingMsgType, pendingValue1, pendingValue2);
      reset();
      break;
  }
}

// Configuration methods
void MagicButtonHandler::enableLocalExecution(bool enable) {
  localExecutionEnabled = enable;
}

void MagicButtonHandler::enableNetworkBroadcast(bool enable) {
  networkBroadcastEnabled = enable;
}

void MagicButtonHandler::enableSerialDebug(bool enable) {
  serialDebugEnabled = enable;
}

// State query methods
bool MagicButtonHandler::isWaitingForValue() {
  return currentState != WAITING_FOR_COMMAND;
}

uint8_t MagicButtonHandler::getCurrentMsgType() {
  return pendingMsgType;
}

void MagicButtonHandler::reset() {
  currentState = WAITING_FOR_COMMAND;
  pendingMsgType = 0;
  pendingValue1 = 0;
  pendingValue2 = 0;
  parameterTimeout.stopTimer();
}

// Generate random value within range (excludes 0 for tail.pspeed special case)
int16_t MagicButtonHandler::generateRandomInRange(int16_t min, int16_t max) {
  if (min < 0) {
    // Handle negative ranges (like tail.pspeed: -3 to 3, excluding 0)
    // Generate random in full range, then skip 0
    int16_t val = random(min, max + 1);
    if (val == 0 && min < 0 && max > 0) {
      // Flip a coin: go negative or positive
      val = random(0, 2) ? random(min, 0) : random(1, max + 1);
    }
    return val;
  } else {
    return random(min, max + 1);
  }
}

// Handle parameter entry timeout
void MagicButtonHandler::handleTimeout() {
  const MessageTypeDef* msgDef = getMessageTypeDef(pendingMsgType);
  
  if (msgDef == nullptr) {
    if (serialDebugEnabled) {
      Serial.printf("MagicButtonHandler: Timeout on unknown msgType %d - resetting\n", pendingMsgType);
    }
    reset();
    return;
  }
  
  TimeoutAction action = (TimeoutAction)pgm_read_byte(&msgDef->timeoutAction);
  
  if (action == TIMEOUT_RESET) {
    if (serialDebugEnabled) {
      Serial.printf("MagicButtonHandler: Timeout on %s - resetting (ignoring input)\n", getCommandName(pendingMsgType));
    }
    // Just reset state, don't execute any command
    reset();
    return;
  }
  
  // TIMEOUT_RANDOM - generate random values for incomplete parameters
  if (serialDebugEnabled) {
    Serial.printf("MagicButtonHandler: Timeout on %s - generating random values\n", getCommandName(pendingMsgType));
  }
  
  uint8_t paramCount = pgm_read_byte(&msgDef->paramCount);
  
  if (currentState == WAITING_FOR_VALUE_1) {
    // Need to generate value1 (and possibly value2)
    int16_t min1 = pgm_read_word(&msgDef->param1.min);
    int16_t max1 = pgm_read_word(&msgDef->param1.max);
    pendingValue1 = generateRandomInRange(min1, max1);
    
    if (paramCount == 2) {
      int16_t min2 = pgm_read_word(&msgDef->param2.min);
      int16_t max2 = pgm_read_word(&msgDef->param2.max);
      pendingValue2 = generateRandomInRange(min2, max2);
    }
  } else if (currentState == WAITING_FOR_VALUE_2) {
    // Already have value1, just need value2
    int16_t min2 = pgm_read_word(&msgDef->param2.min);
    int16_t max2 = pgm_read_word(&msgDef->param2.max);
    pendingValue2 = generateRandomInRange(min2, max2);
  } else if (currentState == WAITING_FOR_DURATION) {
    // Generate random duration for msgType 60
    int16_t minDur = pgm_read_word(&msgDef->param1.min);  // duration stored in param1
    int16_t maxDur = pgm_read_word(&msgDef->param1.max);
    pendingValue2 = generateRandomInRange(minDur, maxDur);  // duration goes in val2
  }
  
  if (serialDebugEnabled) {
    if (paramCount == 1) {
      Serial.printf("MagicButtonHandler: Generated val=%d\n", pendingValue1);
    } else if (paramCount == 2) {
      Serial.printf("MagicButtonHandler: Generated val1=%d, val2=%d\n", pendingValue1, pendingValue2);
    }
  }
  
  // Execute with generated values
  executeCommand(pendingMsgType, pendingValue1, pendingValue2);
  reset();
}

#endif // MAGIC_BUTTON_HANDLER_H
