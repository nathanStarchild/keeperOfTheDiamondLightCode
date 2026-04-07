struct pattern {
  bool enabled;
  int8_t pspeed;
  uint8_t plength;
  uint8_t decay; 
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
  pattern theVoid;
  pattern theBlob;
  pattern noise2D;
  pattern pi;
  pattern rainbowZoom;
  pattern nodeCount;
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
  {false, 25, 0, 64}, //sweep
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
  {false, 1, 30, 30}, //theVoid
  {false, 1, 30, 30}, //theBlob
  {false, 5, 15, 250}, //noise2D
  {false, 5, 15, 250}, //pi
  {false, 5, 15, 250}, //rainbowZoom
  {false, 50, 1, 250}, //nodeCount
  {false, 0, 0, 0}, //rollCall
  };

pattern *patternPointers[] = {
  &mainState.wave,//0
  &mainState.tail,//1
  &mainState.breathe,//2
  &mainState.glitter,//3
  &mainState.crazytown,//4
  &mainState.enlightenment,//5
  &mainState.ripple,//6
  &mainState.blendwave,//7
  &mainState.rain,//8
  &mainState.holdingPattern,//9
  &mainState.mapPattern,//10
  &mainState.paletteDisplay,//11
  &mainState.sweep,//12
  &mainState.dimmer,//13
  &mainState.skaters,//14
  &mainState.poleChaser,//15
  &mainState.powerSaver,//16
  &mainState.ants,//17
  &mainState.launch,//18
  &mainState.houseLights,//19
  &mainState.spiral,//20
  &mainState.rainbow,//21
  &mainState.noise,//22
  &mainState.noiseFade,//23
  &mainState.air,//24
  &mainState.fire,//25
  &mainState.metal,//26
  &mainState.theVoid,//27
  &mainState.theBlob,//28
  &mainState.noise2D,//29
  &mainState.pi,//30
  &mainState.rainbowZoom,//31
  &mainState.nodeCount,//32
  &mainState.rollCall,//33
};

const char* patternNames[] = {
  "wave",           //0
  "tail",           //1
  "breathe",        //2
  "glitter",        //3
  "crazytown",      //4
  "enlightenment",  //5
  "ripple",         //6
  "blendwave",      //7
  "rain",           //8
  "holdingPattern", //9
  "mapPattern",     //10
  "paletteDisplay", //11
  "sweep",          //12
  "dimmer",         //13
  "skaters",        //14
  "poleChaser",     //15
  "powerSaver",     //16
  "ants",           //17
  "launch",         //18
  "houseLights",    //19
  "spiral",         //20
  "rainbow",        //21
  "noise",          //22
  "noiseFade",      //23
  "air",            //24
  "fire",           //25
  "metal",          //26
  "theVoid",        //27
  "theBlob",        //28
  "noise2D",        //29
  "pi",             //30
  "rainbowZoom",    //31
  "nodeCount",      //32
  "rollCall"        //33
};

const int nPatterns = sizeof(patternPointers) / sizeof(patternPointers[0]);
