//b3
const uint16_t stripLength = 19;
const uint8_t nStrips = 9;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_PER_OUTPUT stripLength * nStrips
#define NUM_OUTPUTS 1
#define NUM_LEDS stripLength * nStrips
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, -1, 1, 1, -1, 1, 1, -1, 1};
int directionUD[nStrips] = {1, -1, 1, -1, 1, -1, 1, -1, 1};
int directionIO[nStrips] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
int stripDirection[nStrips] =  {1, -1, 1, -1, 1, -1, 1, -1, 1};
uint16_t audienceSpot = 2;
uint16_t sweepSpot = 2;
uint16_t element = 0;

void elementSetup(){
//    FastLED.addLeds<WS2813, DATA_PIN_1, RGB>(outLeds, 0, LEDS_PER_OUTPUT); 
    //LED type, data pin, clock pin, rgb order
    FastLED.addLeds<WS2812B, DATA2_3PIN, GRB>(outLeds, LEDS_PER_OUTPUT);
    
    ArduinoOTA.setHostname("piCostume");
    stripSpacing = 2;
}
