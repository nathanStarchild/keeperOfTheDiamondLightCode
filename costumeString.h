#include "pinDefinitions.h"

// #define NUMBER 0

const uint16_t stripLength = 50;
const uint8_t nStrips = 2;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_OUTPUT1 stripLength * nStrips
// #define LEDS_OUTPUT2 stripLength * 4
// #define LEDS_OUTPUT3 stripLength * 4
#define NUM_OUTPUTS 1
#define NUM_LEDS stripLength * nStrips
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, 1};
int directionUD[nStrips] = {1, -1};
int directionIO[nStrips] = {1, 1};
int stripDirection[nStrips] =  {1, -1};
uint16_t audienceSpot = 1;
uint16_t sweepSpot = 3;
uint16_t element = 0;

// magic button
#include "magicButton.h"
#include "magicButtonHandler.h"
MagicButton magicButton(BUTTON_PIN, true); // Using pullup mode
MagicButtonHandler magicButtonHandler(&magicButton);
MilliTimer buttonCheckTimer(50); // debounce timer

void elementSetup(){
    FastLED.addLeds<WS2812B, DATA_PIN_1, GRB>(outLeds, 0, LEDS_OUTPUT1); 
    Serial.print("LEDS_OUTPUT1 = ");
    Serial.println(LEDS_OUTPUT1);
    Serial.print("stripLength = ");
    Serial.println(stripLength);
    Serial.print("nStrips = ");
    Serial.println(nStrips);
    Serial.print("NUM_LEDS = ");
    Serial.println(NUM_LEDS);

    // char hostname[32];
    // snprintf(hostname, sizeof(hostname), "costume_%d", NUMBER);
    // ArduinoOTA.setHostname(hostname);

    magicButton.begin();
}

#define ELEMENT_LOOP true
void elementLoop() {
    if (buttonCheckTimer.isItTime()) {
        magicButtonHandler.update();
        buttonCheckTimer.resetTimer();
    }
}
