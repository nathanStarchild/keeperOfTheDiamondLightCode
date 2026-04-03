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

// ========== GLOBAL OBJECTS ==========
painlessMesh mesh;

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

// ========== FORWARD DECLARATIONS ==========
void processWSMessage();  // Defined in main .ino file
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

// ========== MESH CALLBACKS ==========

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Mesh: Received from %u: %s\n", from, msg.c_str());
  
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
  
  Serial.printf("meshNode: Mesh ID = %u\n", mesh.getNodeId());
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
  ArduinoOTA.handle();
  
  // Send outgoing mesh message FIRST (before incoming overwrites wsMsgString)
  if (outbox) {
    mesh.sendBroadcast(wsMsgString);
    Serial.printf("meshNode: Sent to mesh: %s\n", wsMsgString.c_str());
    outbox = false;
  }
  
  // Check for pending scheduled message
  if (pendingStartTime > 0) {
    uint64_t now = mesh.getNodeTime();
    
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
