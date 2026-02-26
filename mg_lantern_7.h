#define ESP8266 true
#include "pinDefinitions.h"

#define ROLE "lantern"
#define NUMBER 7

const uint16_t stripLength = 30;
const uint8_t nStrips = 1;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_PER_PIXEL 10
#define LEDS_OUTPUT1 stripLength * nStrips / LEDS_PER_PIXEL
// #define LEDS_OUTPUT2 stripLength * 4
// #define LEDS_OUTPUT3 stripLength * 4
#define NUM_OUTPUTS 1
#define NUM_LEDS stripLength * nStrips
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];
CRGB targetLeds[NUM_LEDS / LEDS_PER_PIXEL];

int directionLR[nStrips] = {1};
int directionUD[nStrips] = {1};
int directionIO[nStrips] = {1};
int stripDirection[nStrips] =  {1};
int stripZ[nStrips] = {1}
uint16_t audienceSpot = 1;
uint16_t sweepSpot = NUMBER + 3;
uint16_t element = 0;

void elementSetup(){
    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(targetLeds, 0, LEDS_OUTPUT1); 
    // FastLED.addLeds<WS2813, DATA_PIN_2, RGB>(outLeds, LEDS_OUTPUT1, LEDS_OUTPUT2); 
    // FastLED.addLeds<WS2813, DATA_PIN_3, RGB>(outLeds, LEDS_OUTPUT1 + LEDS_OUTPUT2, LEDS_OUTPUT3); 
//    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, 600); 
//    FastLED.addLeds<WS2813, DATA_PIN_2, RGB>(outLeds, 600, 360); 

    char hostname[32];
    snprintf(hostname, sizeof(hostname), "%s_%d", ROLE, NUMBER);
    ArduinoOTA.setHostname(hostname);
}
