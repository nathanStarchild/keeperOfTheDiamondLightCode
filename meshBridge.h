// meshBridge.h - Dedicated bridge between Pi WebSocket server and ESP mesh network
// This ESP connects to Pi AP and forwards messages to/from mesh

// painlessMesh library (includes WiFi internally)
#include "painlessMesh.h"

#include <ArduinoOTA.h>
#include <WebSocketsClient.h>

//https://arduinojson.org/v6/api/config/use_long_long/
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

extern bool debugging;

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

// Role Definition
#define ROLE "bridge"

// ========== GLOBAL OBJECTS ==========
painlessMesh mesh;
WebSocketsClient webSocket;
extern bool debugging;

// ========== STATE TRACKING ==========
bool meshConnected = false;
bool wsConnected = false;
uint16_t nonMeshDeviceCount = 0;  // Pyramid and other direct-connected devices

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
void announceBridge();
void reportNodeCount();

// ========== MESH CALLBACKS ==========

void receivedCallback(uint32_t from, String &msg) {
  if (debugging) {
    Serial.printf("Mesh: Received from %u: %s\n", from, msg.c_str());
  }
  
  if (!wsConnected) {
    return;
  }
  
  DynamicJsonDocument meshDoc(1024);
  DeserializationError error = deserializeJson(meshDoc, msg);
  
  if (error) {
    if (debugging) {
      Serial.printf("Mesh: JSON parse error: %s\n", error.c_str());
    }
    // Forward as-is even if parse fails
    forwardToWebSocket(msg);
    return;
  }
  
  // Convert startTime from mesh time (us) to server time (ms) if present
  if (meshDoc.containsKey("startTime")) {
    uint64_t meshStartTime = meshDoc["startTime"];  // in microseconds
    uint64_t serverStartTime = ((int64_t)meshStartTime + offsetUs) / 1000;  // convert to server milliseconds
    meshDoc["startTime"] = serverStartTime;
    if (debugging) {
      Serial.printf("Mesh: Converted startTime %llu us (mesh) to %llu ms (server)\n", meshStartTime, serverStartTime);
    }
  }
  
  // Serialize and forward to server
  String serverMsg = "";
  serializeJson(meshDoc, serverMsg);
  forwardToWebSocket(serverMsg);
  if (debugging) {
    Serial.println("Mesh: Forwarded to WebSocket server");
  }
}

void newConnectionCallback(uint32_t nodeId) {
  if (debugging) {
    Serial.printf("Mesh: New connection, nodeId = %u\n", nodeId);
  }
  meshConnected = mesh.getNodeList().size() > 0;
}

void changedConnectionCallback() {
  if (debugging) {
    Serial.printf("Mesh: Connections changed\n");
    Serial.printf("Mesh: Nodes connected: %d\n", mesh.getNodeList().size());
  }
  meshConnected = mesh.getNodeList().size() > 0;
  
  // Announce bridge identity when mesh topology changes
  if (meshConnected) {
    announceBridge();
  }
  
  // Report node count to server
  reportNodeCount();
}

void nodeTimeAdjustedCallback(int32_t offset) {
  if (debugging) {
    Serial.printf("Mesh: Time adjusted by %d us\n", offset);
  }
}

// ========== BRIDGE ANNOUNCEMENT ==========

void announceBridge() {
  char msg[64];
  snprintf(msg, sizeof(msg), "{\"msgType\":%d,\"nonMeshCount\":%u}", 
           BRIDGE_ANNOUNCE_MSG_TYPE, nonMeshDeviceCount);
  mesh.sendBroadcast(msg);
  if (debugging) {
    Serial.printf("Bridge: Announced to mesh (nodeId %u, %u non-mesh devices)\n", 
                  mesh.getNodeId(), nonMeshDeviceCount);
  }
}

