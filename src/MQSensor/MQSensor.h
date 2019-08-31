#ifndef MQSensor_h
#define MQSensor_h
#include <stdint.h>

#include <SPI.h> //Library for SPI interface
#include <Wire.h> //Library for I2C interface

#define     CHILD_ID_MQ                   0
/************************Hardware Related Macros************************************/
#define     MQ_SENSOR_ANALOG_PIN         (0)  //define which analog input channel you are going to use
#define         RL_VALUE                     (5)     //define the load resistance on the board, in kilo ohms
#define         RO_CLEAN_AIR_FACTOR          (9.83)  //RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,
//which is derived from the chart in datasheet
/***********************Software Related Macros************************************/
#define         CALIBARAION_SAMPLE_TIMES     (50)    //define how many samples you are going to take in the calibration phase
#define         CALIBRATION_SAMPLE_INTERVAL  (500)   //define the time interval(in milliseconds) between each samples in the
//calibration phase
#define         READ_SAMPLE_INTERVAL         (50)    //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_TIMES            (5)     //define the time interval(in milliseconds) between each samples in
//normal operation
/**********************Application Related Macros**********************************/
#define         GAS_LPG                      (0)
#define         GAS_CO                       (1)
#define         GAS_SMOKE                    (2)

extern int gas_sensor;

class MQSensorClass {
    public:
      void getMQ4();
      float MQResistanceCalculation(int raw_adc);
      float MQCalibration(int mq_pin);
      float MQRead(int mq_pin);
      int  MQGetPercentage(float rs_ro_ratio, float *pcurve);
      int MQGetGasPercentage(float rs_ro_ratio, int gas_id);
      void getMQ7();
    };

#endif
