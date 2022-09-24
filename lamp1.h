//b3
const uint16_t stripLength = 8;
const uint8_t nStrips = 4;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_PER_OUTPUT 32
#define NUM_OUTPUTS 1
#define NUM_LEDS 32
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, 1, 1, 1};
int directionUD[nStrips] = {1, 1, 1, 1};
int directionIO[nStrips] = {1, -1, 1, -1};
int stripDirection[nStrips] =  {1, 1, 1, 1};
uint16_t audienceSpot = 1;
uint16_t sweepSpot = 1;
uint16_t element = 0;

void elementSetup(){
    FastLED.addLeds<WS2812, DATA_PIN_1, BGR>(outLeds, 0, LEDS_PER_OUTPUT); 
    
    ArduinoOTA.setHostname("lamp1");
}
