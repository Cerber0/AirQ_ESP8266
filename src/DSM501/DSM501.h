/* Connect the DSM501 sensor as follow
 * https://www.elektronik.ropla.eu/pdf/stock/smy/dsm501.pdf
 * 1 green vert - Not used
 * 2 yellow jaune - Vout2 - 1 microns (PM1.0)
 * 3 white blanc - Vcc
 * 4 red rouge - Vout1 - 2.5 microns (PM2.5)
 * 5 black noir - GND
*/
#ifndef DSM501_h
#define DSM501_h

#include <stdint.h>

#include <SPI.h> //Library for SPI interface
#include <Wire.h> //Library for I2C interface

#define DUST_SENSOR_DIGITAL_PIN_PM10  D3        // DSM501 Pin 2 of DSM501 (jaune / Yellow)
#define DUST_SENSOR_DIGITAL_PIN_PM25  D5        // DSM501 Pin 4 (rouge / red)
#define COUNTRY                       0         // 0. France, 1. Europe, 2. USA/China
#define EXCELLENT                     "Excelente"
#define GOOD                          "Bueno"
#define ACCEPTABLE                    "Medio"
#define MODERATE                      "Mediocre"
#define HEAVY                         "Malo"
#define SEVERE                        "Muy malo"
#define HAZARDOUS                     "Peligroso"

extern unsigned long   sampletime_ms;  // Durée de mesure - sample time (ms)
extern unsigned long   sampletime_60m;
extern unsigned long   sampletime_24h;

extern unsigned long   AQIPM10;
extern unsigned long   AQIPM25;

struct structAQI{
  // variable enregistreur - recorder variables
  unsigned long   durationPM10;
  unsigned long   lowpulseoccupancyPM10;
  unsigned long   durationPM25;
  unsigned long   lowpulseoccupancyPM25;
  unsigned long   starttime;
  unsigned long   endtime;
  // Sensor AQI data
  float         concentrationPM25;
  float         concentrationPM10;
  int           AqiPM10;
  int           AqiPM25 ;
  // Indicateurs AQI - AQI display
  int           AQI;
  String        AqiString;
  int           AqiColor;
};


extern struct structAQI AQI;
extern struct structAQI AQI60M;
extern struct structAQI AQI24H;

class DSM501Class {
    public:
/*
 * Calcul l'indice de qualité de l'air français ATMO
 * Calculate French ATMO AQI indicator
 */
int getATMO( int sensor, float density );

String updateAQIDisplay(int AQIaux);

int getACQI( int sensor, float density );

/*
 * AQI formula: https://en.wikipedia.org/wiki/Air_Quality_Index#United_States
 * Arduino code https://gist.github.com/nfjinjing/8d63012c18feea3ed04e
 * On line AQI calculator https://www.airnow.gov/index.cfm?action=resources.conc_aqi_calc
 */
float calcAQI(float I_high, float I_low, float C_high, float C_low, float C);

int getAQI(int sensor, float density);

void updateAQILevel(structAQI AQIaux);

void updateAQI();

void updateAQI24H();

void updateAQI60M();
};
#endif
