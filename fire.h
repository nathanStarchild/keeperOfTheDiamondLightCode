//b4
const uint16_t stripLength = 38;
const uint8_t nStrips = 5;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_PER_OUTPUT 114
#define NUM_OUTPUTS 2
#define NUM_LEDS 228
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, 1, 1, 1, 1};
int directionUD[nStrips] = {1, 1, 1, 1, 1};
int directionIO[nStrips] = {1, 1, 1, 1, 1};
int stripDirection[nStrips] =  {1, 1, 1, 1, 1};
uint16_t audienceSpot = 4;
uint16_t sweepSpot = 3;
uint16_t element = 2;

void elementSetup(){
    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, LEDS_PER_OUTPUT); 
    FastLED.addLeds<WS2813, DATA_PIN_2, RGB>(outLeds, LEDS_PER_OUTPUT, LEDS_PER_OUTPUT); 
    
    ArduinoOTA.setHostname("fire");
}