void reportNodeCount() {
  if (!wsConnected) {
    return;
  }
  
  uint16_t nodeCount = mesh.getNodeList().size();  // Excludes bridge itself
  
  char msg[64];
  snprintf(msg, sizeof(msg), "{\"msgType\":%d,\"nodeCount\":%u}", NODE_COUNT_MSG_TYPE, nodeCount);
  webSocket.sendTXT(msg);
  if (debugging) {
    Serial.printf("Bridge: Reported %u mesh nodes to server\n", nodeCount);
  }
}

// ========== MESSAGE FORWARDING ==========

void forwardToMesh(String &message) {
  if (!meshConnected && mesh.getNodeList().size() == 0) {
    if (debugging) {
      Serial.println("Bridge: No mesh nodes connected, skipping forward");
    }
    return;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    if (debugging) {
      Serial.printf("Bridge: JSON parse error: %s\n", error.c_str());
    }
    mesh.sendBroadcast(message);  // Forward as-is if can't parse
    return;
  }
  
  // Convert startTime from server time (ms) to mesh time (us)
  if (doc.containsKey("startTime")) {
    uint64_t serverStartTime = doc["startTime"];  // in milliseconds
    uint64_t meshStartTime = (serverStartTime * 1000) - offsetUs;  // convert to mesh microseconds
    doc["startTime"] = meshStartTime;
    if (debugging) {
      Serial.printf("Bridge: Converted startTime %llu ms (server) to %llu us (mesh)\n", serverStartTime, meshStartTime);
    }
  }
  
  String meshMsg = "";
  serializeJson(doc, meshMsg);
  mesh.sendBroadcast(meshMsg);
  if (debugging) {
    Serial.printf("Bridge: Forwarded to mesh (%d nodes)\n", mesh.getNodeList().size());
  }
}

void forwardToWebSocket(String &message) {
  if (!wsConnected) {
    if (debugging) {
      Serial.println("Bridge: WebSocket not connected");
    }
    return;
  }
  
  webSocket.sendTXT(message);
  if (debugging) {
    Serial.println("Bridge: Forwarded to WebSocket server");
  }
}

