struct pattern {
  bool enabled;
  int8_t pspeed;
  uint8_t plength;
  int16_t decay; 
};

typedef struct patternState {
  uint32_t lastUpdate;
  uint32_t nextUpdate;
  bool stale;
  uint8_t hue;
  uint16_t patternStep;
  pattern wave;
  pattern tail;
  pattern breathe;
  pattern glitter;
  pattern crazytown;
  pattern enlightenment;
  pattern ripple;
  pattern blendwave;
  pattern rain;
  pattern holdingPattern;
  pattern mapPattern;
  pattern paletteDisplay;
  pattern sweep;
  pattern dimmer;
  pattern skaters;
  pattern poleChaser;
  pattern powerSaver;
  pattern ants;
  pattern launch;
  pattern houseLights;
  pattern spiral;
  pattern rainbow;
  pattern noise;
  pattern noiseFade;
  pattern air;
  pattern fire;
  pattern metal;
};

// wave length 1 to 10
// crazytown length is density
// speed -4 to 5
patternState mainState = {
  0, 0, true, 0, 0, 
  {false, 1, 20, 250}, //wave
  {false, 2, 30, 150}, //tail
  {false, 15, 200, 0}, //breathe
  {false, 0, 50, 90}, //glitter
  {false, 0, 15, 250}, //crazytown
  {false, 0, 0, 0}, //enlightenment
  {false, 0, 0, 200}, //ripple
  {false, 0, 0, 140}, //blendwave
  {true, 50, 10, 150}, //rain
  {false, 0, 0, 0}, //holdingPattern
  {false, 0, 0, 0}, //mapPattern
  {false, 0, 0, 0}, //paletteDisplay
  {false, 0, 0, 0}, //sweep
  {false, 0, 0, 0}, //dimmer
  {false, 1, 1, 0}, //skaters
  {false, 1, 1, 1}, //poleChaser
  {false, 0, 0, 0}, //powerSaver
  {false, 3, 10, 70}, //ants
  {false, 0, 0, 0}, //launch
  {false, 0, 0, 0}, //houseLights
  {false, 0, 0, 0}, //spiral
  {false, 5, 5, 250}, //rainbow
  {false, 5, 5, 250}, //noise
  {true, 5, 5, 250}, //noiseFade
  {false, 11, 5, 250}, //air
  {false, 120, 5, 55}, //fire
  {false, 120, 15, 55}, //metal
  };

pattern *patternPointers[] = {
  &mainState.wave,//0
  &mainState.tail,//1
  &mainState.glitter,//2
  &mainState.ripple,//3
  &mainState.blendwave,//4
  &mainState.rain,//5
  &mainState.ants,//6
  &mainState.noiseFade,//7
  &mainState.spiral,//8
  &mainState.rainbow,//9
  &mainState.noise,//10
  &mainState.noiseFade,//11  
};
