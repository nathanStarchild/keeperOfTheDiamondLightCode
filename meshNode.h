// meshNode.h - Multi-bridge mesh network node with WebSocket fallback
// Supports painlessMesh + WebSocket for hybrid AP/mesh connectivity

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
#define MESH_CHANNEL 7  // MUST match Pi hostapd channel!

// Message Protocol
#define PING_MSG_TYPE 999
#define PONG_MSG_TYPE 998
#define ROLE_MSG_TYPE 997
#define MESH_FORWARD_TYPE 996  // Messages forwarded from WebSocket to mesh
#define TO_SERVER_TYPE 995     // Messages from mesh that should go to server

// ========== GLOBAL OBJECTS ==========
Scheduler userScheduler;
painlessMesh mesh;
WebSocketsClient webSocket;

// ========== MESSAGE HANDLING ==========
#define MAX_MSG_LEN 400
DynamicJsonDocument wsMsg(1024);
String wsMsgString;

bool inbox = false;
bool meshInbox = false;
String meshMsgString;

// Pending messages (timed execution)
#define MAX_PENDING_MESSAGES 2
struct PendingMessage {
  String msgString;
  uint64_t startTime;
};

PendingMessage pendingMessages[MAX_PENDING_MESSAGES];
uint8_t pendingMessageCount = 0;

// Bridge election state
bool isBridge = false;
uint32_t currentBridgeNodeId = 0;
int8_t currentBridgeRSSI = -100;  // Signal strength of current bridge
uint64_t lastBridgeAnnounce = 0;
#define BRIDGE_TIMEOUT_US 20000000  // 20 seconds in microseconds

MilliTimer bridgeAnnouncementTimer(5000);  // Announce every 5 seconds
MilliTimer bridgeCheckTimer(30000);        // Check for bridge every 30 seconds

#define MSG_TYPE_BRIDGE_ANNOUNCE 990

// Message deduplication
#define MAX_SEEN_MESSAGES 20
struct SeenMessage {
  uint32_t messageId;
  uint64_t timestamp;
};

SeenMessage seenMessages[MAX_SEEN_MESSAGES];
uint8_t seenMessageIndex = 0;
uint32_t localMessageCounter = 1;  // Only for mesh-originated messages

// ========== STATE TRACKING ==========
// alone = true;  // Not connected to Pi AP
bool meshConnected = false;
bool wsConnected = false;

MilliTimer tryToConnectTimer(1 * 60000);
MilliTimer wsReconnectTimer(5000);

uint32_t pingInterval = 60000;
MilliTimer pingTimer(pingInterval);

int64_t offsetMs = 0;        // smoothed server offset (can be negative)
bool firstOffset = true;
uint64_t lastPingT0 = 0;
bool pingOutstanding = false;

// ========== FORWARD DECLARATIONS ==========
void processWSMessage();  // Defined in main .ino file
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void sendPing();
void handlePong(uint64_t t0, uint64_t serverTime);
void sendRole();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void addSeenMessage(uint32_t msgId);
bool hasSeenMessage(uint32_t msgId);
void pruneSeenMessages();
void forwardToMesh(String &message);
void forwardToWebSocket(String &message);
void checkWebSocketConnection();
void checkBridgeStatus();
void announceBridge();
void handleBridgeMessage(JsonDocument& msg, uint32_t from);

// ========== UTILITY FUNCTIONS ==========

// Add message ID to seen list (circular buffer)
void addSeenMessage(uint32_t msgId) {
  seenMessages[seenMessageIndex].messageId = msgId;
  seenMessages[seenMessageIndex].timestamp = mesh.getNodeTime();
  seenMessageIndex = (seenMessageIndex + 1) % MAX_SEEN_MESSAGES;
}

// Check if we've already processed this message
bool hasSeenMessage(uint32_t msgId) {
  if (msgId == 0) return false;  // No ID means not deduplicated
  
  for (uint8_t i = 0; i < MAX_SEEN_MESSAGES; i++) {
    if (seenMessages[i].messageId == msgId) {
      return true;
    }
  }
  return false;
}

