#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


#include <WebSocketsClient.h>
#include <ArduinoJson.h>


WebSocketsClient webSocket;
DynamicJsonDocument wsMsg(1024);
String wsMsgString;

bool inbox = false;
uint32_t loopCounter = 0;


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void processWSMessage();

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin("keeperOfTheDiamondLights", "enlighten");
}

void wsSetup() {

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.begin("keeperOfTheDiamondLights", "enlighten");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
   
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  webSocket.begin("192.168.4.184", 82, "/");
  webSocket.onEvent(webSocketEvent);
  Serial.println("Web socket client started");

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");
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
    ArduinoOTA.handle();

  if (loopCounter % 4 == 0){
    webSocket.loop();
  }

  if (inbox) {
    processWSMessage();
    inbox = false;
  }

}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

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
      // webSocketsServer.broadcastTXT("message here");
      
      
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
