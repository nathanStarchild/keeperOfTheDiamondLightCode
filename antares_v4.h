// Unified configuration for all three pyramid ESPs (antares_v4)
// Replaces antares_v4_a.h, antares_v4_b.h, antares_v4_c.h
// sweepSpot is now assigned dynamically by the system
// Hostname is auto-generated from MAC address for uniqueness

#define ROLE "pyramid"

#include "pinDefinitions.h"

const uint16_t stripLength = 114;
const uint8_t nStrips = 11;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_OUTPUT1 stripLength * 5
#define LEDS_OUTPUT2 stripLength * 6
#define NUM_OUTPUTS 2
#define NUM_LEDS stripLength * nStrips
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, 1, 1, 1, -1, 1, 1, -1, -1, 1, 1};
int directionUD[nStrips] = {1, 1, -1, 1, 1, -1, 1, 1, -1, -1, 1};
int directionIO[nStrips] = {1, -1, 1, -1, 1, -1, 1, -1, 1, 1, 1};
int stripDirection[nStrips] =  {1, 1, -1, 1, 1, -1, 1, 1, -1, -1, 1};

// audienceSpot can be set manually if needed, or could be dynamic in future
uint16_t audienceSpot = 0;

// sweepSpot is now assigned dynamically - initialize to 0
uint16_t sweepSpot = 0;

uint16_t element = 0;

void elementSetup(){
    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, LEDS_OUTPUT1); 
    FastLED.addLeds<WS2813, DATA_PIN_3, RGB>(outLeds, LEDS_OUTPUT1, LEDS_OUTPUT2); 
//    FastLED.addLeds<WS2813, DATA_PIN_3, RGB>(outLeds, LEDS_OUTPUT1 + LEDS_OUTPUT2, LEDS_OUTPUT3); 
//    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, 600); 
//    FastLED.addLeds<WS2813, DATA_PIN_2, RGB>(outLeds, 600, 360); 

    // Generate unique hostname from MAC address (last 6 hex chars)
    // e.g., "antares_v4_A1B2C3" instead of hardcoded _a, _b, _c
    uint8_t mac[6];
    #ifdef ESP8266
      WiFi.macAddress(mac);
    #else
      esp_read_mac(mac, ESP_MAC_WIFI_STA);
    #endif
    
    char hostname[32];
    snprintf(hostname, sizeof(hostname), "antares_v4_%02X%02X%02X", 
             mac[3], mac[4], mac[5]);
    
    ArduinoOTA.setHostname(hostname);
}
