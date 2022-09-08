//b3
const uint16_t stripLength = 8;
const uint8_t nStrips = 10;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_PER_OUTPUT 80
#define NUM_OUTPUTS 1
#define NUM_LEDS 80
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, 1, 1, 1, 1, 1, 1, -1, 1, 1};
int directionUD[nStrips] = {1, 1, 1, 1, 1, 1, 1, -1, 1, 1};
int directionIO[nStrips] = {1, 1, 1, 1, 1, 1, 1, -1, 1, 1};
int stripDirection[nStrips] =  {1, -1, 1, -1, 1, 1, -1, 1, -1, 1};
uint16_t audienceSpot = 3;
uint16_t sweepSpot = 3;

void elementSetup(){
//    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, LEDS_PER_OUTPUT); 
    //LED type, data pin, clock pin, rgb order
    FastLED.addLeds<APA102, DATA2_DATA1, DATA1_CLOCK1, BGR>(outLeds, LEDS_PER_OUTPUT);
    
    ArduinoOTA.setHostname("lightPainting3");
}
