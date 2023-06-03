//b3
const uint16_t stripLength = 120;
const uint8_t nStrips = 5;
const uint16_t num_leds = stripLength * nStrips;

#define LEDS_PER_OUTPUT 600
#define NUM_OUTPUTS 1
#define NUM_LEDS 600
CRGB leds[NUM_LEDS];
CRGB oldLeds[NUM_LEDS];
CRGB outLeds[NUM_LEDS];

int directionLR[nStrips] = {1, 1, 1, 1, 1};
int directionUD[nStrips] = {1, 1, 1, 1, 1};
int directionIO[nStrips] = {1, 1, 1, 1, 1};
int stripDirection[nStrips] =  {1, 1, 1, 1, 1};
uint16_t audienceSpot = 0;
uint16_t sweepSpot = 0;

void elementSetup(){
   FastLED.addLeds<WS2813, DATA_PIN_3, RGB>(outLeds, 0, LEDS_PER_OUTPUT); 
    //LED type, data pin, clock pin, rgb order
    // FastLED.addLeds<APA102, DATA2_DATA1, DATA1_CLOCK1, BGR>(outLeds, LEDS_PER_OUTPUT);
    
    ArduinoOTA.setHostname("tetrahedron");
    Serial.println("tetra");
}
