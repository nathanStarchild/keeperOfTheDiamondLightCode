// meshBridge.h - Dedicated bridge between Pi WebSocket server and ESP mesh network
// This ESP connects to Pi AP and forwards messages to/from mesh

// painlessMesh library (includes WiFi internally)
#include "painlessMesh.h"

#include <ArduinoOTA.h>
#include <WebSocketsClient.h>

//https://arduinojson.org/v6/api/config/use_long_long/
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

// ========== CONFIGURATION ==========
// Pi Access Point Settings
#define PI_SSID "diamondLightNetwork"
#define WIFI_KEY ""
#define WS_SERVER "200.200.200.1"
#define WS_PORT 80
#define WS_PATH "/"

// Mesh Network Settings
#define MESH_SSID "diamondLightMesh"
#define MESH_PASSWORD "keeperOfTheDiamondLight"
#define MESH_PORT 5555
#define MESH_CHANNEL 7

// Message Protocol
#define PING_MSG_TYPE 999
#define PONG_MSG_TYPE 998
#define ROLE_MSG_TYPE 997

// ========== GLOBAL OBJECTS ==========
painlessMesh mesh;
WebSocketsClient webSocket;

// ========== STATE TRACKING ==========
bool meshConnected = false;
bool wsConnected = false;

MilliTimer wsReconnectTimer(5000);

uint32_t pingInterval = 60000;
MilliTimer pingTimer(pingInterval);

int64_t offsetUs = 0;  // Offset between server time (ms) and mesh time (us): serverTime*1000 - meshTime
bool firstOffset = true;
uint64_t lastPingT0 = 0;
bool pingOutstanding = false;
// bool alone = true;

 DynamicJsonDocument wsMsg(32);
 String wsMsgString;

// ========== FORWARD DECLARATIONS ==========
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void sendPing();
void handlePong(uint64_t t0, uint64_t serverTime);
void sendRole();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void forwardToMesh(String &message);
void forwardToWebSocket(String &message);

// ========== MESH CALLBACKS ==========

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Mesh: Received from %u: %s\n", from, msg.c_str());
  
  DynamicJsonDocument meshDoc(1024);
  DeserializationError error = deserializeJson(meshDoc, msg);
  
  if (error) {
    Serial.printf("Mesh: JSON parse error: %s\n", error.c_str());
    return;
  }
  
  // Check if this message should be forwarded to server
  bool toServer = meshDoc["toServer"] | false;
  if (toServer && wsConnected) {
    Serial.println("Mesh: Forwarding to WebSocket server");
    forwardToWebSocket(msg);
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

// ========== MESSAGE FORWARDING ==========

void forwardToMesh(String &message) {
  if (!meshConnected && mesh.getNodeList().size() == 0) {
    Serial.println("Bridge: No mesh nodes connected, skipping forward");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.printf("Bridge: JSON parse error: %s\n", error.c_str());
    mesh.sendBroadcast(message);  // Forward as-is if can't parse
    return;
  }
  
  // Convert startTime from server time (ms) to mesh time (us)
  if (doc.containsKey("startTime")) {
    uint64_t serverStartTime = doc["startTime"];  // in milliseconds
    uint64_t meshStartTime = (serverStartTime * 1000) - offsetUs;  // convert to mesh microseconds
    doc["startTime"] = meshStartTime;
    Serial.printf("Bridge: Converted startTime %llu ms (server) to %llu us (mesh)\n", serverStartTime, meshStartTime);
  }
  
  String meshMsg;
  serializeJson(doc, meshMsg);
  mesh.sendBroadcast(meshMsg);
  Serial.printf("Bridge: Forwarded to mesh (%d nodes)\n", mesh.getNodeList().size());
}

void forwardToWebSocket(String &message) {
  if (!wsConnected) {
    Serial.println("Bridge: WebSocket not connected");
    return;
  }
  
  webSocket.sendTXT(message);
  Serial.println("Bridge: Forwarded to WebSocket server");
}

// ========== WEBSOCKET HANDLING ==========

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  DynamicJsonDocument wsDoc(1024);
  DeserializationError error;
  Serial.printf("\n=== WS EVENT ===\n");
  Serial.printf("Type: %d, Length: %d\n", type, length);
  Serial.printf("================\n");

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("WebSocket: Disconnected!\n");
      wsConnected = false;
      break;

    case WStype_CONNECTED:
      Serial.printf("WebSocket: Connected to %s\n", payload);
      wsConnected = true;

      #ifdef ROLE
      sendRole();
      #endif
      sendPing();
      webSocket.sendTXT("lockState");
      break;

    case WStype_BIN:
      Serial.printf("WebSocket: Binary data (treating as text): length=%d\n", length);
      
    case WStype_TEXT:
      Serial.printf("WebSocket: Text: %s\n", payload);
      
      error = deserializeJson(wsDoc, payload);
      if (error) {
        Serial.printf("WebSocket: JSON parse error: %s\n", error.f_str());
      } else {
        Serial.print("WebSocket: Valid JSON received: ");
        serializeJson(wsDoc, Serial);
        Serial.println();

        // Handle PONG (time sync with server)
        if (wsDoc["msgType"] == PONG_MSG_TYPE) {
          uint64_t t0 = wsDoc["t0"];
          uint64_t serverTime = wsDoc["serverTime"];
          handlePong(t0, serverTime);
          return;
        }

        // Forward to mesh if we have connections
        if (meshConnected || mesh.getNodeList().size() > 0) {
          String wsMsgString = "";
          serializeJson(wsDoc, wsMsgString);
          forwardToMesh(wsMsgString);
        }
      }
      break;
  }
}