// Remove old entries from seen messages (called periodically)
void pruneSeenMessages() {
  uint64_t now = mesh.getNodeTime();
  uint32_t threshold = 30000000;  // 30 seconds in microseconds
  
  for (uint8_t i = 0; i < MAX_SEEN_MESSAGES; i++) {
    if (now - seenMessages[i].timestamp > threshold) {
      seenMessages[i].messageId = 0;  // Mark as empty
    }
  }
}

// ========== MESH CALLBACKS ==========

// Received data from mesh network
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Mesh: Received from %u: %s\n", from, msg.c_str());
  
  DynamicJsonDocument meshDoc(1024);
  DeserializationError error = deserializeJson(meshDoc, msg);
  
  if (error) {
    Serial.printf("Mesh: JSON parse error: %s\n", error.c_str());
    return;
  }
  
  // Check for bridge protocol messages first
  uint32_t msgType = meshDoc["msgType"] | 0;
  if (msgType == MSG_TYPE_BRIDGE_ANNOUNCE) {
    handleBridgeMessage(meshDoc, from);
    return;  // Don't process as regular message
  }
  
  // Check for message ID (deduplication)
  // Server messages have "msgId", mesh-originated may have local IDs
  uint32_t msgId = meshDoc["msgId"] | 0;
  if (msgId > 0 && hasSeenMessage(msgId)) {
    Serial.printf("Mesh: Ignoring duplicate msgId %u\n", msgId);
    return;
  }
  
  if (msgId > 0) {
    addSeenMessage(msgId);
  }
  
  // Check if this message should be forwarded to server
  bool toServer = meshDoc["toServer"] | false;
  if (toServer && wsConnected) {
    Serial.println("Mesh: Forwarding to WebSocket server");
    forwardToWebSocket(msg);
    return;  // Don't process locally if it's meant for server
  }
  
  // Process message locally
  // Check if message has a startTime
  if (meshDoc.containsKey("startTime")) {
    uint64_t startTime = meshDoc["startTime"];
    // Note: mesh uses microseconds, we need milliseconds
    uint64_t startLocal = startTime / 1000;  // Convert to ms
    uint64_t now = (uint64_t)millis();
    
    if (startLocal <= now) {
      // Execute immediately
      meshMsgString = msg;
      meshInbox = true;
    } else {
      // Queue for later
      if (pendingMessageCount < MAX_PENDING_MESSAGES) {
        pendingMessages[pendingMessageCount].msgString = msg;
        pendingMessages[pendingMessageCount].startTime = startLocal;
        pendingMessageCount++;
        Serial.printf("Mesh: Queued message for %llu (count: %d)\n", startLocal, pendingMessageCount);
      } else {
        Serial.println("Mesh: ERROR - Pending message buffer full!");
      }
    }
  } else {
    // No startTime, execute immediately
    meshMsgString = msg;
    meshInbox = true;
  }
}

// New node joined the mesh
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("Mesh: New connection, nodeId = %u\n", nodeId);
  meshConnected = mesh.getNodeList().size() > 0;
}

// Mesh topology changed
void changedConnectionCallback() {
  Serial.printf("Mesh: Connections changed\n");
  Serial.printf("Mesh: Nodes connected: %d\n", mesh.getNodeList().size());
  
  meshConnected = mesh.getNodeList().size() > 0;
  
  // Check if we should attempt WebSocket connection
  checkWebSocketConnection();
}

// Mesh time was adjusted for sync
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Mesh: Time adjusted by %d us\n", offset);
}

// ========== BRIDGE ELECTION ==========

void announceBridge() {
  if (!wsConnected || !isBridge) {
    return;
  }
  
  int8_t rssi = WiFi.RSSI();
  
  DynamicJsonDocument doc(256);
  doc["msgType"] = MSG_TYPE_BRIDGE_ANNOUNCE;
  doc["bridgeNodeId"] = mesh.getNodeId();
  doc["rssi"] = rssi;
  doc["timestamp"] = mesh.getNodeTime();
  
  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
  
  Serial.printf("Bridge: Announced (RSSI: %d dBm, nodes: %d)\n", rssi, mesh.getNodeList().size());
}

