const uint16_t stripLength = 120;
const uint8_t nStrips = 11;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_OUTPUT1 1320
#define NUM_OUTPUTS 1
#define NUM_LEDS 1320
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1};
int directionUD[nStrips] = {1, -1, 1, -1, 1, -1, 1, 1, 1, 1, -1};
int directionIO[nStrips] = {1, -1, 1, -1, 1, -1, 1, 1, 1, 1, -1};
int stripDirection[nStrips] =  {1, -1, 1, -1, 1, -1, 1, 1, 1, 1, -1};
uint16_t audienceSpot = 1;
uint16_t sweepSpot = 1;
uint16_t element = 0;

void elementSetup(){
    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, LEDS_OUTPUT1); 
//    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, 600); 
//    FastLED.addLeds<WS2813, DATA_PIN_2, RGB>(outLeds, 600, 360); 

    ArduinoOTA.setHostname("djBooth");
}