#ifdef ROLE
void sendRole() {
  char buf[128];
  snprintf(buf, sizeof(buf),
    "{\"msgType\":%d,\"role\":\"%s\"}",
    ROLE_MSG_TYPE,
    ROLE
  );
  webSocket.sendTXT(buf);
}
#endif

void sendPing() {
  if (!wsConnected) return;
  if (pingOutstanding) return;

  lastPingT0 = (uint64_t)millis();
  pingOutstanding = true;

  char buf[128];
  snprintf(buf, sizeof(buf),
    "{\"msgType\":%d,\"t0\":%llu}",
    PING_MSG_TYPE,
    lastPingT0
  );

  webSocket.sendTXT(buf);
}

void handlePong(uint64_t t0, uint64_t serverTime) {
  uint64_t now = (uint64_t)millis();

  if (!pingOutstanding) {
    Serial.println("Pong: Ignoring (no ping outstanding)");
    return;
  }
  if (t0 != lastPingT0) {
    Serial.println("Pong: Ignoring stale pong");
    return;
  }

  pingOutstanding = false;

  uint64_t rtt = now - t0;
  uint64_t serverTimeAdjusted = serverTime + (rtt / 2);  // Estimate current server time
  uint64_t meshTime = mesh.getNodeTime();  // in microseconds
  int64_t newOffset = (int64_t)(serverTimeAdjusted * 1000) - (int64_t)meshTime;

  Serial.printf("Pong: serverTime=%llu ms, meshTime=%llu us, rtt=%llu ms\n", 
                serverTimeAdjusted, meshTime, rtt);

  if (firstOffset) {
    offsetUs = newOffset;
    firstOffset = false;
    Serial.printf("Pong: First offset set: %lld us\n", offsetUs);
  } else {
    int64_t delta = llabs(newOffset - offsetUs);
    if (delta > 100000) {  // 100ms in microseconds
      Serial.printf("Pong: Large jump (%lld us), snapping\n", delta);
      offsetUs = newOffset;
    } else {
      offsetUs = (int64_t)(offsetUs * 0.9 + newOffset * 0.1);
    }
    Serial.printf("Pong: Offset=%lld us (delta: %lld us)\n", offsetUs, newOffset - offsetUs);
  }
}

// ========== SETUP AND LOOP ==========

void wsSetup() {
  Serial.println("meshBridge: Initializing...");
  
  // Initialize mesh network
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Initialize mesh
  mesh.init(MESH_SSID, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  
  // Connect to Pi AP in station mode
  mesh.stationManual(PI_SSID, WIFI_KEY);
//   mesh.setHostname("DiamondLightBridge");
  
  // This device acts as the mesh root/bridge
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
  // Register mesh callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  Serial.printf("meshBridge: Mesh ID = %u\n", mesh.getNodeId());
  Serial.println("meshBridge: Connecting to Pi AP and establishing mesh...");
  
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
  
  Serial.println("meshBridge: Setup complete");
  
  WiFi.setSleep(false);
}

void wsLoop() {
  // Update mesh network
  mesh.update();
  
  // Handle ArduinoOTA
  ArduinoOTA.handle();
  
  // Handle WiFi connection state
  if (WiFi.status() == WL_CONNECTED) {
    if (alone) {
      alone = false;
      firstOffset = true;
      Serial.println("meshBridge: Connected to Pi AP");
      Serial.printf("meshBridge: IP = %s\n", WiFi.localIP().toString().c_str());
      
      // Start WebSocket connection
      webSocket.begin(WS_SERVER, WS_PORT, WS_PATH);
      webSocket.onEvent(webSocketEvent);
    }
  } else {
    if (!alone) {
      alone = true;
      wsConnected = false;
      Serial.println("meshBridge: Disconnected from Pi AP");
    }
  }
  
  // Handle WebSocket
  if (!alone) {
    webSocket.loop();
    
    // Reconnect if needed
    if (!wsConnected && wsReconnectTimer.isItTime()) {
      Serial.println("meshBridge: Attempting WebSocket reconnection...");
      webSocket.begin(WS_SERVER, WS_PORT, WS_PATH);
      webSocket.onEvent(webSocketEvent);
      wsReconnectTimer.resetTimer();
    }
    
    // Send periodic ping
    if (wsConnected && pingTimer.isItTime()) {
      sendPing();
      pingTimer.setInterval(pingInterval + random(-5000, 5000));
    }
  }
}