void handleBridgeMessage(JsonDocument& msg, uint32_t from) {
  uint32_t msgType = msg["msgType"];
  
  if (msgType == MSG_TYPE_BRIDGE_ANNOUNCE) {
    uint32_t bridgeNodeId = msg["bridgeNodeId"];
    int8_t bridgeRSSI = msg["rssi"];
    uint64_t timestamp = msg["timestamp"];
    
    currentBridgeNodeId = bridgeNodeId;
    currentBridgeRSSI = bridgeRSSI;
    lastBridgeAnnounce = timestamp;
    
    Serial.printf("Bridge: Heard announce from %u (RSSI: %d dBm)\n", bridgeNodeId, bridgeRSSI);
    
    // If we're bridge but someone else announced with better signal, step down
    if (isBridge && bridgeNodeId != mesh.getNodeId()) {
      Serial.printf("Bridge: Conflict! Node %u also claims bridge\n", bridgeNodeId);
      
      int8_t myRSSI = WiFi.RSSI();
      if (bridgeRSSI > myRSSI + 5) {  // Other bridge has significantly better signal
        Serial.printf("Bridge: Stepping down (their RSSI %d > my RSSI %d)\n", bridgeRSSI, myRSSI);
        isBridge = false;
      } else if (bridgeNodeId < mesh.getNodeId()) {
        // Equal signal, defer to lower node ID
        Serial.printf("Bridge: Stepping down (tie-breaker: lower node ID)\n");
        isBridge = false;
      }
    }
  }
}

void checkBridgeStatus() {
  uint64_t now = mesh.getNodeTime();
  
  // If we're already connected or bridge, nothing to do
  if (wsConnected || isBridge) {
    return;
  }
  
  // Check if it's time to try becoming bridge
  if (bridgeCheckTimer.isItTime()) {
    bridgeCheckTimer.resetTimer();
    
    // Check if there's no bridge or bridge timed out
    bool shouldClaim = false;
    
    if (currentBridgeNodeId == 0) {
      Serial.println("Bridge: No bridge exists, attempting to claim");
      shouldClaim = true;
    }
    else if (now - lastBridgeAnnounce > BRIDGE_TIMEOUT_US) {
      Serial.printf("Bridge: Bridge %u timed out, attempting to claim\n", currentBridgeNodeId);
      shouldClaim = true;
    }
    
    // Try to connect to Pi AP if we should become bridge
    if (shouldClaim) {
      Serial.println("Bridge: Attempting to connect to Pi AP...");
      mesh.stationManual(PI_SSID, WIFI_KEY);
    }
  }
  
  // If we just connected to Pi, claim bridge role
  if (wsConnected && !isBridge) {
    int8_t myRSSI = WiFi.RSSI();
    Serial.printf("Bridge: Connected to Pi, claiming bridge role (RSSI: %d dBm)\n", myRSSI);
    isBridge = true;
    currentBridgeNodeId = mesh.getNodeId();
    currentBridgeRSSI = myRSSI;
    announceBridge();
  }
  
  // If we're bridge but lost Pi connection, step down immediately
  if (isBridge && !wsConnected) {
    Serial.println("Bridge: Lost Pi connection, stepping down and disconnecting");
    isBridge = false;
    if (currentBridgeNodeId == mesh.getNodeId()) {
      currentBridgeNodeId = 0;
      currentBridgeRSSI = -100;
    }
    // Disconnect from Pi AP
    mesh.stationManual("", "");  // Empty credentials disconnects
  }
  
  // Periodic announcements if we're bridge
  if (isBridge && bridgeAnnouncementTimer.isItTime()) {
    announceBridge();
    bridgeAnnouncementTimer.resetTimer();
  }
}

// ========== MESSAGE FORWARDING ==========

