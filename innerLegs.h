const uint16_t stripLength = 220;
const uint8_t nStrips = 3;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_PER_OUTPUT 220
#define NUM_OUTPUTS 3
#define NUM_LEDS 660
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, 1, 1,};
int directionUD[nStrips] = {1, 1, 1};
int directionIO[nStrips] = {1, 1, 1};
int stripDirection[nStrips] =  {-1, -1, -1};
uint16_t audienceSpot = 0;
uint16_t sweepSpot = 6;
uint16_t element = 0;

void elementSetup(){
    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, LEDS_PER_OUTPUT); 
    FastLED.addLeds<WS2813, DATA_PIN_2, RGB>(outLeds, LEDS_PER_OUTPUT, LEDS_PER_OUTPUT); 
    FastLED.addLeds<WS2813, DATA_PIN_3, RGB>(outLeds, LEDS_PER_OUTPUT * 2, LEDS_PER_OUTPUT); 

    ArduinoOTA.setHostname("innerLegs");
}
