#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


#include <WebSocketsClient.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

WebSocketsServer webSocketServer = WebSocketsServer(82);
WebSocketsClient webSocketClient;
IPAddress local_IP(192, 168, 4, 184);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 0, 0);

DynamicJsonDocument wsMsg(1024);
String wsMsgString;

bool inbox = false;
uint32_t loopCounter = 0;


void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length);
void processWSMessage();


void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin("fiveElementsOrrery", "enlighten");
}

void sendMessage(uint8_t messageType) {
  DynamicJsonDocument nm_root(1024);
  
  nm_root["msgType"] = messageType;
  
  String nm_msgp;
  serializeJson(nm_root, nm_msgp);
  webSocketServer.broadcastTXT(nm_msgp);
}


void wsSetup() {

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin("fiveElementsOrrery", "enlighten");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
   
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  
  webSocketServer.begin();
  webSocketServer.onEvent(webSocketServerEvent);
  Serial.println("Web socket server started");
  
  webSocketClient.begin("192.168.4.1", 81, "/");
  webSocketClient.onEvent(webSocketClientEvent);
  Serial.println("Web socket client started");

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  ArduinoOTA.setPassword("admin");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
}

void wsLoop() {
    loopCounter++;

  if (loopCounter % 4 == 0){
    webSocketServer.loop();
  }
  
  if (loopCounter % 4 == 2){
    webSocketClient.loop();
  }

  if (inbox) {
    processWSMessage();
    inbox = false;
  }
  
  ArduinoOTA.handle();
}


void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  DeserializationError error;

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocketServer.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        // webSocketsServer.sendTXT(num, "Connected");
      }
      break;

    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      // send message to client
      // webSocketsServer.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocketsServer.broadcastTXT(payload);
      
      
      // error = deserializeJson(wsMsg, payload);
      // if (error) {
      //  Serial.print(F("deserializeJson() failed: "));
      //  Serial.println(error.f_str());
      // } else {
      //   Serial.print("we got a json!");
      //   wsMsgString = "";
      //   serializeJson(wsMsg, wsMsgString);
      //   serializeJson(wsMsg, Serial);
      //   inbox = true;
      //   messageTimer.resetTimer();
      // }
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


void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length) {

  DeserializationError error;

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("Disconnected!\n");
      break;

    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      // webSocket.sendTXT("Connected");
      break;

    case WStype_TEXT:
      Serial.printf("get Text: %s\n", payload);

      // send message to client
      // webSocketsServer.sendTXT(num, "message here");

      // send data to all connected clients
       webSocketServer.broadcastTXT(payload);
      
      
      error = deserializeJson(wsMsg, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      } else {
        Serial.print("we got a json!");
        wsMsgString = "";
        serializeJson(wsMsg, wsMsgString);
        serializeJson(wsMsg, Serial);
        inbox = true;
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
