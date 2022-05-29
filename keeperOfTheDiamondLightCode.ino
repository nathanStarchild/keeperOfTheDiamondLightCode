#define CONFIG_LWIP_MAX_SOCKETS 16

#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ESP32_I2S true
#include <FastLED.h>
FASTLED_USING_NAMESPACE

#include "millitimer.h"
#include "pleiadianPalettes.h"
#include "pleiadianPatterns.h"

MilliTimer boredTimer(11 * 60000); //bored timer, change if no messages or controller input

#include "server.h"
//#include "relayer.h"
//#include "client.h"


#define   LED  2       // GPIO number of connected LED, ON ESP-12 IS GPIO2
#define DATA_PIN_1 14
#define DATA_PIN_2 27
#define DATA_PIN_3 15

// #define FASTLED_ESP8266_D1_PIN_ORDER


const uint16_t enlightenTime = 60000; //ms
MilliTimer offline_pattern(3 * 60000); //1000 seconds
//MilliTimer boredTimer(11 * 60000); //bored timer, change if no messages or controller input
MilliTimer glitterTimer(10000); //how long glitter runs for
MilliTimer enlightenment(enlightenTime); //enlightenment button hold time
MilliTimer paletteBlendTimer(100); //how often to perform palette blending steps when moving to new palette
MilliTimer paletteCycleTimer(5 * 60000); // how often to move to the next palette
MilliTimer testingTimer(10000);
MilliTimer tripperTrapTimer(5 * 60000); //how long to stay in tripper trap mode
MilliTimer batteryUpdateTimer(9*60000); //how long to wait before assuming the voltage screamer isn't coming back online
MilliTimer frameTimer(20);

uint16_t aoIndex = 0;
uint16_t frameCount = 0;
uint16_t stepRate = 1;
uint8_t fadeRate = 100;
bool controllerEnabled = true;
bool messagingEnabled = true;
bool holdingPatternLockdown = false;
int launchProgress = 0;

const float PHI = 1.61803398875;
uint8_t primes[4] = {5, 3, 2, 1};
uint8_t sPrimes[9] = {2, 3, 5, 7, 11, 13, 17, 19, 23};
const float fib =  1.61803;

uint16_t nX(uint8_t n, int x);

//#include "theDome.h"
//#include "outerLegs.h"
//#include "innerLegs.h"
// #include "air.h"
// #include "water.h"
//#include "earth.h"
//#include "fire.h"
//#include "metal.h"
//#include "lamp1.h"
//#include "serverOfTheDiamondLightNetwork.h"
#include "hotJam.h"

//CRGB leds[NUM_LEDS];
//CRGB oldLeds[NUM_LEDS];
//CRGB outLeds[NUM_LEDS];

#include "rippler.h"
const uint8_t nRipples = 10;
uint8_t nRipplesEnabled = 0;
Rippler ripples[nRipples];

#include "spiraliser.h"
Spiraliser spiral;

void setup() {
    Serial.begin(9600);
    elementSetup();
    wsSetup();

    randomSeed(analogRead(A0));

    currentBlending = LINEARBLEND;
    currentPalette = Deep_Skyblues_gp;
    targetPalette = Deep_Skyblues_gp;
    upset_mainState();
    stepRate = 3;
    doubleRainbow();
}

void loop(){
    wsLoop();

    if ( frameTimer.isItTime()) {
        blendFrames();
        FastLED.show();
        mainState.stale = true;
        frameTimer.resetTimer();
    }

    if (mainState.stale) {
      updatePatterns(); //fill the buffer up with the next lot of patterns.
    }

    if (paletteBlendTimer.isItTime()) {//blend to the new palette palette
        uint8_t maxChanges = 48;
        nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
        paletteBlendTimer.resetTimer();
    }  

    if (nothingIsOn()) {
        shootingStars();
    }

//    dontGetBored();
}

void blendFrames(){
    uint8_t ratio = map(frameCount % stepRate, 0, stepRate-1, 0, 255);
    for (int i = 0; i < NUM_LEDS; i++) {
        outLeds[i] = blend( oldLeds[i], leds[i], ratio );
    }
}

void upset_mainState() {
    //randomise wave, tail, breathe, hue=0, patternStep=0
    //add faderate
    nextPalette();
    patternsOff();
    fadeRate = random8(10, 240);
        //  stepRate = max(1, random8(0, 15) - 10);
    stepRate = random8(2, 10);

    mainState.hue = 0;
    mainState.patternStep = 0;

    mainState.tail.enabled = (random8() > 180);
    mainState.tail.plength = random(20, 120);
    mainState.tail.pspeed = random(0, 6) - 3;
    if (mainState.tail.pspeed == 0) {
        mainState.tail.pspeed = 1;
    }

    mainState.glitter.enabled = (random8() > 180);
    mainState.glitter.plength = random8(50, 255);
    mainState.glitter.pspeed = (int) random(0, 2) - 1;

    mainState.rain.enabled = (random8() > 140);
    mainState.rain.plength = random8(3, 30);
    mainState.rain.pspeed = random8(10, 50);

    mainState.ripple.enabled = (random8() > 160);
    setNRipples(random8(1, nRipples));

    mainState.blendwave.enabled =  (random8() > 196);

    mainState.ants.enabled = (random8() > 156);
    mainState.ants.pspeed = ((random8(0, 33)) % 21) + 1;
    mainState.ants.plength = random8(1, 57);

    mainState.spiral.enabled = (random8() > 200);

    
    mainState.rainbow.enabled = (random8() > 190);
    mainState.rainbow.pspeed = random8(1, 10);
    mainState.rainbow.plength = random8(1, 10);

    
    mainState.noise.enabled = (random8() > 200);
    mainState.noise.plength = random8(1, 30);
    mainState.noise.pspeed = random8(1, 5);
    
    mainState.noiseFade.enabled = (random8() > 90);
    mainState.noiseFade.plength = random8(1, 33);
    mainState.noiseFade.pspeed = random8(1, 10);
}

