#ifdef ESP8266
//  dedoop;
  // //for esp8266 
  // #define ESP8266 true
  #define FASTLED_ESP8266_D1_PIN_ORDER

  //- APA102C
  //#define DATA_PIN 14
  //#define CLOCK_PIN 13

  //- WS2812
  #define DATA_PIN_1 4
#else

  
//  shoop;
  #define FASTLED_ESP32_I2S true
  #define   LED  2       // GPIO number of connected LED, ON ESP-12 IS GPIO2
  #define DATA1_CLOCK1 14 //maybe switch to 27?
  #define DATA_PIN_1 27
  #define DATA2_DATA1 27 //Maybe switch to 14?
  #define DATA_PIN_2 14
  #define DATA3_DATA2 15
  #define DATA2_3PIN 15
  #define DATA_PIN_3 15
  #define CLOCK2 2
#endif
