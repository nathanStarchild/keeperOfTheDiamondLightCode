#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif

#include <WiFiUdp.h>
#include <ArduinoOTA.h>


#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define SSID "queerBurners"
#define WIFI_KEY "nextyearwasbetter"
#define WS_SERVER "192.168.0.10"
#define WS_PORT 80
#define WS_PATH "/"


WebSocketsClient webSocket;
StaticJsonDocument<300> wsMsgIncoming;
StaticJsonDocument<300> wsMsgReady;

MilliTimer tryToConnectTimer(1 * 60000);

bool inbox = false;
#define MAX_PENDING_MESSAGES 4
struct PendingMessage {
  StaticJsonDocument<300> doc;  // Store parsed JSON directly
  uint64_t startTime;
};

PendingMessage pendingMessages[MAX_PENDING_MESSAGES];
uint8_t pendingMessageCount = 0;

uint32_t pingInterval = 60000;
MilliTimer pingTimer(pingInterval);

uint64_t offsetMs = 0;        // smoothed server offset
bool firstOffset = true;
uint64_t lastPingT0 = 0;
bool pingOutstanding = false;
#define PING_MSG_TYPE = 999
#define PONG_MSG_TYPE = 998


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void processWSMessage();
void sendPing();
void handlePong(uint64_t t0, uint64_t serverTime);

#ifdef ESP8266
WiFiEventHandler stationConnectedHandler;
void WiFiGotIP(const WiFiEventStationModeGotIP& event);
WiFiEventHandler stationDisconnectedHandler;
void WiFiStationDisconnected(const WiFiEventStationModeDisconnected& event);
#else
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
#endif


void wsSetup() {

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.setSleep(false);
  #ifdef WIFI_KEY
    WiFi.begin(SSID, WIFI_KEY);
  #else
    WiFi.begin(SSID);
  #endif

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
   while (WiFi.status() != WL_CONNECTED && retries < 10) {
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
        WiFi.begin("diamondLightNetwork");
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
        // Copy the JSON document to wsMsgReady
        wsMsgReady = pendingMessages[i].doc;
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
    webSocket.sendTXT("ping");
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

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("Disconnected!\n");
      break;

    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      sendPing()
      webSocket.sendTXT("lockState");
      break;

    case WStype_TEXT:
      Serial.printf("get Text: %s\n", payload);

      // send message to client
      // webSocketsServer.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocketsServer.broadcastTXT("message here");
      
      
      error = deserializeJson(wsMsgIncoming, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      } else {
        Serial.print("we got a json!");
        serializeJson(wsMsgIncoming, Serial);

        if (wsMsgIncoming["msgType"] == PONG_MSG_TYPE) {
          uint64_t t0 = wsMsgIncoming["t0"];
          uint64_t serverTime = wsMsgIncoming["serverTime"];
          handlePong(t0, serverTime);
          return;
        }

        // Check if message has a startTime
        if (wsMsgIncoming.containsKey("startTime")) {
          uint64_t startTime = wsMsgIncoming["startTime"];
          uint64_t startLocal = startTime - offsetMs;
          uint64_t now = (uint64_t)millis();
          
          if (startLocal <= now) {
            // Execute immediately
            wsMsgReady = wsMsgIncoming;
            inbox = true;
          } else {
            // Queue for later
            if (pendingMessageCount < MAX_PENDING_MESSAGES) {
              // Copy the parsed JSON document directly
              pendingMessages[pendingMessageCount].doc = wsMsgIncoming;
              pendingMessages[pendingMessageCount].startTime = startLocal;
              pendingMessageCount++;
              Serial.printf("Queued message for startLocal: %llu (count: %d)\n", startLocal, pendingMessageCount);
            } else {
              Serial.println("ERROR: Pending message buffer full!");
            }
          }
        } else {
          // No startTime, execute immediately
          wsMsgReady = wsMsgIncoming;
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

  ws.sendTXT(buf);
}

void handlePong(uint64_t t0, uint64_t serverTime) {
  uint64_t now = (uint64_t)millis();

  if (!pingOutstanding) return;
  if (t0 != lastPingT0) return;   // stale pong

  pingOutstanding = false;

  uint64_t rtt = now - t0;

  int64_t newOffset =
      (int64_t)serverTime
    - (int64_t)now
    + (int64_t)(rtt / 2);

  if (firstOffset) {
    // take first pong as absolute offset, no smoothing
    offsetMs = newOffset;
    firstOffset = false;
  } else {
    // snap if huge jump
    if (llabs(newOffset - (int64_t)offsetMs) > 100) {
      offsetMs = newOffset;
    } else {
      // smooth
      offsetMs = (int64_t)(offsetMs * 0.9 + newOffset * 0.1);
    }
  }
}