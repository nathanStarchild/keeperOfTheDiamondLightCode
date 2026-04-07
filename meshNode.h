// meshNode.h - Mesh-only network node (for costume ESPs)
// Connects to mesh network only, receives forwarded messages from bridge

// painlessMesh library (includes WiFi internally)
#include "painlessMesh.h"

#include <ArduinoOTA.h>
#include <EEPROM.h>

//https://arduinojson.org/v6/api/config/use_long_long/
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

extern bool debugging;

// ========== CONFIGURATION ==========
// EEPROM addresses
#define EEPROM_BRIDGE_ID_ADDR 0  // 4 bytes for uint32_t
#define EEPROM_SIZE 512

// Mesh Network Settings
#define MESH_SSID "diamondLightMesh"
#define MESH_PASSWORD "keeperOfTheDiamondLight"
#define MESH_PORT 5555
#define MESH_CHANNEL 7

// ========== GLOBAL OBJECTS ==========
painlessMesh mesh;
uint32_t myId;

// ========== MESSAGE HANDLING ==========
DynamicJsonDocument wsMsg(1024);
String wsMsgString;

bool inbox = false;
bool meshInbox = false;
bool outbox = false;
String meshMsgString;
uint64_t pendingStartTime = 0;  // 0 means no pending scheduled message

// ========== STATE TRACKING ==========
bool meshConnected = false;
bool bridgeConnected = false;
uint32_t bridgeId = 0;
uint16_t nonMeshDeviceCount = 0;  // Pyramid and other direct-connected devices
uint32_t lastNodeCounterTime = 0;  // Debounce nodeCounter() calls
#define NODE_COUNTER_DEBOUNCE_MS 3000  // Don't run nodeCounter() more than once per 3 seconds

// ========== FORWARD DECLARATIONS ==========
extern void processWSMessage();  // Defined in main .ino file
extern void nodeCounter();  // Defined in main .ino file
extern MilliTimer nodeCountTimer;  // Defined in main .ino file
extern uint16_t sweepSpot; // defined in element definition


void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void updateNodeState(bool runNodeCounter = true);
void saveBridgeId();
void loadBridgeId();

// ========== EEPROM FUNCTIONS ==========

void saveBridgeId() {
  EEPROM.put(EEPROM_BRIDGE_ID_ADDR, bridgeId);
  EEPROM.commit();
  if (debugging) {
    Serial.printf("Saved bridgeId to EEPROM: %u\n", bridgeId);
  }
}

void loadBridgeId() {
  uint32_t storedBridgeId = 0;
  EEPROM.get(EEPROM_BRIDGE_ID_ADDR, storedBridgeId);
  
  // Check if we have a valid stored bridge ID (not 0 or 0xFFFFFFFF)
  if (storedBridgeId != 0 && storedBridgeId != 0xFFFFFFFF) {
    bridgeId = storedBridgeId;
    if (debugging) {
      Serial.printf("Loaded bridgeId from EEPROM: %u\n", bridgeId);
    }
  } else {
    if (debugging) {
      Serial.println("No valid bridgeId in EEPROM");
    }
  }
}

// ========== STATE MANAGEMENT ==========

void updateNodeState(bool runNodeCounter) {
  // Calculate this node's sweepSpot based on position in sorted mesh network
  // Mesh nodes start after non-mesh devices (pyramids, etc.)
  // Bridge is excluded from position counting
  
  SimpleList<uint32_t> nodeList = mesh.getNodeList();
  nodeList.push_back(myId);
  
  // Sort the list
  nodeList.sort([](uint32_t a, uint32_t b) { return a < b; });
  
  // Find my position, excluding bridge if connected
  uint8_t position = 0;
  
  SimpleList<uint32_t>::iterator it = nodeList.begin();
  while (it != nodeList.end()) {
    uint32_t nodeId = *it;
    
    if (nodeId == myId) {
      sweepSpot = position + nonMeshDeviceCount;
      Serial.printf("Updated sweepSpot to %d (position %d in mesh, %d non-mesh devices)\n", 
                    sweepSpot, position, nonMeshDeviceCount);
      break;
    }
    
    // Only increment position for non-bridge nodes
    if (!bridgeConnected || nodeId != bridgeId) {
      position++;
    }
    
    ++it;
  }
  
  // Calculate total LED count: mesh nodes + self - bridge (if present) + non-mesh devices
  uint16_t meshNodes = mesh.getNodeList().size() + 1;  // +1 for self
  uint16_t totalNodes = meshNodes - (bridgeConnected ? 1 : 0) + nonMeshDeviceCount;
  
  mainState.nodeCount.plength = totalNodes;
  nodeCountTimer.setInterval(mainState.nodeCount.decay * 10 * totalNodes);
  
  // Only run nodeCounter if requested and debounce time has passed
  if (runNodeCounter) {
    uint32_t now = millis();
    if (now - lastNodeCounterTime >= NODE_COUNTER_DEBOUNCE_MS) {
      lastNodeCounterTime = now;
      nodeCounter();
    } else {
      if (debugging) {
        Serial.printf("Skipping nodeCounter() - debounced (%lu ms since last)\n", now - lastNodeCounterTime);
      }
    }
  }
}

