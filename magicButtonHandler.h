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
      WAITING_FOR_VALUE_2
    };
    
    State currentState;
    uint8_t pendingMsgType;
    uint16_t pendingValue1;
    uint16_t pendingValue2;
    
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
};

// Constructor
MagicButtonHandler::MagicButtonHandler(MagicButton* button) {
  magicButton = button;
  currentState = WAITING_FOR_COMMAND;
  pendingMsgType = 0;
  pendingValue1 = 0;
  pendingValue2 = 0;
  localExecutionEnabled = true;
  networkBroadcastEnabled = true;
  serialDebugEnabled = true;
}

// Get number of parameters needed for a message type
uint8_t MagicButtonHandler::getParameterCount(uint8_t msgType) {
  // Commands with 0 parameters
  if (msgType == 1 || msgType == 2 || msgType == 3 || msgType == 4 ||
      msgType == 5 || msgType == 6 || msgType == 7 || msgType == 8 ||
      msgType == 9 || msgType == 14 || msgType == 15 || msgType == 20 ||
      msgType == 21 || msgType == 22 || msgType == 23 || msgType == 24 ||
      msgType == 28 || msgType == 29 || msgType == 30 || msgType == 31 ||
      msgType == 39 || msgType == 40 || msgType == 50 || msgType == 51 || 
      msgType == 52 || msgType == 53 || msgType == 54 || msgType == 55 ||
      msgType == 63) {
    return 0;
  }
  
  // Commands with 1 parameter (val)
  if (msgType == 10 || msgType == 11 || msgType == 12 || msgType == 13 ||
      msgType == 16 || msgType == 17 || msgType == 18 || msgType == 19 ||
      msgType == 25 || msgType == 26 || msgType == 32 || msgType == 33 ||
      msgType == 34 || msgType == 41 || msgType == 48) {
    return 1;
  }
  
  // Commands with 2 parameters - not supported yet
  // msgType 37, 38, 42, 43 would need special handling
  
  // Unknown command
  return 255;  // Invalid
}

// Check if message type needs synchronized execution
bool MagicButtonHandler::needsSync(uint8_t msgType) {
  return (msgType == 9 || msgType == 48 || msgType == 50 || msgType == 54);
}

// Get human-readable command name for debugging
const char* MagicButtonHandler::getCommandName(uint8_t msgType) {
  switch(msgType) {
    case 1: return "upset_mainState";
    case 2: return "doubleRainbow";
    case 3: return "tranquilityMode";
    case 4: return "nextPalette";
    case 5: return "tripperTrapMode";
    case 6: return "antsMode";
    case 7: return "shootingStars";
    case 8: return "toggleHouseLights";
    case 9: return "launch";
    case 10: return "setStepRate";
    case 11: return "setFadeRate";
    case 12: return "setBrightness";
    case 13: return "setNRipples";
    case 14: return "noiseTest";
    case 15: return "noiseFader";
    case 16: return "noiseLength";
    case 17: return "noiseSpeed";
    case 18: return "noiseFadeLength";
    case 19: return "noiseFadeSpeed";
    case 20: return "earthMode";
    case 21: return "fireMode";
    case 22: return "airMode";
    case 23: return "waterMode";
    case 24: return "metalMode";
    case 25: return "airLength";
    case 26: return "airSpeed";
    case 28: return "rippleGeddon";
    case 29: return "tailTime";
    case 30: return "rainbowNoise";
    case 31: return "flashGrid";
    case 32: return "fireDecay";
    case 33: return "fireSpeed";
    case 34: return "setPalette";
    case 39: return "rainbowSpiral";
    case 40: return "blender";
    case 41: return "enlightenment";
    case 48: return "sweep";
    case 50: return "zoomToColour";
    case 51: return "mg_noise_party";
    case 52: return "mg_blob";
    case 53: return "mg_random";
    case 54: return "enlightenmentAchieved";
    case 55: return "returnTimer";
    case 63: return "nodeCounter";
    default: return "unknown";
  }
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
  uint16_t value = magicButton->checkButton();
  
  if (value == 0) {
    return;  // No new value
  }
  
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
          if (serialDebugEnabled) {
            Serial.printf("MagicButtonHandler: Command %d (%s) needs 1 value. Waiting...\n", 
                         pendingMsgType, getCommandName(pendingMsgType));
          }
        } else if (paramCount == 2) {
          // Wait for first value
          currentState = WAITING_FOR_VALUE_1;
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
          // Execute with one value
          executeCommand(pendingMsgType, pendingValue1);
          reset();
        } else if (paramCount == 2) {
          // Wait for second value
          currentState = WAITING_FOR_VALUE_2;
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
}

#endif // MAGIC_BUTTON_HANDLER_H
