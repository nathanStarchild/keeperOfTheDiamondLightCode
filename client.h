#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif

#include <WiFiUdp.h>
#include <ArduinoOTA.h>


#include <WebSocketsClient.h>

//https://arduinojson.org/v6/api/config/use_long_long/
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

// Network configuration: set to "Pi" or "Router"
#define SERVER "Pi"

#if SERVER == "Pi"
  // Pi Access Point configuration (for pyramid installations and other direct-connected devices)
  #define SSID "diamondLightNetwork"
  #define WIFI_KEY ""
  #define WS_SERVER "200.200.200.1"
#else
  // Router configuration (for development/testing)
  #define SSID "queerBurners"
  #define WIFI_KEY "nextyearwasbetter"
  #define WS_SERVER "192.168.0.10"
#endif

#define WS_PORT 80
#define WS_PATH "/"


WebSocketsClient webSocket;

#define MAX_MSG_LEN 400
DynamicJsonDocument wsMsg(1024);
String wsMsgString;

MilliTimer tryToConnectTimer(1 * 60000);

bool inbox = false;
#define MAX_PENDING_MESSAGES 2
struct PendingMessage {
  String msgString;
  uint64_t startTime;
};

PendingMessage pendingMessages[MAX_PENDING_MESSAGES];
uint8_t pendingMessageCount = 0;

uint32_t pingInterval = 60000;
MilliTimer pingTimer(pingInterval);

int64_t offsetMs = 0;        // smoothed server offset (can be negative)
bool firstOffset = true;
uint64_t lastPingT0 = 0;
bool pingOutstanding = false;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void processWSMessage();
void sendPing();
void handlePong(uint64_t t0, uint64_t serverTime);
void sendRole();
void handleSweepSpotAssignment(uint16_t newSweepSpot);
void handleTotalLEDCount(uint16_t totalCount);

// External references to main .ino functions
extern void nodeCounter();  // Defined in main .ino file
extern MilliTimer nodeCountTimer;  // Defined in main .ino file
extern uint16_t sweepSpot;  // Defined in element definition
extern MainState mainState;  // Defined in main .ino file

#ifdef ESP8266
WiFiEventHandler stationConnectedHandler;
void WiFiGotIP(const WiFiEventStationModeGotIP& event);
WiFiEventHandler stationDisconnectedHandler;
void WiFiStationDisconnected(const WiFiEventStationModeDisconnected& event);
#else
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
#endif

size_t safeStrCopy(char* dest, const uint8_t* src, size_t srcLen, size_t maxLen) {
    // Clamp number of bytes to copy
    size_t n = srcLen;
    if (n >= maxLen) n = maxLen - 1;

    // Copy bytes
    for (size_t i = 0; i < n; i++) {
        dest[i] = (char)src[i];
    }

    // Null-terminate
    dest[n] = '\0';

    // Return number of bytes copied (excluding '\0')
    return n;
}

void wsSetup() {

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.setSleep(false);
  
  WiFi.begin(SSID, WIFI_KEY);

  #ifdef ESP8266
    stationConnectedHandler = WiFi.onStationModeGotIP(&WiFiGotIP);
    stationDisconnectedHandler = WiFi.onStationModeDisconnected(&WiFiStationDisconnected);
  #else
    WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    //  WiFi.onEvent(WiFiStationConnected,ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  #endif

  // No authentication by default
  ArduinoOTA.setPassword("antares");
  ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    });
  ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
   int retries = 0;
   while (WiFi.status() != WL_CONNECTED && retries < 20) {
     delay(500);
     Serial.print(".");
     retries ++;
   }
   if (WiFi.status() != WL_CONNECTED){
     Serial.println("Couldn't connect to network");
     alone = true;
   }
}