// ========== MESH CALLBACKS ==========

void receivedCallback(uint32_t from, String &msg) {
  if (debugging) {
    Serial.printf("Mesh: Received from %u: %s\n", from, msg.c_str());
  }
  
  // Check for bridge announcement
  DynamicJsonDocument doc(128);
  if (deserializeJson(doc, msg) == DeserializationError::Ok) {
    if (doc["msgType"] == BRIDGE_ANNOUNCE_MSG_TYPE) {
      bool wasNewBridge = (bridgeId != from);
      bridgeId = from;
      bridgeConnected = true;
      
      // Get non-mesh device count from bridge announcement
      if (doc.containsKey("nonMeshCount")) {
        nonMeshDeviceCount = doc["nonMeshCount"];
      }
      
      if (debugging) {
        Serial.printf("Mesh: Bridge connected (nodeId %u, %u non-mesh devices)\n", 
                      bridgeId, nonMeshDeviceCount);
      }
      
      // Save bridge ID to EEPROM for future recognition
      if (wasNewBridge) {
        saveBridgeId();
      }
      
      // Always run nodeCounter when bridge announces (confirms final device count)
      updateNodeState(true);
      return;  // Don't process as pattern message
    }
  }
  
  // Check if we're still waiting on a previous scheduled message
  if (pendingStartTime > 0) {
    if (debugging) {
      Serial.println("Mesh: Warning - new message received while waiting on scheduled message, dropping old message");
    }
    pendingStartTime = 0;
  }
  
  DynamicJsonDocument meshDoc(1024);
  DeserializationError error = deserializeJson(meshDoc, msg);
  
  if (error) {
    if (debugging) {
      Serial.printf("Mesh: JSON parse error: %s\n", error.c_str());
    }
    return;
  }
  
  // Check if message has a startTime
  if (meshDoc.containsKey("startTime")) {
    uint64_t startTime = meshDoc["startTime"].as<uint64_t>();
    uint64_t now = mesh.getNodeTime();
    
    // Check if message is stale (startTime more than 5 seconds ago)
    if (now > startTime && (now - startTime) > 5000000) {  // 5 seconds = 5,000,000 microseconds
      if (debugging) {
        Serial.printf("Mesh: Dropping stale message (startTime was %llu us ago)\n", (now - startTime));
      }
      return;
    }
    
    if (now < startTime) {
      // Schedule for later execution
      meshMsgString = msg;
      pendingStartTime = startTime;
      if (debugging) {
        uint64_t waitUs = startTime - now;
        Serial.printf("Mesh: Scheduled message for execution in %llu us\n", waitUs);
      }
      return;
    }
    
    // StartTime already passed (but within 5 seconds), execute immediately
    if (debugging) {
      Serial.println("Mesh: StartTime recently passed, executing immediately");
    }
  }
  
  // No startTime or time already passed - execute immediately
  meshMsgString = msg;
  meshInbox = true;
}

void newConnectionCallback(uint32_t nodeId) {
  if (debugging) {
    Serial.printf("Mesh: New connection, nodeId = %u\n", nodeId);
  }
  bool wasConnected = meshConnected;
  meshConnected = mesh.getNodeList().size() > 0;
  
  // Check if this is the bridge we know from EEPROM
  if (bridgeId != 0 && nodeId == bridgeId) {
    bridgeConnected = true;
    if (debugging) {
      Serial.printf("Mesh: Recognized bridge from EEPROM (nodeId %u)\n", bridgeId);
    }
    // Don't run updateNodeState here - wait for bridge announce with device count
  }
  
  // If we just connected after being disconnected, reset nonMeshDeviceCount
  if (!wasConnected && meshConnected) {
    nonMeshDeviceCount = 0;
    if (debugging) {
      Serial.println("Mesh: Reconnected - reset non-mesh device count to 0");
    }
  }
}

