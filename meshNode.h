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
#define MESH_CHANNEL 

// ========== GLOBAL OBJECTS ==========
painlessMesh mesh;

// ========== MESSAGE HANDLING ==========
DynamicJsonDocument wsMsg(1024);
String wsMsgString;

bool inbox = false;
bool meshInbox = false;
String meshMsgString;

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
  
  // Execute all messages immediately (ignore startTime for now)
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