// Forward WebSocket message to mesh network (only if bridge)
void forwardToMesh(String &message) {
  // Only forward if we're the active bridge
  if (!isBridge) {
    return;
  }
  
  if (!meshConnected && mesh.getNodeList().size() == 0) {
    Serial.println("Forward: No mesh nodes connected, skipping");
    return;
  }
  
  // Parse the message
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.printf("Forward: JSON parse error: %s\n", error.c_str());
    return;
  }
  
  // Use server-provided msgId for deduplication
  uint32_t msgId = 0;
  if (doc.containsKey("msgId")) {
    msgId = doc["msgId"];
  } else {
    // No server ID - generate local ID as fallback
    msgId = localMessageCounter++;
    doc["msgId"] = msgId;
    Serial.println("Forward: Warning - no server msgId, using local ID");
  }
  
  // Check if we've already seen this message
  if (hasSeenMessage(msgId)) {
    Serial.printf("Forward: Not forwarding msgId %u, already seen\n", msgId);
    return;
  }
  
  addSeenMessage(msgId);
  
  String meshMsg;
  serializeJson(doc, meshMsg);
  
  mesh.sendBroadcast(meshMsg);
  Serial.printf("Forward: Sent msgId %u to mesh (%d nodes)\n", msgId, mesh.getNodeList().size());
}

// Forward mesh message to WebSocket server
void forwardToWebSocket(String &message) {
  if (!wsConnected) {
    Serial.println("Forward: WebSocket not connected");
    return;
  }
  
  webSocket.sendTXT(message);
  Serial.println("Forward: Sent to WebSocket server");
}

// ========== WEBSOCKET HANDLING ==========

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
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
      // Fall through to handle like text
      
    case WStype_TEXT:
      Serial.printf("WebSocket: Text: %s\n", payload);
      
      error = deserializeJson(wsMsg, payload);
      if (error) {
        Serial.printf("WebSocket: JSON parse error: %s\n", error.f_str());
      } else {
        Serial.print("WebSocket: Valid JSON received: ");
        serializeJson(wsMsg, Serial);
        Serial.println();

        // Handle PONG (time sync with server)
        if (wsMsg["msgType"] == PONG_MSG_TYPE) {
          uint64_t t0 = wsMsg["t0"];
          uint64_t serverTime = wsMsg["serverTime"];
          handlePong(t0, serverTime);
          return;
        }

        // Forward to mesh if we have connections (and are a bridge)
        if (meshConnected || mesh.getNodeList().size() > 0) {
          wsMsgString = "";
          serializeJson(wsMsg, wsMsgString);
          forwardToMesh(wsMsgString);
        }

        // Check if message has a startTime
        if (wsMsg.containsKey("startTime")) {
          uint64_t startTime = wsMsg["startTime"];
          // Convert server time to local ESP time
          int64_t startLocal = (int64_t)startTime - offsetMs;
          uint64_t now = (uint64_t)millis();
          
          if (startLocal <= (int64_t)now) {
            // Execute immediately
            wsMsgString = "";
            serializeJson(wsMsg, wsMsgString);
            inbox = true;
          } else {
            // Queue for later
            if (pendingMessageCount < MAX_PENDING_MESSAGES) {
              pendingMessages[pendingMessageCount].msgString = "";
              serializeJson(wsMsg, pendingMessages[pendingMessageCount].msgString);
              pendingMessages[pendingMessageCount].startTime = startLocal;
              pendingMessageCount++;
              Serial.printf("WebSocket: Queued for %lld, current %llu (count: %d)\n", 
                           startLocal, now, pendingMessageCount);
            } else {
              Serial.println("WebSocket: ERROR - Pending buffer full!");
            }
          }
        } else {
          // No startTime, execute immediately
          wsMsgString = "";
          serializeJson(wsMsg, wsMsgString);
          inbox = true;
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
  if (pingOutstanding) return;   // don't overlap

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
  int64_t newOffset = (int64_t)serverTime - (int64_t)now + (int64_t)(rtt / 2);

  Serial.printf("Pong: t0=%llu, now=%llu, serverTime=%llu, rtt=%llu\n", 
                t0, now, serverTime, rtt);

  if (firstOffset) {
    offsetMs = newOffset;
    firstOffset = false;
    Serial.printf("Pong: First offset set: %lld ms\n", offsetMs);
  } else {
    int64_t delta = llabs(newOffset - offsetMs);
    if (delta > 100) {
      Serial.printf("Pong: Large jump (%lld ms), snapping\n", delta);
      offsetMs = newOffset;
    } else {
      offsetMs = (int64_t)(offsetMs * 0.9 + newOffset * 0.1);
    }
    Serial.printf("Pong: Offset=%lld ms (delta: %lld ms)\n", offsetMs, newOffset - offsetMs);
  }
}

// Check if we should connect/reconnect WebSocket
void checkWebSocketConnection() {
  if (WiFi.status() == WL_CONNECTED && !wsConnected && !alone) {
    if (wsReconnectTimer.isItTime()) {
      Serial.println("WebSocket: Attempting connection...");
      webSocket.begin(WS_SERVER, WS_PORT, WS_PATH);
      webSocket.onEvent(webSocketEvent);
      wsReconnectTimer.resetTimer();
    }
  }
}

// ========== SETUP AND LOOP ==========

void wsSetup() {
  Serial.println("meshNode: Initializing...");
  
  // Initialize mesh network
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  
  // Initialize mesh - don't specify channel, let mesh handle it
  // Channel will be set by bridge when it connects to Pi AP
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA);
  
  // DON'T connect to Pi AP yet - only bridge should connect
  // Bridge election will trigger connection for winning node
  mesh.setHostname("DiamondLight");
  
  // Enable automatic bridge behavior
  mesh.setRoot(false);  // All nodes can participate as bridges
  mesh.setContainsRoot(true);  // Network can contain root node
  
  // Register mesh callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  Serial.printf("meshNode: Mesh ID = %u\n", mesh.getNodeId());
  Serial.println("meshNode: Mesh-only mode, will scan for Pi AP periodically");
  
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
  
  // Disable WiFi sleep for better performance
  WiFi.setSleep(false);
}