// ========== WEBSOCKET HANDLING ==========

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  DynamicJsonDocument wsDoc(1024);
  DeserializationError error;
  if (debugging) {
    Serial.printf("\n=== WS EVENT ===\n");
    Serial.printf("Type: %d, Length: %d\n", type, length);
    Serial.printf("================\n");
  }

  switch (type) {
    case WStype_DISCONNECTED:
      if (debugging) {
        Serial.printf("WebSocket: Disconnected!\n");
      }
      wsConnected = false;
      break;

    case WStype_CONNECTED:
      if (debugging) {
        Serial.printf("WebSocket: Connected to %s\n", payload);
      }
      wsConnected = true;

      #ifdef ROLE
      sendRole();
      #endif
      sendPing();
      webSocket.sendTXT("lockState");
      break;

    case WStype_BIN:
      if (debugging) {
        Serial.printf("WebSocket: Binary data (treating as text): length=%d\n", length);
      }
      
    case WStype_TEXT:
      if (debugging) {
        Serial.printf("WebSocket: Text: %s\n", payload);
      }
      
      error = deserializeJson(wsDoc, payload);
      if (error) {
        if (debugging) {
          Serial.printf("WebSocket: JSON parse error: %s\n", error.f_str());
        }
      } else {
        if (debugging) {
          Serial.print("WebSocket: Valid JSON received: ");
          serializeJson(wsDoc, Serial);
          Serial.println();
        }

        // Handle PONG (time sync with server)
        if (wsDoc["msgType"] == PONG_MSG_TYPE) {
          uint64_t t0 = wsDoc["t0"];
          uint64_t serverTime = wsDoc["serverTime"];
          handlePong(t0, serverTime);
          return;
        }
        
        // Handle TOTAL_LED_COUNT (update non-mesh device count)
        if (wsDoc["msgType"] == TOTAL_LED_COUNT_MSG_TYPE) {
          uint16_t pyramidCount = wsDoc["pyramidCount"];
          
          // Only re-announce if count changed
          if (pyramidCount != nonMeshDeviceCount) {
            nonMeshDeviceCount = pyramidCount;
            if (debugging) {
              Serial.printf("Bridge: Non-mesh device count changed to %u - re-announcing\n", nonMeshDeviceCount);
            }
            announceBridge();
          } else {
            if (debugging) {
              Serial.printf("Bridge: Non-mesh device count unchanged (%u) - no re-announce\n", nonMeshDeviceCount);
            }
          }
          
          return;  // Don't forward this message to mesh
        }

        // Forward to mesh if we have connections
        if (meshConnected || mesh.getNodeList().size() > 0) {
          wsMsgString = "";
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
    if (debugging) {
      Serial.println("Pong: Ignoring (no ping outstanding)");
    }
    return;
  }
  if (t0 != lastPingT0) {
    if (debugging) {
      Serial.println("Pong: Ignoring stale pong");
    }
    return;
  }

  pingOutstanding = false;

  uint64_t rtt = now - t0;
  uint64_t serverTimeAdjusted = serverTime + (rtt / 2);  // Estimate current server time
  uint64_t meshTime = mesh.getNodeTime();  // in microseconds
  int64_t newOffset = (int64_t)(serverTimeAdjusted * 1000) - (int64_t)meshTime;

  if (debugging) {
    Serial.printf("Pong: serverTime=%llu ms, meshTime=%llu us, rtt=%llu ms\n", 
                  serverTimeAdjusted, meshTime, rtt);
  }

  if (firstOffset) {
    offsetUs = newOffset;
    firstOffset = false;
    if (debugging) {
      Serial.printf("Pong: First offset set: %lld us\n", offsetUs);
    }
  } else {
    int64_t delta = llabs(newOffset - offsetUs);
    if (delta > 100000) {  // 100ms in microseconds
      if (debugging) {
        Serial.printf("Pong: Large jump (%lld us), snapping\n", delta);
      }
      offsetUs = newOffset;
    } else {
      offsetUs = (int64_t)(offsetUs * 0.9 + newOffset * 0.1);
    }
    if (debugging) {
      Serial.printf("Pong: Offset=%lld us (delta: %lld us)\n", offsetUs, newOffset - offsetUs);
    }
  }
}

// ========== SETUP AND LOOP ==========

void wsSetup() {
  Serial.println("meshBridge: Initializing...");
  
  // Initialize mesh network
  if (debugging) {
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  } else {
    mesh.setDebugMsgTypes(ERROR | STARTUP);
  }
  
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
  
  Serial.println("meshBridge: Setup complete");
  
  WiFi.setSleep(false);
}

void wsLoop() {
  // Update mesh network
  mesh.update();
  
  // Handle ArduinoOTA
  // ArduinoOTA.handle();
  
  // Handle WiFi connection state
  if (WiFi.status() == WL_CONNECTED) {
    if (alone) {
      alone = false;
      firstOffset = true;
      if (debugging) {
        Serial.println("meshBridge: Connected to Pi AP");
        Serial.printf("meshBridge: IP = %s\n", WiFi.localIP().toString().c_str());
      }
      
      // Start WebSocket connection
      webSocket.begin(WS_SERVER, WS_PORT, WS_PATH);
      webSocket.onEvent(webSocketEvent);
    }
  } else {
    if (!alone) {
      alone = true;
      wsConnected = false;
      if (debugging) {
        Serial.println("meshBridge: Disconnected from Pi AP");
      }
    }
  }
  
  // Handle WebSocket
  if (!alone) {
    webSocket.loop();
    
    // Reconnect if needed
    if (!wsConnected && wsReconnectTimer.isItTime()) {
      if (debugging) {
        Serial.println("meshBridge: Attempting WebSocket reconnection...");
      }
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
