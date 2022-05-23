const uint16_t stripLength = 240;
const uint8_t nStrips = 2;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_PER_OUTPUT 240
#define NUM_OUTPUTS 2
#define NUM_LEDS 480
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, 1};
int directionUD[nStrips] = {1, 1};
int directionIO[nStrips] = {1, 1};
int stripDirection[nStrips] =  {1, 1};
uint16_t audienceSpot = 6;
uint16_t sweepSpot = 1;

void elementSetup(){
    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, LEDS_PER_OUTPUT); 
    FastLED.addLeds<WS2813, DATA_PIN_2, RGB>(outLeds, LEDS_PER_OUTPUT, LEDS_PER_OUTPUT); 

    ArduinoOTA.setHostname("theDome");
}
