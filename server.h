#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <SPIFFS.h>

#define WEBSOCKETS_SERVER_CLIENT_MAX 8
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

#include <FS.h>

DNSServer dnsServer;
AsyncWebServer server(80);
WebSocketsServer webSocketsServer = WebSocketsServer(81);
DynamicJsonDocument wsMsg(1024);
String wsMsgString;

bool inbox = false;
uint32_t loopCounter = 0;

MilliTimer messageTimer(7.3 * 60000); 
void sendMessage(uint8_t messageType);
void earthMode();
void fireMode();
void airMode();
void waterMode();
void metalMode();
void doubleRainbow();
void tranquilityMode();
void tripperTrapMode();
void antsMode();
void upset_mainState();
void blender();
void tailTime();
void shootingStars();


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void processWSMessage();

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {

    if (SPIFFS.exists("/index.html")) {

      Serial.println("index.html exists!");

      AsyncResponseStream *response = request->beginResponseStream("text/html");

      File file = SPIFFS.open("/index.html");

      while(file.available()){

          response->write(file.read());
      }

      request->send(response);
    }

    else{
      request->send(404);
    }

  }
};


void sendMessage(uint8_t messageType) {
  DynamicJsonDocument nm_root(1024);
  
  nm_root["msgType"] = messageType;
  
  String nm_msgp;
  serializeJson(nm_root, nm_msgp);
  webSocketsServer.broadcastTXT(nm_msgp);
}

void wsSetup() {
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.softAP("keeperOfTheDiamondLights", "enlighten", 1, false, 8);
  Serial.println(WiFi.softAPIP());
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println("DNS Server started!");

  server.serveStatic("/", SPIFFS, "/", "max-age=86400").setDefaultFile("index.html");
  server.begin();
  Serial.println("Webserver started!");
  
  webSocketsServer.begin();
  webSocketsServer.onEvent(webSocketEvent);
  Serial.println("Web socket server started");

  // No authentication by default
//  ArduinoOTA.setPassword("admin");
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
    webSocketsServer.loop();
    
    dnsServer.processNextRequest();

    ArduinoOTA.handle();
  }

  if (inbox) {
    processWSMessage();
    inbox = false;
  }
  
  if (messageTimer.isItTime()) {
    messageTimer.resetTimer();
    boredTimer.resetTimer();
    uint8_t ran = random8();
    if (!mainState.launch.enabled){ 
      if (ran < 15) {
        sendMessage(9);
        mainState.launch.enabled = true;
      } else if (ran < 37) {
        sendMessage(21);
        fireMode();
//        sendMessage(20);
//        earthMode();
      } else if (ran < 60) {
        sendMessage(21);
        fireMode();
      } else if (ran < 83) {
//        sendMessage(22);
//        airMode();
        sendMessage(1);
        upset_mainState();
      } else if (ran < 106) {
        sendMessage(21);
        fireMode();
      } else if (ran < 129) {
        sendMessage(24);
        metalMode();
      } else if (ran < 152) {
        sendMessage(5);
        tripperTrapMode();
      } else if (ran < 175) {
        sendMessage(6);
        antsMode();
      } else if (ran < 198) {
        sendMessage(2);
        doubleRainbow();
      } else if (ran < 221) {
//        sendMessage(3);
//        tranquilityMode();
        sendMessage(21);
        fireMode();
      } else if (ran < 245) {
        sendMessage(1);
        upset_mainState();
      }
    }
  }

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  DeserializationError error;

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocketsServer.remoteIP(num);
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
      webSocketsServer.broadcastTXT(payload);
      
      
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
        messageTimer.resetTimer();
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
