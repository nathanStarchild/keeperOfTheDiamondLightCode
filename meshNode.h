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
String meshMsgString;

// Pending messages (timed execution)
#define MAX_PENDING_MESSAGES 10  // Increased from 2, but not too large for ESP8266 RAM
struct PendingMessage {
  String msgString;
  uint64_t startTime;
};

PendingMessage pendingMessages[MAX_PENDING_MESSAGES];
uint8_t pendingMessageCount = 0;

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
  
  DynamicJsonDocument meshDoc(1024);
  DeserializationError error = deserializeJson(meshDoc, msg);
  
  if (error) {
    Serial.printf("Mesh: JSON parse error: %s\n", error.c_str());
    return;
  }
  
  // Process message locally - execute immediately or queue for later
  if (meshDoc.containsKey("startTime")) {
    uint64_t startTime = meshDoc["startTime"];  // in mesh microseconds
    uint64_t now = mesh.getNodeTime();  // in mesh microseconds
    
    if (startTime <= now) {
      Serial.printf("Mesh: now = %11u, processing immediately\n", now);
      // Execute immediately
      meshMsgString = msg;
      meshInbox = true;
    } else {
      // Queue for later
      if (pendingMessageCount < MAX_PENDING_MESSAGES) {
        pendingMessages[pendingMessageCount].msgString = msg;
        pendingMessages[pendingMessageCount].startTime = startTime;
        pendingMessageCount++;
        Serial.printf("Mesh: Queued message for %llu us (count: %d)\n", startTime, pendingMessageCount);
      } else {
        // Buffer full - drop oldest message to make room
        Serial.println("Mesh: WARNING - Buffer full, dropping oldest message");
        for (uint8_t j = 0; j < MAX_PENDING_MESSAGES - 1; j++) {
          pendingMessages[j] = pendingMessages[j + 1];
        }
        pendingMessages[MAX_PENDING_MESSAGES - 1].msgString = msg;
        pendingMessages[MAX_PENDING_MESSAGES - 1].startTime = startTime;
        // Count stays at MAX_PENDING_MESSAGES
      }
    }
  } else {
    // No startTime, execute immediately
    meshMsgString = msg;
    meshInbox = true;
  }
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
  
  // Process pending messages (with bounds checking)
  if (pendingMessageCount > 0) {
    // Sanity check - fix corruption if it happened
    if (pendingMessageCount > MAX_PENDING_MESSAGES) {
      Serial.printf("meshNode: ERROR - Corrupted count %d, resetting to %d\n", pendingMessageCount, MAX_PENDING_MESSAGES);
      pendingMessageCount = MAX_PENDING_MESSAGES;
    }
    
    uint64_t now = mesh.getNodeTime();  // in mesh microseconds
    for (uint8_t i = 0; i < pendingMessageCount && i < MAX_PENDING_MESSAGES; i++) {
      if (pendingMessages[i].startTime <= now) {
        wsMsgString = pendingMessages[i].msgString;
        inbox = true;
        Serial.printf("meshNode: Executing pending message (count: %d)\n", pendingMessageCount);
        
        for (uint8_t j = i; j < pendingMessageCount - 1; j++) {
          pendingMessages[j] = pendingMessages[j + 1];
        }
        pendingMessageCount--;
        break;
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
