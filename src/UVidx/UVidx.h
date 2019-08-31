#ifndef UVidx_h
#define UVidx_h
#include <SPI.h> //Library for SPI interface
#include <Wire.h> //Library for I2C interface


#define UV_SENSOR_ANALOG_PIN 0
extern float uvIndex;

class UVidxClass {
    public:
      void getUV();
    };
#endif