void tripperTrapMode() {
    //wave, tail, breathe, hue=0, patternStep=0
    patternsOff();
    mainState.hue = 0;
    mainState.patternStep = 0;
    stepRate = random8(1, 5);

    mainState.wave.enabled = true; //wave is now for tripper trap mode only
    mainState.wave.plength = random8(1, 5);
    mainState.wave.pspeed = random8(1, 10);


    mainState.tail.enabled = true;
    mainState.tail.plength = random(20, 120);
    mainState.tail.pspeed = (int) (random(0, 30) - 15) / 12;

    mainState.breathe.enabled = (random8() > 50);
    mainState.breathe.plength = random8(50, 240);
    mainState.breathe.pspeed = random8(10, 30);
    
    mainState.noiseFade.enabled = true;
    mainState.noiseFade.plength = random8(1, 30);
    mainState.noiseFade.pspeed = random8(1, 10);

    tripperTrapTimer.startTimer();
}

void tranquilityMode() {
    patternsOff();
    targetPalette = OceanColors_p;
    paletteCycleIndex = 0;
    mainState.rain.enabled = true;
    mainState.rain.decay = 100;
    mainState.rain.pspeed = 50;
    mainState.rain.plength = stripLength/11;
    stepRate = 6;
    fadeRate = 50;
}

void shootingStars() {
    patternsOff();
    setNRipples(1);
    mainState.ripple.enabled = true;
    stepRate = 4;
    fadeRate = 50;
}

void rippleGeddon() {
    patternsOff();
    mainState.ripple.enabled = true;
    stepRate = 7;
    fadeRate = 20;
    setNRipples(9);
    targetPalette = Alive_And_Kicking_gp;  
}

void tailTime() {
  patternsOff();
  mainState.tail.enabled = true;
  mainState.tail.plength = stripLength + 5;
  mainState.tail.pspeed = 1;
  mainState.glitter.enabled = true;
  mainState.glitter.plength = random8(50, 255);
  mainState.glitter.pspeed = (int) random(0, 2) - 1;
  //  mainState.rainbow.enabled = true;
  mainState.noiseFade.enabled = true;
  mainState.noiseFade.plength = random8(1, 30);
  mainState.noiseFade.pspeed = random8(1, 10);
  stepRate = 4;
  fadeRate = 10;
}

void blender() {
  patternsOff();
  mainState.blendwave.enabled = true;
  mainState.rain.enabled = true;
  mainState.rain.plength = random8(3, 30);
  mainState.rain.pspeed = random8(10, 50);
  mainState.tail.enabled = true;
  mainState.tail.plength = 22;
  mainState.tail.pspeed = 1;
  mainState.noiseFade.enabled = true;
  mainState.noiseFade.plength = random8(1, 30);
  mainState.noiseFade.pspeed = random8(1, 10);
  stepRate = 6;
  fadeRate = 20;
}

void doubleRainbow() {
  patternsOff();
  mainState.rainbow.enabled = true;
  mainState.noiseFade.enabled = true;
  mainState.noiseFade.plength = random8(1, 30);
  mainState.noiseFade.pspeed = random8(1, 10);
  stepRate = 4;
  fadeRate = 100;
}

void rainbowSpiral() {
  patternsOff();
  targetPalette = Sleep_Deprevation_gp;
  mainState.spiral.enabled = true;
  stepRate = 8;
}

void flashGrid() {
  patternsOff();
  //  mainState.blendwave.enabled = true;
  //  mainState.rainbow.enabled = true;
  mainState.ants.enabled = true;
  mainState.ants.pspeed = 8;
  mainState.ants.plength = 87;
  mainState.noiseFade.enabled = true;
  mainState.noiseFade.plength = random8(1, 30);
  mainState.noiseFade.pspeed = random8(1, 10);
  stepRate = 12;
}

void antsMode() {
      patternsOff();
      mainState.ants.enabled = true;
      mainState.ants.plength = 20;
      mainState.ants.pspeed = 8;
      mainState.ants.decay = 150;
      stepRate = 2;
      fadeRate = 100;
}

void noiseTest() {
  patternsOff();
  mainState.noise.enabled = true;
  mainState.noise.plength = random8(1, 25);
  mainState.noise.pspeed = random8(1, 10);
}

void noiseFader() {
  mainState.noiseFade.enabled = !mainState.noiseFade.enabled;
  mainState.noiseFade.plength = random8(1, 15);
  mainState.noiseFade.pspeed = random8(1, 10);
}

void earthMode() {
  targetPalette = ForestColors_p;
  stepRate = 5;
  patternsOff();
  if (audienceSpot == 0 || audienceSpot == 6 || audienceSpot == 1){
    mainState.noise.enabled = true;
    mainState.noise.plength = random8(16,22);
    mainState.noise.pspeed = random8(1, 7);
    mainState.noiseFade.enabled = true;
    mainState.noiseFade.plength = random8(1, 5);
    mainState.noiseFade.pspeed = random8(1, 15);
  }
}

void fireMode() {
  targetPalette = Sunset_Real_gp;
  stepRate = 1;
  patternsOff();
  //if (audienceSpot == 0 || audienceSpot == 6 || audienceSpot == 4){
  if (audienceSpot == 0 || audienceSpot == 6){
    mainState.fire.enabled = true;
    mainState.fire.pspeed = 80;
    mainState.fire.decay = 70;
  } else if (audienceSpot == 4) {
    mainState.fire.enabled = true;
    mainState.fire.pspeed = 20;
    mainState.fire.decay = 60;
    stepRate = 2;
    
  }
}

void waterMode() {
  targetPalette = OceanColors_p;
  stepRate = 1;
  patternsOff();
  if (audienceSpot == 0 || audienceSpot == 6 || audienceSpot == 2){
    tranquilityMode();
    stepRate = 1;
  }
}

void metalMode() {
  targetPalette = metal_gold_002_gp;
  stepRate = 1;
  patternsOff();
  if (audienceSpot == 0 || audienceSpot == 5){
    mainState.metal.enabled = true;
    mainState.metal.pspeed = num_leds / 10;
    mainState.metal.plength = 150;
    stepRate = 4;
  } else if (audienceSpot == 6) {
    mainState.glitter.enabled = true;
    mainState.spiral.enabled = true;
    mainState.ripple.enabled = true;
  }
}