void changedConnectionCallback() {
  if (debugging) {
    Serial.printf("Mesh: Connections changed\n");
    Serial.printf("Mesh: Nodes connected: %d\n", mesh.getNodeList().size());
  }
  
  bool wasConnected = meshConnected;
  meshConnected = mesh.getNodeList().size() > 0;
  
  // If we just lost all connections, reset nonMeshDeviceCount
  if (wasConnected && !meshConnected) {
    nonMeshDeviceCount = 0;
    bridgeConnected = false;
    if (debugging) {
      Serial.println("Mesh: Disconnected - reset non-mesh device count to 0");
    }
  }
  
  // Check if bridge disconnected (was in list, now isn't)
  if (bridgeConnected) {
    bool bridgeStillConnected = false;
    SimpleList<uint32_t> nodeList = mesh.getNodeList();
    SimpleList<uint32_t>::iterator it = nodeList.begin();
    while (it != nodeList.end()) {
      if (*it == bridgeId) {
        bridgeStillConnected = true;
        break;
      }
      ++it;
    }
    
    if (!bridgeStillConnected) {
      bridgeConnected = false;
      nonMeshDeviceCount = 0;
      if (debugging) {
        Serial.println("Mesh: Bridge disconnected - reset non-mesh device count to 0");
      }
    }
  }
  
  // Run nodeCounter() only if there's no bridge (bridge will announce with count)
  bool runCounter = !bridgeConnected;
  updateNodeState(runCounter);
}

void nodeTimeAdjustedCallback(int32_t offset) {
  if (debugging) {
    Serial.printf("Mesh: Time adjusted by %d us\n", offset);
  }
}

// ========== SETUP AND LOOP ==========

void wsSetup() {
  Serial.println("meshNode: Initializing...");
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Load bridge ID from EEPROM if available
  loadBridgeId();
  
  // Initialize mesh network
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Initialize mesh in AP+STA mode on channel 7
  mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.setHostname("DiamondLightNode");
    
  // Register mesh callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);


  myId = mesh.getNodeId();
  
  Serial.printf("meshNode: Mesh ID = %u\n", myId);
  Serial.println("meshNode: Mesh-only mode");
  
  // Setup ArduinoOTA
  ArduinoOTA.setPassword("antares");
  ArduinoOTA.onStart([]() {
    if (debugging) {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else
        type = "filesystem";
      Serial.println("OTA: Start updating " + type);
    }
  });
  ArduinoOTA.onEnd([]() {
    if (debugging) {
      Serial.println("\nOTA: End");
    }
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (debugging) {
      Serial.printf("OTA: Progress: %u%%\r", (progress / (total / 100)));
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (debugging) {
      Serial.printf("OTA: Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
  Serial.println("meshNode: Setup complete");
  
  WiFi.setSleep(false);
}

void wsLoop() {
  static uint8_t loopCounter = 0;
  loopCounter++;

  // Update mesh network (only every 10th loop to reduce overhead)
  if(loopCounter % 10 == 0){
    mesh.update();
  }
  
  // Handle ArduinoOTA
  // ArduinoOTA.handle();
  
  // Send outgoing mesh message FIRST (before incoming overwrites wsMsgString)
  if (outbox) {
    mesh.sendBroadcast(wsMsgString);
    if (debugging) {
      Serial.printf("meshNode: Sent to mesh: %s\n", wsMsgString.c_str());
    }
    outbox = false;
  }
  
  // Check for pending scheduled message
  if (pendingStartTime > 0) {
    uint32_t before = millis();
    uint64_t now = mesh.getNodeTime();
    uint32_t after = millis();
    
    if (now >= pendingStartTime) {
      // Time to execute!
      if (debugging) {
        Serial.println("meshNode: Executing scheduled message (startTime reached)");
      }
      meshInbox = true;
      pendingStartTime = 0;
    } else {
      // Still waiting
      if (debugging && loopCounter % 100 == 0) {  // Only print occasionally to avoid spam
        uint64_t waitUs = pendingStartTime - now;
        Serial.printf("meshNode: Waiting %llu us for scheduled execution\n", waitUs);
        Serial.printf("it takes %u ms to get mesh time\n", after - before);
      }
    }
  }
  
  // Process incoming mesh message
  if (meshInbox) {
    DeserializationError error = deserializeJson(wsMsg, meshMsgString);
    if (!error) {
      wsMsgString = meshMsgString;
      processWSMessage();
    } else {
      if (debugging) {
        Serial.printf("meshNode: Error parsing mesh message: %s\n", error.c_str());
      }
    }
    meshInbox = false;
  }
}