void wsLoop() {

    if(alone) {
      if (tryToConnectTimer.isItTime()) {
        Serial.println("tryin to connect");
        WiFi.begin(SSID, WIFI_KEY);
        tryToConnectTimer.resetTimer();
      } 
      return;

    }

    ArduinoOTA.handle();

    webSocket.loop();

  // Check for pending messages that are ready to execute
  if (pendingMessageCount > 0) {
    uint64_t now = (uint64_t)millis();
    for (uint8_t i = 0; i < pendingMessageCount; i++) {
      if (pendingMessages[i].startTime <= now) {
//        size_t n = min(pendingMessages[i].length, (size_t)(MAX_MSG_LEN - 1));
//        memcpy(wsMsgString, pendingMessages[i].msgString, n);
//        wsMsgString[n] = '\0';
//        safeStrCopy(wsMsgString, (const uint8_t*)pendingMessages[i].msgString, pendingMessages[i].length, MAX_MSG_LEN);
        wsMsgString = "";
        wsMsgString = pendingMessages[i].msgString;
        inbox = true;
        Serial.printf("Executing pending message (startTime: %llu)\n", pendingMessages[i].startTime);
        
        // Shift remaining messages down
        for (uint8_t j = i; j < pendingMessageCount - 1; j++) {
          pendingMessages[j] = pendingMessages[j + 1];
        }
        pendingMessageCount--;
        break;  // Process one per loop iteration
      }
    }
  }

  if (inbox) {
    processWSMessage();
    inbox = false;
  }

  if (pingTimer.isItTime()) {
    sendPing();
    pingTimer.setInterval(pingInterval + random(-5000, 5000)); //also resets the timer
  }

}

#ifdef ESP8266
void WiFiStationDisconnected(const WiFiEventStationModeDisconnected& event) {
#else
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
#endif
    alone = true;
    webSocket.disconnect();
//    if (tryToConnectTimer.isItTime()) {
//      Serial.println("tryin to connect");
//      WiFi.begin("diamondLightNetwork");
//      tryToConnectTimer.resetTimer();
//    } 
  //  Serial.println("Disconnected from WiFi access point");
  ////  Serial.print("WiFi lost connection. Reason: ");
  ////  Serial.println(info.disconnected.reason);
  //  Serial.println("Trying to Reconnect");
  //  WiFi.begin("diamondLightNetwork");
  }