void wsLoop() {
  // Update mesh network
  mesh.update();
  
  // Handle ArduinoOTA
  ArduinoOTA.handle();
  
  // Check and manage bridge status
  checkBridgeStatus();
  
  // Update connection state
  if (WiFi.status() == WL_CONNECTED) {
    if (alone) {
      alone = false;
      firstOffset = true;
      Serial.println("meshNode: Connected to Pi AP");
      Serial.printf("meshNode: IP = %s\n", WiFi.localIP().toString().c_str());
      
      // Start WebSocket connection
      webSocket.begin(WS_SERVER, WS_PORT, WS_PATH);
      webSocket.onEvent(webSocketEvent);
    }
  } else {
    if (!alone) {
      alone = true;
      wsConnected = false;
      Serial.println("meshNode: Disconnected from Pi AP (mesh only mode)");
    }
  }
  
  // Handle WebSocket
  if (!alone) {
    webSocket.loop();
    
    // Check for WebSocket reconnection
    checkWebSocketConnection();
    
    // Send periodic ping
    if (wsConnected && pingTimer.isItTime()) {
      sendPing();
      pingTimer.setInterval(pingInterval + random(-5000, 5000));
    }
  }
  
  // Process pending messages
  if (pendingMessageCount > 0) {
    uint64_t now = (uint64_t)millis();
    for (uint8_t i = 0; i < pendingMessageCount; i++) {
      if (pendingMessages[i].startTime <= now) {
        wsMsgString = pendingMessages[i].msgString;
        inbox = true;
        Serial.printf("meshNode: Executing pending message (count: %d)\n", pendingMessageCount);
        
        // Shift remaining messages down
        for (uint8_t j = i; j < pendingMessageCount - 1; j++) {
          pendingMessages[j] = pendingMessages[j + 1];
        }
        pendingMessageCount--;
        break;  // Process one per loop iteration
      }
    }
  }
  
  // Process incoming WebSocket message
  if (inbox) {
    processWSMessage();
    inbox = false;
  }
  
  // Process incoming mesh message
  if (meshInbox) {
    // Parse mesh message and copy to wsMsg/wsMsgString for processing
    DeserializationError error = deserializeJson(wsMsg, meshMsgString);
    if (!error) {
      wsMsgString = meshMsgString;
      processWSMessage();
    } else {
      Serial.printf("meshNode: Error parsing mesh message: %s\n", error.c_str());
    }
    meshInbox = false;
  }
  
  // Periodically prune seen messages (every 30 seconds)
  static uint32_t lastPrune = 0;
  if (millis() - lastPrune > 30000) {
    pruneSeenMessages();
    lastPrune = millis();
  }
}
