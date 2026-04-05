// meshNode.h - Mesh-only network node (for costume ESPs)
// Connects to mesh network only, receives forwarded messages from bridge

// painlessMesh library (includes WiFi internally)
#include "painlessMesh.h"

#include <ArduinoOTA.h>

//https://arduinojson.org/v6/api/config/use_long_long/
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

// ========== CONFIGURATION ==========
// Mesh Network Settings
#define MESH_SSID "diamondLightMesh"
#define MESH_PASSWORD "keeperOfTheDiamondLight"
#define MESH_PORT 5555
#define MESH_CHANNEL 7

// Message Protocol
#define BRIDGE_ANNOUNCE_MSG_TYPE 996

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

// ========== FORWARD DECLARATIONS ==========
extern void processWSMessage();  // Defined in main .ino file
extern void nodeCounter();  // Defined in main .ino file
extern MilliTimer nodeCountTimer;  // Defined in main .ino file
extern uint16_t sweepSpot; // defined in element definition


void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

// ========== STATE MANAGEMENT ==========

void updateSweepSpot() {
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
      sweepSpot = position + 3;
      Serial.printf("Updated sweepSpot to %d (position %d, %d total nodes)\n", 
                    sweepSpot, position, nodeList.size());
      break;
    }
    
    // Only increment position for non-bridge nodes
    if (!bridgeConnected || nodeId != bridgeId) {
      position++;
    }
    
    ++it;
  }
}

// ========== MESH CALLBACKS ==========

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Mesh: Received from %u: %s\n", from, msg.c_str());
  
  // Check for bridge announcement
  DynamicJsonDocument doc(128);
  if (deserializeJson(doc, msg) == DeserializationError::Ok) {
    if (doc["msgType"] == BRIDGE_ANNOUNCE_MSG_TYPE) {
      bridgeId = from;
      bridgeConnected = true;
      Serial.printf("Mesh: Bridge connected (nodeId %u)\n", bridgeId);
      updateSweepSpot();
      return;  // Don't process as pattern message
    }
  }
  
  // Check if we're still waiting on a previous scheduled message
  if (pendingStartTime > 0) {
    Serial.println("Mesh: Warning - new message received while waiting on scheduled message, dropping old message");
    pendingStartTime = 0;
  }
  
  DynamicJsonDocument meshDoc(1024);
  DeserializationError error = deserializeJson(meshDoc, msg);
  
  if (error) {
    Serial.printf("Mesh: JSON parse error: %s\n", error.c_str());
    return;
  }
  
  // Check if message has a startTime
  if (meshDoc.containsKey("startTime")) {
    uint64_t startTime = meshDoc["startTime"].as<uint64_t>();
    uint64_t now = mesh.getNodeTime();
    
    // Check if message is stale (startTime more than 5 seconds ago)
    if (now > startTime && (now - startTime) > 5000000) {  // 5 seconds = 5,000,000 microseconds
      Serial.printf("Mesh: Dropping stale message (startTime was %llu us ago)\n", (now - startTime));
      return;
    }
    
    if (now < startTime) {
      // Schedule for later execution
      meshMsgString = msg;
      pendingStartTime = startTime;
      uint64_t waitUs = startTime - now;
      Serial.printf("Mesh: Scheduled message for execution in %llu us\n", waitUs);
      return;
    }
    
    // StartTime already passed (but within 5 seconds), execute immediately
    Serial.println("Mesh: StartTime recently passed, executing immediately");
  }
  
  // No startTime or time already passed - execute immediately
  meshMsgString = msg;
  meshInbox = true;
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("Mesh: New connection, nodeId = %u\n", nodeId);
  meshConnected = mesh.getNodeList().size() > 0;
}

void changedConnectionCallback() {
  Serial.printf("Mesh: Connections changed\n");
  Serial.printf("Mesh: Nodes connected: %d\n", mesh.getNodeList().size());
  meshConnected = mesh.getNodeList().size() > 0;
  if (meshConnected) {
    updateSweepSpot();
  }
  mainState.nodeCount.plength = mesh.getNodeList().size() + 1;
  nodeCountTimer.setInterval(mainState.nodeCount.decay * 10 * mainState.nodeCount.plength);
  nodeCounter();
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Mesh: Time adjusted by %d us\n", offset);
}

// ========== SETUP AND LOOP ==========

void wsSetup() {
  Serial.println("meshNode: Initializing...");
  
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
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";
    Serial.println("OTA: Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA: End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA: Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
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
    Serial.printf("meshNode: Sent to mesh: %s\n", wsMsgString.c_str());
    outbox = false;
  }
  
  // Check for pending scheduled message
  if (pendingStartTime > 0) {
    uint32_t before = millis();
    uint64_t now = mesh.getNodeTime();
    uint32_t after = millis();
    
    if (now >= pendingStartTime) {
      // Time to execute!
      Serial.println("meshNode: Executing scheduled message (startTime reached)");
      meshInbox = true;
      pendingStartTime = 0;
    } else {
      // Still waiting
      if (loopCounter % 100 == 0) {  // Only print occasionally to avoid spam
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
      Serial.printf("meshNode: Error parsing mesh message: %s\n", error.c_str());
    }
    meshInbox = false;
  }
}