#ifdef ESP8266
void WiFiGotIP(const WiFiEventStationModeGotIP& event) {
#else
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
#endif
  alone = false;
  firstOffset = true;
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
//  #ifdef NO_RELAYER
  // if(noRelayer){
    webSocket.begin(WS_SERVER, WS_PORT, WS_PATH);
    Serial.println("connecting to server");
//   } else {
// //  #else
//     webSocket.begin("192.168.4.184", 82, "/");
//     Serial.println("connecting to relayer");
//   }
//  #endif
  webSocket.onEvent(webSocketEvent);
  Serial.println("Web socket client started");

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");


  ArduinoOTA.begin();
  Serial.println("arduino OTA started");

}
//#endif


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  DeserializationError error;
  Serial.printf("\n=== WS EVENT RECEIVED ===\n");
  Serial.printf("Event type: %d\n", type);
  Serial.printf("Length: %d\n", length);
  Serial.printf("========================\n");

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("Disconnected!\n");
      break;

    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      #ifdef ROLE
      sendRole();
      #endif
      sendPing();
      webSocket.sendTXT("lockState");
      break;

    case WStype_BIN:
      Serial.printf("get Binary (treating as text): length=%d\n", length);
      // Fall through to handle like text
      
    case WStype_TEXT:
      Serial.printf("get Text: %s\n", payload);

      // send message to client
      // webSocketsServer.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocketsServer.broadcastTXT("message here");
      
      
      error = deserializeJson(wsMsg, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      } else {
        Serial.print("we got a json! ");
        serializeJson(wsMsg, Serial);

        if (wsMsg["msgType"] == PONG_MSG_TYPE) {
          uint64_t t0 = wsMsg["t0"];
          uint64_t serverTime = wsMsg["serverTime"];
          handlePong(t0, serverTime);
          return;
        }
        
        // Handle sweepSpot assignment (for pyramid devices)
        if (wsMsg["msgType"] == SWEEP_SPOT_MSG_TYPE) {
          uint16_t newSweepSpot = wsMsg["sweepSpot"];
          handleSweepSpotAssignment(newSweepSpot);
          return;
        }
        
        // Handle total LED count update (for pyramid devices)
        if (wsMsg["msgType"] == TOTAL_LED_COUNT_MSG_TYPE) {
          uint16_t totalCount = wsMsg["totalLEDCount"];
          handleTotalLEDCount(totalCount);
          return;
        }

        // Check if message has a startTime
        if (wsMsg.containsKey("startTime")) {
          uint64_t startTime = wsMsg["startTime"];
          // Convert server time to local ESP time
          int64_t startLocal = (int64_t)startTime - offsetMs;
          uint64_t now = (uint64_t)millis();
          
          if (startLocal <= (int64_t)now) {
            // Execute immediately
//            size_t n = min(length, (size_t)(MAX_MSG_LEN - 1));
//            memcpy(wsMsgString, payload, n);
//            wsMsgString[n] = '\0';
            
//            safeStrCopy(wsMsgString, payload, length, MAX_MSG_LEN);
            
            wsMsgString = "";
            serializeJson(wsMsg, wsMsgString);
            
            
            inbox = true;
          } else {
            // Queue for later
            if (pendingMessageCount < MAX_PENDING_MESSAGES) {
//              size_t n = min(length, (size_t)(MAX_MSG_LEN - 1));
//              memcpy(pendingMessages[pendingMessageCount].msgString, payload, n);
//              pendingMessages[pendingMessageCount].msgString[n] = '\0';
//              size_t n = safeStrCopy(pendingMessages[pendingMessageCount].msgString, payload, length, MAX_MSG_LEN);
//              pendingMessages[pendingMessageCount].length = n;
              
              pendingMessages[pendingMessageCount].msgString = "";
              serializeJson(wsMsg, pendingMessages[pendingMessageCount].msgString);
              pendingMessages[pendingMessageCount].startTime = startLocal;
              pendingMessageCount++;
              Serial.printf("Queued message for startLocal: %llu, current local: %llu (count: %d)\n", startLocal, now, pendingMessageCount);
            } else {
              Serial.println("ERROR: Pending message buffer full!");
            }
          }
        } else {
          // No startTime, execute immediately
//          size_t n = min(length, (size_t)(MAX_MSG_LEN - 1));
//          memcpy(wsMsgString, payload, n);
//          wsMsgString[n] = '\0';
//          safeStrCopy(wsMsgString, payload, length, MAX_MSG_LEN);
          wsMsgString = "";
          serializeJson(wsMsg, wsMsgString);
          Serial.printf("saved %s\n", wsMsgString);
          inbox = true;
        }

        // messageTimer.resetTimer();
      }
      break;

    //    case WStype_BIN:
    //      Serial.printf("[%u] get binary length: %u\n", num, length);
    //      hexdump(payload, length);
    //
    //      // send message to client
    //      // webSocketsServer.sendBIN(num, payload, lenght);
    //      break;
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

  // Validate pong
  if (!pingOutstanding) {
    Serial.println("Ignoring pong (no ping outstanding)");
    return;
  }
  if (t0 != lastPingT0) {
    Serial.println("Ignoring stale pong");
    return;
  }

  pingOutstanding = false;

  // Calculate round-trip time
  uint64_t rtt = now - t0;
  
  // Calculate offset: how much to add to ESP millis() to get server time
  // Compensate for half the round-trip time (assume symmetric network delay)
  int64_t newOffset = (int64_t)serverTime - (int64_t)now + (int64_t)(rtt / 2);

  Serial.printf("Pong received: t0=%llu, now=%llu, serverTime=%llu, rtt=%llu\n", 
                t0, now, serverTime, rtt);

  if (firstOffset) {
    // First pong: take as absolute offset, no smoothing
    offsetMs = newOffset;
    firstOffset = false;
    Serial.printf("First offset set: %lld ms\n", offsetMs);
  } else {
    // Check for large jumps (> 100ms)
    int64_t delta = llabs(newOffset - offsetMs);
    if (delta > 100) {
      Serial.printf("Large offset jump detected (%lld ms), snapping\n", delta);
      offsetMs = newOffset;
    } else {
      // Smooth the offset with exponential moving average (90% old, 10% new)
      offsetMs = (int64_t)(offsetMs * 0.9 + newOffset * 0.1);
    }
    Serial.printf("Offset adjusted: %lld ms (delta: %lld ms)\n", offsetMs, newOffset - offsetMs);
  }
}

void handleSweepSpotAssignment(uint16_t newSweepSpot) {
  sweepSpot = newSweepSpot;
  Serial.printf("SweepSpot assigned: %d\n", sweepSpot);
}

void handleTotalLEDCount(uint16_t totalCount) {
  Serial.printf("Total LED count updated: %d devices\n", totalCount);
  
  // Update node count for patterns
  mainState.nodeCount.plength = totalCount;
  nodeCountTimer.setInterval(mainState.nodeCount.decay * 10 * totalCount);
  
  // Run nodeCounter() to flash the lights
  nodeCounter();
}