void airMode() {
  targetPalette = air_gp;
  stepRate = 2;
  fadeRate = 120;
  patternsOff();
  if (audienceSpot == 0 || audienceSpot == 6 || audienceSpot == 3){
    mainState.air.enabled = true;
    mainState.air.pspeed = random8(2, 7);
    mainState.air.plength = random8(1, 10);
    stepRate = 6;
    fadeRate = 120;
  }
}

void patternsOff() {
  //turn off all the patterns, turn something on after calling it!
  mainState.wave.enabled = false;
  mainState.tail.enabled = false;
  mainState.breathe.enabled = false;
  mainState.glitter.enabled = false;
  mainState.crazytown.enabled = false;
  mainState.enlightenment.enabled = false;
  mainState.ripple.enabled = false;
  mainState.blendwave.enabled = false;
  mainState.rain.enabled = false;
  mainState.holdingPattern.enabled = false;
  mainState.mapPattern.enabled = false;
  mainState.paletteDisplay.enabled = false;
  mainState.sweep.enabled = false;
  mainState.skaters.enabled = false;
  mainState.ants.enabled = false;
  mainState.houseLights.enabled = false;
  mainState.spiral.enabled = false;
  mainState.rainbow.enabled = false;
  mainState.noise.enabled = false;
  mainState.noiseFade.enabled = false;
  mainState.air.enabled = false;
  mainState.fire.enabled = false;
  mainState.metal.enabled = false;
}

bool nothingIsOn() {
  return !(
    mainState.wave.enabled ||
    mainState.tail.enabled ||
    mainState.breathe.enabled ||
    mainState.glitter.enabled ||
    mainState.crazytown.enabled ||
    mainState.enlightenment.enabled ||
    mainState.ripple.enabled ||
    mainState.blendwave.enabled ||
    mainState.rain.enabled ||
    mainState.holdingPattern.enabled ||
    mainState.mapPattern.enabled ||
    mainState.paletteDisplay.enabled ||
    mainState.sweep.enabled ||
    mainState.skaters.enabled ||
    mainState.ants.enabled || 
    mainState.houseLights.enabled ||
    mainState.launch.enabled ||
    mainState.spiral.enabled ||
    mainState.rainbow.enabled ||
    mainState.noise.enabled ||
    mainState.air.enabled ||
    mainState.fire.enabled ||
    mainState.metal.enabled
  );
}

void nextPalette() {
  paletteCycleIndex = (paletteCycleIndex + 1) % nPalettes;
  targetPalette = cyclePalettes[paletteCycleIndex];
  Serial.printf("palette: %d\n", paletteCycleIndex);
}

void setPalette(int n) {
  paletteCycleIndex = n % nPalettes;
  targetPalette = cyclePalettes[paletteCycleIndex];  
}

void addRipple() {
  Serial.println("adding ripple");
  if (nRipplesEnabled < nRipples) {
    nRipplesEnabled++;
    for (int i=0; i<nRipplesEnabled; i++) {
      ripples[i].enable();
    }
  } else {
    ripplesOff();
  }
}

void setNRipples(int n) {
  Serial.println("setting ripples");
  if (n > nRipples) {
    n = nRipples;
  }
  nRipplesEnabled = n;
  for (int i=0; i<nRipples; i++) {
    if (i < nRipplesEnabled) {
      ripples[i].enable();
    } else {
      ripples[i].disable();
    }
  }
}

void ripplesOff() {
  for (int i=0; i<nRipples; i++) {
    ripples[i].disable();
  }
  nRipplesEnabled = 0;
  addRipple(); //keep one ripple on
}


