//b5
const uint16_t stripLength = 22;
const uint8_t nStrips = 5;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_PER_OUTPUT 110
#define NUM_OUTPUTS 1
#define NUM_LEDS 110
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, 1, 1, 1, 1};
int directionUD[nStrips] = {1, 1, 1, 1, 1};
int directionIO[nStrips] = {1, 1, 1, 1, 1};
int stripDirection[nStrips] =  {1, 1, 1, 1, 1};
uint16_t audienceSpot = 5;
uint16_t sweepSpot = 2;
uint16_t element = 5;

void elementSetup(){
    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, LEDS_PER_OUTPUT); 
    
    ArduinoOTA.setHostname("metal");
}