void updatePatterns() { //render the next LED state in the buffer using the current state variables.
  frameCount++;
  if (frameCount % stepRate == 0) {
    mainState.patternStep++;
    //    Serial.println(mainState.patternStep);
    if (mainState.patternStep == 65535){
      Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
    //    memcpy(oldLeds, leds, NUM_LEDS);
    for (int i = 0; i < NUM_LEDS; i++) {
      oldLeds[i] = leds[i];
    }
  } else {
    mainState.stale = false;
    return;
  }
  //  Serial.println("running");
  int i;
  int tailPos;
  CRGB glitterMode = CRGB::White;
  fadeToBlackBy(leds, NUM_LEDS, fadeRate);

  //  if (controllerEnabled) { //maybe change to if (controller_enabled)
  //    paletteFillController();
  //  } else {
  //    redController();
  //  }

  if (mainState.wave.enabled) {
    //update wave
    //speed is hue change
    //length is hue cycle length
    for (i = 0; i < num_leds; i++) {
      leds[i].setHue(((mainState.patternStep % 256) * mainState.wave.pspeed) + (i / mainState.wave.plength));
    }
  }

  if (mainState.blendwave.enabled) {
    paletteBlendwave();
  }

  if (mainState.rainbow.enabled) {
    rainbow();
  }

  if (mainState.noise.enabled) {
    noisePattern();
  }

  if (mainState.air.enabled) {
    air();
  }
  
  if (mainState.fire.enabled) {
    Fire2012();
  }

  if (mainState.spiral.enabled) {
    spiral.animate();
  }

  if (mainState.ants.enabled) {
    ants();
  }

  if (mainState.mapPattern.enabled) {
    mapPattern();
  }

  if (mainState.holdingPattern.enabled) {
    holdingPattern();
  }

  if (mainState.ripple.enabled) {
  //    ripple();
    for (int i=0; i<nRipples; i++) {
      if (ripples[i].enabled) {
        //        Serial.printf("ripple " + i);
        ripples[i].animate();
      }
    }
  }

  if (mainState.crazytown.enabled) {
    for (int i=0; i<mainState.crazytown.plength; i++){
      if (random8() < mainState.crazytown.pspeed) {
        leds[random16(num_leds)] = ColorFromPalette(currentPalette, random8(), 250);
      }
    }
  }

  if (mainState.rain.enabled) {
    rain();
  }

  if (mainState.skaters.enabled) {
    skaters();
  }

  if (mainState.houseLights.enabled) {
    houseLights();
  }
  
  if (mainState.paletteDisplay.enabled && audienceSpot == 4) {
    paletteDesignDisplay();
  }
  
  if (mainState.tail.enabled) {
    //speed has direction
    //length is how often a tail
    if (mainState.tail.pspeed >= 0) {
      tailPos = ( ((mainState.patternStep * mainState.tail.pspeed) ) %  mainState.tail.plength);
    }
    else {
      tailPos = ( (((int)abs(65535 - mainState.patternStep) * (int)abs(mainState.tail.pspeed)) ) % mainState.tail.plength);
    }
    //    tailPos = ( ((mainState.patternStep * mainState.tail.pspeed) ) %  mainState.tail.plength);
    for (i = 0; i < num_leds; i++) {
      leds[i] = tailScale(leds[i] , abs(tailPos - (i %  mainState.tail.plength)));
    }
  }

  if (mainState.glitter.enabled) {
    if (mainState.glitter.pspeed < 0) {
      glitterMode = CRGB::Black;
    }
    if (random8() < mainState.glitter.plength) {
      leds[ random16(num_leds) ] = ColorFromPalette(currentPalette, 36, 250);
      leds[ random16(num_leds) ] = ColorFromPalette(currentPalette, 36, 250);
      leds[ random16(num_leds) ] = ColorFromPalette(currentPalette, 36, 250);
      leds[ random16(num_leds) ] = ColorFromPalette(currentPalette, 36, 250);
      leds[ random16(num_leds) ] = ColorFromPalette(currentPalette, 36, 250);
      leds[ random16(num_leds) ] = ColorFromPalette(currentPalette, 36, 250);
    }
  }

  if (mainState.metal.enabled) {
    metal();
  }
  
  if (mainState.breathe.enabled) {
    //speed is tempo
    //length is depth of darkness
    nscale8_video(leds, NUM_LEDS, ( sin8((mainState.breathe.pspeed * mainState.patternStep / 6)))); //pspeed is approx bpm
  }

  if (mainState.sweep.enabled) {
    sweep();
  }

  if (mainState.poleChaser.enabled) {
    poleChaserOff();
  }

  if (mainState.powerSaver.enabled) {
    powerSaver();
  }

  if (audienceSpot == 7) { //dim the guardian costume
    fade_video(leds, num_leds, 100);
  }

  if (mainState.noiseFade.enabled) {
    noiseFade();
  }

  if (mainState.dimmer.enabled || true) {//dimmer
  //  if (true) {
    fade_video(leds, num_leds, mainState.dimmer.plength * 25);
  //    fade_video(leds, num_leds, 150);
  }

  if (mainState.enlightenment.enabled) {
    enlightenmentBuildUp();
    //fade out more pixels progressively
    if (mainState.enlightenment.pspeed < 4) {
      for (int j = 0; j <= mainState.enlightenment.pspeed; j++) {
        uint8_t inc = primes[j];
        for (int i = 0; i < num_leds; i += inc) {
          uint8_t scale = 0;
          if (j == mainState.enlightenment.pspeed) {
            scale = 255 - mainState.enlightenment.decay;
          }
          leds[i].nscale8_video(scale);
        }
      }
    }
    if (mainState.enlightenment.pspeed == 4) {
      enlightenmentAchieved();
    }
  }

  if (mainState.launch.enabled) {
    launch();
  }

  if (false) {
    paletteFill();
  }

  //   alwaysOn();

  //  Serial.printf("%d - %d\n", frameCount, mainState.patternStep);

  //  if (mainState.patternStep >= NUM_LEDS) {//boo
  //    mainState.patternStep=0;
  //  }

  mainState.stale = false;
} // updatePatterns()

// Pattern functions, shared
CRGB tailScale (CRGB pixel, int pos) {
  switch (abs(pos)) {
    case 0:
      return (-pixel);
      break;
    case 2:
      return ( (pixel).nscale8(16) );
    default:
      return (pixel);
      break;
  }
}

void enlightenmentBuildUp() {
  float progress = (float) enlightenment.elapsed() / (float) enlightenTime;
  uint8_t attainment = 0; //Sotapanna - stream-enterer
  if (progress > 0.25) { //Sakadagami - once returner
    attainment = 1;
  }
  if (progress > 0.5) { //Anagami - non-returner
    attainment = 2;
  }
  if (progress > 0.75) { //Arahant - Deserving
    attainment = 3;
  }
  uint8_t decayVal = (progress * 255 * 4);
  if (progress >= 1) {
    //enlightenment attained!
    decayVal = 255;
    attainment = 4;
  }
  decayVal = ease8InOutApprox(decayVal);
  mainState.enlightenment.decay = decayVal;
  if (mainState.enlightenment.pspeed != attainment) {
    mainState.enlightenment.pspeed = attainment;
  }

}

void enlightenmentAchieved() {
  enlightenment.stopTimer();
  mainState.enlightenment.enabled = false;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(3000);

  //lightning
  uint8_t frequency = 5;                                       // controls the interval between strikes
  uint8_t flashes = 8;                                          //the upper limit of flashes per strike
  unsigned int dimmer = 1;
  uint8_t ledstart;                                             // Starting location of a flash
  uint8_t ledlen; // Length of a flash

  for (int fl = 0; fl < 60; fl++) {
    ledstart = random16(num_leds);                               // Determine starting location of flash
    ledlen = random8(num_leds - ledstart);                      // Determine length of flash (not to go beyond NUM_LEDS-1)

    for (int flashCounter = 0; flashCounter < random8(3, flashes); flashCounter++) {
      if (flashCounter == 0) dimmer = 5;                        // the brightness of the leader is scaled down by a factor of 5
      else dimmer = random8(1, 3);                              // return strokes are brighter than the leader

      fill_solid(leds + ledstart, ledlen, CHSV(255, 0, 255 / dimmer));
      FastLED.show();                       // Show a section of LED's
      delay(random8(2, 15));                                    // each flash only lasts 4-10 milliseconds
      fill_solid(leds + ledstart, ledlen, CHSV(255, 0, 0));     // Clear the section of LED's
      FastLED.show();

      if (flashCounter == 0) delay (random8(10));                       // longer delay until next flash after the leader

      delay(random8(10));                                   // shorter delay between strokes
    } // for()

    delay(random8(frequency * 4));                            // delay between strikes
  }
  //fade back on in flashes
  for (int k = 1; k <= 1; k++) {
    uint8_t fadeOn = 0;
    uint16_t fadeBase = 0;
    while (fadeBase < 255) {
      fadeBase += k;
      fadeOn = ease8InOutApprox(fadeBase);
      fill_solid(leds, NUM_LEDS, CHSV(128, 180, fadeOn));
      FastLED.show();
      delay(6);
    }
    fill_solid(leds, NUM_LEDS, CHSV(128, 180, 255));
    FastLED.show();
    delay(k * 50);
  }
  delay(2000);
  mainState.glitter.enabled = true;
  glitterTimer.startTimer();
}

void ripple() {
  //  fadeToBlackBy(leds, NUM_LEDS, 20);                             // 8 bit, 1 = slow, 255 = fast
  static int rippleLoc;
  static uint8_t colour;                                               // Ripple colour is randomized.
  static int center = 0;                                               // Center of the current ripple.
  static int rippleStep = -1;                                          // -1 is the initializing step.
  static uint8_t myfade = 255;                                         // Starting brightness.
  switch (rippleStep) {
    case -1:                                                          // Initialize ripple variables.
      center = random(num_leds);
      colour = random8();
      rippleStep = 0;
      break;
    case 0:
      leds[center] = ColorFromPalette(currentPalette, colour, myfade, currentBlending);
      if (frameCount % stepRate == 0) {
        rippleStep ++;
      }
      break;
    case 108:                                                    // At the end of the ripples.
      if (random8(1,10)==5) {
        rippleStep = -1;
      }
      break;
    default:                                                          // Middle of the ripples.
      rippleLoc = ease8InOutApprox(rippleStep);
      leds[(center + rippleLoc + num_leds) % num_leds] += ColorFromPalette(currentPalette, colour, myfade / (rippleLoc + 1) * 2, currentBlending); // Simple wrap from Marc Miller
      leds[(center - rippleLoc + num_leds) % num_leds] += ColorFromPalette(currentPalette, colour, myfade / (rippleLoc + 1) * 2, currentBlending);
      
      if (frameCount % stepRate == 0) {
        rippleStep ++;// Next step.
      }
      break;
  } // switch step
} // ripple()

void blendwave() {
  static CRGB clr1;
  static CRGB clr2;
  static uint8_t blendSpeed;
  static uint8_t loc1;
  static uint8_t loc2;
  static uint8_t ran1;
  static uint8_t ran2;
  blendSpeed = beatsin8(6, 0, 255);
  clr1 = blend(CHSV(beatsin8(3, 0, 255), 255, 255), CHSV(beatsin8(4, 0, 255), 255, 255), blendSpeed); //TODO get colour from palette instead
  clr2 = blend(CHSV(beatsin8(4, 0, 255), 255, 255), CHSV(beatsin8(3, 0, 255), 255, 255), blendSpeed);
  loc1 = beatsin8(10, 0, num_leds - 1);
  fill_gradient_RGB(leds, 0, clr2, loc1, clr1);
  fill_gradient_RGB(leds, loc1, clr2, num_leds - 1, clr1);
} // blendwave()

void paletteBlendwave() {
  static CRGB clr1;
  static CRGB clr2;
  static uint8_t blendSpeed;
  static uint8_t loc1;
  static uint8_t loc2;
  static uint8_t ran1;
  static uint8_t ran2;
  //  blendSpeed = beatsin8(4, 0, 255);
  blendSpeed = sin8(4 * mainState.patternStep / 6);
  //  clr1 = blend(ColorFromPalette(currentPalette, beatsin8(2, 0, 255), 255), ColorFromPalette(currentPalette, beatsin8(3, 0, 255), 255), blendSpeed); 
  //  clr2 = blend(ColorFromPalette(currentPalette, beatsin8(3, 0, 255), 255), ColorFromPalette(currentPalette, beatsin8(2, 0, 255), 255), blendSpeed);
  clr1 = blend(ColorFromPalette(currentPalette, sin8(2 * mainState.patternStep / 6), 255), ColorFromPalette(currentPalette, sin8(3 * mainState.patternStep / 6), 255), blendSpeed); 
  clr2 = blend(ColorFromPalette(currentPalette, sin8(3 * mainState.patternStep / 6), 255), ColorFromPalette(currentPalette, sin8(2 * mainState.patternStep / 6), 255), blendSpeed);
  //  loc1 = beatsin8(6, 0, num_leds - 1);
  loc1 = map(sin8(6 * mainState.patternStep / 6), 0, 255, 0, num_leds-1);
  fill_gradient_RGB(leds, 0, clr2, loc1, clr1);
  fill_gradient_RGB(leds, loc1, clr2, num_leds - 1, clr1);
} // blendwave()

void holdingPattern() {
  //  uint8_t fps = 1000000 / INTERVAL;
  //  uint8_t hPeriod = fps * 3; //every 3 seconds
  //  uint8_t hStep = mainState.patternStep % hPeriod;
  //  if (hStep == 0) {
  //    mainState.hue++;//use the hue as a palette index
  //  }
  //  for (int n = 0; n < nStrips; n++) {
  //    if (hStep < 11) {
  //      for (int j = stripLength - 3 - 11 + hStep; j < stripLength; j++) {
  //        int i = nX(n, j);
  //        leds[i] = ColorFromPalette(currentPalette, mainState.hue, 255);
  //      }
  //    }
  //    if ((hStep > 13 && hStep < 18) || (hStep > 20 && hStep < 27)) {
  //      for (int j = stripLength - 3; j < stripLength; j++) {
  //        int i = nX(n, j);
  //        leds[i] = ColorFromPalette(currentPalette, mainState.hue, 255);
  //      }
  //    }
  //  }
}

void alwaysOn() {
  if (mainState.patternStep % 144 == 0) {
    aoIndex++;
  }
  leds[aoIndex % num_leds] = ColorFromPalette(currentPalette, aoIndex % 256, 255);
}

void paletteFill() {
  for (int i = 0; i < num_leds; i++) {
    uint8_t pIdx = map(i, 0, num_leds - 1, 0, 255);
    leds[i] = ColorFromPalette(currentPalette, pIdx, 255);
  }
}

void rain() {
  static uint16_t dropPosition[30] = {0};
  static uint8_t dropSpeed[30] = {0};
  static uint16_t lastStep = mainState.patternStep;
  mainState.rain.plength = max((uint8_t) 3, mainState.rain.plength);
  //  fadeToBlackBy(leds, num_leds, 100);
  for (int nDrop = 0; nDrop < mainState.rain.plength; nDrop++) {
    if (dropSpeed[nDrop] == 0) {
      dropSpeed[nDrop] = random8(3, mainState.rain.pspeed);
    }
    if (dropPosition[nDrop] == 0) {
      dropPosition[nDrop] = max(1, stripLength - random8(1, 11));
    }
    if (mainState.patternStep != lastStep) {
      if (mainState.patternStep % dropSpeed[nDrop] == 0) {
        dropPosition[nDrop]--;
      }
    }
    //    uint8_t pos = ease8InOutApprox(dropPosition[nDrop]);
    //    uint8_t pos = map(dropPosition[nDrop], 0, 255, 0, stripLength-1);
    uint16_t pos = dropPosition[nDrop];
    for (int strip = 0; strip < nStrips; strip++) {
      int i = nX(strip, pos);
      leds[i] += ColorFromPalette(currentPalette, nDrop * 15, mainState.rain.decay);
    }
  }
  lastStep = mainState.patternStep;
}

void mapPattern() {
  for (int strip = 0; strip < nStrips; strip++) {
    for (int pos = stripLength - 1; pos >= 0; pos--) {
      int i = nX(strip, pos);
      leds[i] = ColorFromPalette(mapPalettes[mainState.mapPattern.plength - 1], (mainState.patternStep + i) * mainState.mapPattern.pspeed, 150);
    }
  }
}

void paletteDesignDisplay() {
  static uint8_t pos;
  pos = beatsin8(9, 0, stripLength);
  //  pos = (pos + stripLength/2) % stripLength;
  //  pos = pos % stripLength;
  leds[nX(0, pos)] = designColour;
  leds[nX(0, pos) + 1] = designColour;

  //current palette
  for (int ppos = 0; ppos < stripLength; ppos++) {
    uint8_t pIdx = map(ppos, 0, stripLength - 1, 0, 255);
    leds[nX(1, ppos)] = ColorFromPalette(currentDesignPalette, pIdx, 100);
  }

}

void sweep() {
  //  uint8_t sweepSpot = audienceSpot;
  long sweepDistance = abs(mainState.sweep.decay - (mainState.sweep.pspeed * sweepSpot));
  //  Serial.println(sweepDistance);
  uint8_t bright = map(sweepDistance, 0, 14 * mainState.sweep.pspeed, 255, 0);
  if (0x1 & mainState.sweep.plength){//rocker switch keeps or sets to palette
    fill_solid(leds, num_leds, ColorFromPalette(currentPalette, sweepSpot * 16));
  }
  if (mainState.sweep.plength & (0x1 << 2)){//toggle switch changes harsh or smooth
    if (sweepDistance < mainState.sweep.pspeed) {
      bright = 245;
    } else {
      bright = 10;
    }
  }
  if (mainState.sweep.plength & (0x1 << 1)) {//push switch sweeps darkness or lightness
    bright = 255 - bright;
  }
  //  Serial.println(bright);
  fade_video(leds, num_leds, bright);
  mainState.sweep.decay++;
  if (sweepDistance >= 14 * mainState.sweep.pspeed) {
    mainState.sweep.enabled = false;
  }
}

void skaters() {
  //  static uint8_t skaterPosition[num_leds/2] = {0};
  static uint8_t leaderPosition = 0;
  leaderPosition = (leaderPosition + mainState.skaters.pspeed) % stripLength;
  //  mainState.skaters.plength = min(mainState.skaters.plength, (int8_t) (9));
  for (int nSkater = 0; nSkater < mainState.skaters.plength; nSkater++) {
    uint8_t offset = sPrimes[8 - nSkater];
    for (int rep = 0; rep * offset < stripLength; rep++) {
      uint8_t pos = (leaderPosition + rep * offset) % stripLength;
      for (int strip = 0; strip < nStrips; strip++) {//eep triple nest for loop
        int i = nX(strip, pos);
  //          int i = 1;
        leds[i] = ColorFromPalette(cyclePalettes[(paletteCycleIndex + 5) % nPalettes], nSkater * 15, 255);
      }
    }
  }
}

void poleOff(int8_t n) { 
  leds[n*stripLength, (n+1)*stripLength - 1].fadeToBlackBy(255);
}

void poleChaserOff() {
  for (int pole=0; pole < nStrips; pole++) {
    if ((mainState.patternStep * mainState.poleChaser.pspeed) % nStrips == pole) {
      poleOff(pole);
    }
  }
}

void powerSaver() {
  static int start = 0;
  fade_video(leds, num_leds, 100);
  if (mainState.powerSaver.plength == 2) {
    //turn off every 7th led 
    for (int i=start; i<num_leds; i+=7) {
      leds[i].nscale8(0);
    }
  }
  if (mainState.patternStep % 244 == 0) {
    start = (start + 1) % 7;
  }
}

void ants() {
  static int leader = 0;
  static uint8_t pIdx = 13 * 16;
  uint8_t nAnts = mainState.ants.plength;
  uint8_t spacing = mainState.ants.pspeed;
  int pos;
  for (int i=0; i<nAnts; i++) {
    pos = leader - (i * spacing);
    if ((pos >= 0) && (pos < stripLength)) {
      for (int strip = 0; strip < nStrips; strip++) {
        leds[nX(strip, pos)] = ColorFromPalette(currentPalette, pIdx, mainState.ants.decay);
      }
    }
  }
  leader++;
  if (pos > stripLength) {
    leader = 0;
  }
}

void Fire2012(){
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((mainState.fire.decay * 10) / stripLength) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int s=0; s<nStrips; s++){
      for( int k= stripLength - 1; k >= 2; k--) {
        heat[nX(s, k)] = (heat[nX(s, k - 1)] + heat[nX(s, k - 2)] + heat[nX(s, k - 2)] ) / 3;
      }
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    for (int s=0; s<nStrips; s++){
      if( random8() < mainState.fire.pspeed ) {
        int y = random8(7);
        heat[nX(s, y)] = qadd8( heat[nX(s, y)], random8(160,255) );
      }
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      leds[j] = color;
    }
}

void launch() {
  static int sweepProgress = 0;
  static int sweepBase = 32;
  static int onFor = 72*6;
  static bool phase2 = false;
  static int phase2Progress = 0;
  static uint8_t lowCount = 0;
  uint8_t sweepRate = map(ease8InOutApprox(map(sweepBase, 0, 96, 0, 255)), 0, 255, 1, 96); //we want to use easing, which works on a scale of 0 to 255, but we are working on a scale of 3 to 96 instead.

  //stage 1 - fade out
  if (!phase2) {
    if (launchProgress <= 255/3) { // 255/3 = 85 frames (about 3 seconds)
      fadeToBlackBy(leds, num_leds, ease8InOutApprox(launchProgress*3));
    } else if (launchProgress < 85 + 25) { //stay black for 1 second
      fill_solid(leds, num_leds, CRGB::Black);
    } else if (launchProgress == 85 + 25) {
      fill_solid(leds, num_leds, CRGB::Black);
      patternsOff();
      mainState.paletteDisplay.enabled = false;
      controllerEnabled = false;
      fadeRate = 10;
      mainState.dimmer.plength = 1;
      sweepProgress = 0;
      targetPalette = CloudColors_p;
      if (sweepSpot == 6) {
        fadeRate = 150;
        mainState.breathe.enabled = true;
        mainState.breathe.pspeed = 30;
        mainState.ants.enabled = true;
        mainState.ants.plength = 62;
        mainState.ants.pspeed = 6;
      }
    } else if (sweepBase > 0) {
      if (sweepSpot != 6) {
        if ((sweepProgress >= sweepRate * sweepSpot) && (sweepProgress < (sweepRate * sweepSpot) + onFor)) {
          houseLights();
        }
      }
      sweepProgress++;
      if (sweepProgress == sweepRate * 6) {
        onFor = 3;
        fadeRate = min(fadeRate + 10, 255);
        sweepProgress = 0;
        lowCount++;
        mainState.ants.pspeed = sweepRate;
        if (sweepBase > 16) {
          sweepBase -= 4;
        } else if (sweepBase > 8) {
          sweepBase--;
        } else if ((sweepBase > 6) && (lowCount % 2 == 0)) {
          sweepBase--;
        } else if ((sweepBase > 4) && (lowCount % 4 == 0)) {
          sweepBase--;
        } else if ((sweepBase > 1) && (lowCount % 6 == 0)) {
          sweepBase--;
        } else if ((sweepBase == 1) && (lowCount % 12 == 0)) {
          sweepBase--;
        }
        if (sweepBase == 52) {
          mainState.tail.enabled = true;
          mainState.tail.plength = 255;
          mainState.tail.pspeed = 1;
        }
        if (sweepBase == 40) {
          mainState.tail.pspeed = 2;
        }
        if (sweepBase == 28) {
          mainState.tail.pspeed = 3;
        }
        if (sweepBase == 16) {
          mainState.tail.pspeed = 4;
        }
        Serial.printf("sweepBase: %d, sweepRate: %d\n", sweepBase, sweepRate);
      }
    } else {
      phase2 = true;
      houseLights();
      mainState.dimmer.plength = 0;
      fadeRate = 1;
      stepRate = 1;
      mainState.ants.enabled = true;
      mainState.ants.plength = 200;
      mainState.ants.pspeed = 6;  
      mainState.ants.decay = 150;
    }
  }

  if (phase2) {
    //5 seconds of house lights
    if (phase2Progress < 175) {
      houseLights();
    } else if ((phase2Progress % 2 == 0) && (fadeRate < 255)) {
      fadeRate++;
    } else if (phase2Progress < 1023) {
      if (phase2Progress % 75 == 0) {
        mainState.ants.pspeed += 4;
      }
    } else if (phase2Progress == 1023) {
      mainState.launch.enabled = false;
      tranquilityMode();
      mainState.ants.enabled = true;
      mainState.glitter.enabled = true;
      mainState.glitter.pspeed = 1;
      mainState.glitter.plength = 255;
      glitterTimer.startTimer();
      fadeRate = 10;
      controllerEnabled = true;
      phase2 = false;
      launchProgress = 0;
      sweepProgress = 0;
      sweepBase = 64;
      onFor = 72*6;
      phase2Progress = 0;
    }
    phase2Progress++;
  }

  launchProgress++;
}

void houseLights() {
    fill_solid(leds, num_leds, CRGB::White);
    for (int i=0; i+1<num_leds; i+=3) {
        leds[i] = CRGB::Black;
        leds[i+1] = CRGB::Black;
    }
    fadeToBlackBy(leds, NUM_LEDS, 128);
}

void rainbow() {
    for (int i=0; i<NUM_LEDS; i++) {
      leds[i] = ColorFromPalette(RainbowColors_p, mainState.patternStep * mainState.rainbow.pspeed + i * mainState.rainbow.plength, mainState.rainbow.decay);
    }
}

void noisePattern() {
  for (int i=0; i<num_leds; i++) {
//    uint8_t val = (3*inoise8(i*mainState.noise.plength, mainState.patternStep * mainState.noise.pspeed)%256);
    uint8_t val = inoise8(i*mainState.noise.plength, mainState.patternStep * mainState.noise.pspeed);
    int vm = map(val, 0, 255, -80, 255+80);
    uint8_t pv = (uint8_t)min(255, max(vm, 0));
    leds[i] = ColorFromPalette(currentPalette, pv, 255);
//    Serial.println(val);
//    uint8_t bri = (val + 0) % 256;
//    bri = max(0, bri - 130);
//    bri = map(bri, 0, 256-130, 0, 255);
////    bri = map(bri, 0, 255, 0, 10);
////    bri = map(bri, 0, 10, 0, 120);
//    bri = dim8_video(bri);
//    if (bri > 0) {
//      leds[i] = ColorFromPalette(currentPalette, val, bri);
//    }
  }
}

void noiseFade() {
  for (int i=0; i<num_leds; i++) {
  //    uint8_t val = (uint8_t) max((int8_t) 0, (int8_t) (1.2*inoise8(i*mainState.noiseFade.plength*2, mainState.patternStep * mainState.noiseFade.pspeed)-20));
    uint8_t val = inoise8(i*mainState.noiseFade.plength*2, mainState.patternStep * mainState.noiseFade.pspeed);
    int vm = map(val, 0, 255, -190, 300);
    uint8_t pv = (uint8_t)min(255, max(vm, 0));
    leds[i] %= pv;
  }
}

void air() {
  for (int i=0; i<num_leds; i++) {
  //    uint8_t val = (uint8_t) max((int8_t) 0, (int8_t) (1.2*inoise8(i*mainState.noiseFade.plength*2, mainState.patternStep * mainState.noiseFade.pspeed)-20));
    uint8_t val = inoise8(i*mainState.air.plength*2, mainState.patternStep * mainState.air.pspeed);
    if (val > 100 && val < 120){
      val = map(val, 100, 120, 10, 240);
      leds[i] = ColorFromPalette(currentPalette, val, 250);
    }
  }
}

void metal(){  
  for (int i=0; i<mainState.metal.pspeed; i++){
    if (random8() < mainState.metal.plength) {
      uint16_t r = random16(num_leds);
      leds[nX(random8(nStrips), random8(stripLength))] = ColorFromPalette(currentPalette, random8(), 250);
    }
  }
}

uint16_t nX(uint8_t n, int x) {
  uint16_t i;
  n = min(n, nStrips);
  x = min(x, stripLength - 1);

  if ( stripDirection[n] == 1 ) {
    i = (n) * stripLength + x;
  } else {
    i = (n) * stripLength + stripLength - x - 1;
  }
  return min(i, num_leds);
}

void dontGetBored(){
  Serial.println("Bored!");
  
  if (boredTimer.isItTime()) {
    boredTimer.resetTimer();
    if (!mainState.launch.enabled){
      if (random8() > 200) {
        earthMode();
      } else if (random8() > 200) {
        fireMode();
      } else if (random8() > 200) {
        airMode();
      } else if (random8() > 200) {
        waterMode();
      } else if (random8() > 200) {
        metalMode();
      } else if (random8() > 210) {
        doubleRainbow();
      } else if (random8() > 210) {
        tranquilityMode();
      } else if (random8() > 210) {
        tripperTrapMode();
      } else if (random8() > 210) {
        antsMode();
      } else if (random8() > 200) {
        upset_mainState();
      }
    }
  }
}


void processWSMessage(){
  DeserializationError error;
  error = deserializeJson(wsMsg, wsMsgString);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  Serial.print("we loaded a json!");
  serializeJson(wsMsg, Serial);

  uint8_t mtype = wsMsg["msgType"].as<int>();
    //  Serial.println("that json again is:");
    //  serializeJson(wsMsg, Serial);
  if (mtype) {
    Serial.printf(" of msgType %i\n", mtype);
    boredTimer.resetTimer();
   switch (mtype) {
     case 1:
       upset_mainState();
       break;
     case 2:
       doubleRainbow();
       break;
     case 3:
       tranquilityMode();
       break;
     case 4:
       nextPalette();
       break;
     case 5:
       tripperTrapMode();
       break;
     case 6:
       antsMode();
       break;
     case 7:
       shootingStars();
       break;
     case 8:
       mainState.houseLights.enabled = !mainState.houseLights.enabled;
       break;
     case 9:
       mainState.launch.enabled = true;
       break;
     case 10:
       stepRate = wsMsg["val"].as<int>();
       stepRate = max((uint16_t)1, stepRate);
       break;
     case 11:
       fadeRate = wsMsg["val"].as<int>();
       fadeRate = max((uint8_t)1, fadeRate);
       fadeRate = min((uint8_t)255, fadeRate);
       break;
     case 12:
       mainState.dimmer.plength = wsMsg["val"].as<int>();
       mainState.dimmer.plength = min((uint8_t)10, mainState.dimmer.plength);
       break;
     case 13:
       setNRipples(wsMsg["val"].as<int>());
       break;
     case 14:
       noiseTest();
       break;
     case 15:
       noiseFader();
       break;
     case 16:
       mainState.noise.plength = wsMsg["val"].as<int>();
       break;
     case 17:
       mainState.noise.pspeed = wsMsg["val"].as<int>();
       break;
     case 18:
       mainState.noiseFade.plength = wsMsg["val"].as<int>();
       break;
     case 19:
       mainState.noiseFade.pspeed = wsMsg["val"].as<int>();
       break;
     case 20:
       earthMode();
       break;
     case 21:
       fireMode();
       break;
     case 22:
       airMode();
       break;
     case 23:
       waterMode();
       break;
     case 24:
       metalMode();
       break;
     case 25:
       mainState.air.plength = wsMsg["val"].as<int>();
       break;
     case 26:
       mainState.air.pspeed = wsMsg["val"].as<int>();
       break;
     case 28:
       rippleGeddon();
       break;
     case 29:
       tailTime();
       break;
     case 30:
       blender();
       break;
     case 31:
       flashGrid();
       break;
   }
  }
}
