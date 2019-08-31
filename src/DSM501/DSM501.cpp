#include "DSM501.h"

struct structAQI AQI;
struct structAQI AQI60M;
struct structAQI AQI24H;

unsigned long   AQIPM10 = 0;
unsigned long   AQIPM25 = 0;
unsigned long   duration;
unsigned long   starttime;
unsigned long   endtime;
unsigned long   lowpulseoccupancy = 0;

unsigned long   sampletime_ms = 3000;  // Durée de mesure - sample time (ms)
unsigned long   sampletime_60m = 3600000;
unsigned long   sampletime_24h = 86400000;

/*
 * Calcul l'indice de qualité de l'air français ATMO
 * Calculate French ATMO AQI indicator
 */
int DSM501Class::getATMO( int sensor, float density ){
  if ( sensor == 0 ) { //PM2,5
    if ( density <= 11 ) {
      return 1;
    } else if ( density > 11 && density <= 24 ) {
      return 2;
    } else if ( density > 24 && density <= 36 ) {
      return 3;
    } else if ( density > 36 && density <= 41 ) {
      return 4;
    } else if ( density > 41 && density <= 47 ) {
      return 5;
    } else if ( density > 47 && density <= 53 ) {
      return 6;
    } else if ( density > 53 && density <= 58 ) {
      return 7;
    } else if ( density > 58 && density <= 64 ) {
      return 8;
    } else if ( density > 64 && density <= 69 ) {
      return 9;
    } else {
      return 10;
    }
  } else {
    if ( density <= 6 ) {
      return 1;
    } else if ( density > 6 && density <= 13 ) {
      return 2;
    } else if ( density > 13 && density <= 20 ) {
      return 3;
    } else if ( density > 20 && density <= 27 ) {
      return 4;
    } else if ( density > 27 && density <= 34 ) {
      return 5;
    } else if ( density > 34 && density <= 41 ) {
      return 6;
    } else if ( density > 41 && density <= 49 ) {
      return 7;
    } else if ( density > 49 && density <= 64 ) {
      return 8;
    } else if ( density > 64 && density <= 79 ) {
      return 9;
    } else {
      return 10;
    }
  }
}

String DSM501Class::updateAQIDisplay(int AQIaux){
  /*
   * 1 EXCELLENT
   * 2 GOOD
   * 3 ACCEPTABLE
   * 4 MODERATE
   * 5 HEAVY
   * 6 SEVERE
   * 7 HAZARDOUS
   */
  String AqiString = "";
  if ( COUNTRY == 0 ) {
    // Système ATMO français - French ATMO AQI system
    switch ( AQIaux) {
      case 10:
        AqiString = SEVERE;
        break;
      case 9:
        AqiString = HEAVY;
        break;
      case 8:
        AqiString = HEAVY;
        break;
      case 7:
        AqiString = MODERATE;
        break;
      case 6:
        AqiString = MODERATE;
        break;
      case 5:
        AqiString = ACCEPTABLE;
        break;
      case 4:
        AqiString = GOOD;
        break;
      case 3:
        AqiString = GOOD;
        break;
      case 2:
        AqiString = EXCELLENT;
        break;
      case 1:
        AqiString = EXCELLENT;
        break;
      }
  } else if ( COUNTRY == 1 ) {
    // European CAQI
    switch ( AQIaux) {
      case 25:
        AqiString = GOOD;
        break;
      case 50:
        AqiString = ACCEPTABLE;
        break;
      case 75:
        AqiString = MODERATE;
        break;
      case 100:
        AqiString = HEAVY;
        break;
      default:
        AqiString = SEVERE;
      }
  } else if ( COUNTRY == 2 ) {
    // USA / CN
    if ( AQIaux <= 50 ) {
        AqiString = GOOD;
    } else if ( AQIaux > 50 && AQIaux <= 100 ) {
        AqiString = ACCEPTABLE;
    } else if ( AQIaux > 100 && AQIaux <= 150 ) {
        AqiString = MODERATE;
    } else if ( AQIaux > 150 && AQIaux <= 200 ) {
        AqiString = HEAVY;
    } else if ( AQIaux > 200 && AQIaux <= 300 ) {
        AqiString = SEVERE;
    } else {
       AqiString = HAZARDOUS;
    }
  }
  return AqiString;
}
/*
 * CAQI Européen - European CAQI level
 * source : http://www.airqualitynow.eu/about_indices_definition.php
 */

int DSM501Class::getACQI( int sensor, float density ){
  if ( sensor == 0 ) {  //PM2,5
    if ( density == 0 ) {
      return 0;
    } else if ( density <= 15 ) {
      return 25 ;
    } else if ( density > 15 && density <= 30 ) {
      return 50;
    } else if ( density > 30 && density <= 55 ) {
      return 75;
    } else if ( density > 55 && density <= 110 ) {
      return 100;
    } else {
      return 150;
    }
  } else {              //PM10
    if ( density == 0 ) {
      return 0;
    } else if ( density <= 25 ) {
      return 25 ;
    } else if ( density > 25 && density <= 50 ) {
      return 50;
    } else if ( density > 50 && density <= 90 ) {
      return 75;
    } else if ( density > 90 && density <= 180 ) {
      return 100;
    } else {
      return 150;
    }
  }
}

/*
 * AQI formula: https://en.wikipedia.org/wiki/Air_Quality_Index#United_States
 * Arduino code https://gist.github.com/nfjinjing/8d63012c18feea3ed04e
 * On line AQI calculator https://www.airnow.gov/index.cfm?action=resources.conc_aqi_calc
 */
float DSM501Class::calcAQI(float I_high, float I_low, float C_high, float C_low, float C) {
  return (I_high - I_low) * (C - C_low) / (C_high - C_low) + I_low;
}

int DSM501Class::getAQI(int sensor, float density) {
  int d10 = (int)(density * 10);
  if ( sensor == 0 ) {
    if (d10 <= 0) {
      return 0;
    }
    else if(d10 <= 120) {
      return calcAQI(50, 0, 120, 0, d10);
    }
    else if (d10 <= 354) {
      return calcAQI(100, 51, 354, 121, d10);
    }
    else if (d10 <= 554) {
      return calcAQI(150, 101, 554, 355, d10);
    }
    else if (d10 <= 1504) {
      return calcAQI(200, 151, 1504, 555, d10);
    }
    else if (d10 <= 2504) {
      return calcAQI(300, 201, 2504, 1505, d10);
    }
    else if (d10 <= 3504) {
      return calcAQI(400, 301, 3504, 2505, d10);
    }
    else if (d10 <= 5004) {
      return calcAQI(500, 401, 5004, 3505, d10);
    }
    else if (d10 <= 10000) {
      return calcAQI(1000, 501, 10000, 5005, d10);
    }
    else {
      return 1001;
    }
  } else {
    if (d10 <= 0) {
      return 0;
    }
    else if(d10 <= 540) {
      return calcAQI(50, 0, 540, 0, d10);
    }
    else if (d10 <= 1540) {
      return calcAQI(100, 51, 1540, 541, d10);
    }
    else if (d10 <= 2540) {
      return calcAQI(150, 101, 2540, 1541, d10);
    }
    else if (d10 <= 3550) {
      return calcAQI(200, 151, 3550, 2541, d10);
    }
    else if (d10 <= 4250) {
      return calcAQI(300, 201, 4250, 3551, d10);
    }
    else if (d10 <= 5050) {
      return calcAQI(400, 301, 5050, 4251, d10);
    }
    else if (d10 <= 6050) {
      return calcAQI(500, 401, 6050, 5051, d10);
    }
    else {
      return 1001;
    }
  }
}

void DSM501Class::updateAQILevel(structAQI AQIaux){
  AQIaux.AQI = AQIaux.AqiPM10;
}

void DSM501Class::updateAQI() {
  // Actualise les mesures - update measurements
  AQI.endtime = millis();
  float ratio = AQI.lowpulseoccupancyPM10 / (sampletime_ms * 10.0);
  float concentration = 1.1 * pow( ratio, 3) - 3.8 *pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
  if ( sampletime_ms < 3600000 ) { concentration = concentration * ( sampletime_ms / 3600000.0 ); }

  AQI.concentrationPM10 = concentration;
  Serial.print("================================= ");Serial.print(sampletime_ms / 1000);Serial.println("s ====================================");
  Serial.print("Ratio => PM10: "); Serial.print(ratio);
  ratio = AQI.lowpulseoccupancyPM25 / (sampletime_ms * 10.0);
  concentration = 1.1 * pow( ratio, 3) - 3.8 *pow(ratio, 2) + 520 * ratio + 0.62;
  Serial.print(" | PM2.5: "); Serial.println(ratio);
  if ( sampletime_ms < 3600000 ) { concentration = concentration * ( sampletime_ms / 3600000.0 ); }
  Serial.print("LowPulse => PM10: "); Serial.print(AQI.lowpulseoccupancyPM10);Serial.print(" | PM2.5: "); Serial.println(AQI.lowpulseoccupancyPM25);

  AQI.lowpulseoccupancyPM10 = 0;
  AQI.lowpulseoccupancyPM25 = 0;
  AQI.concentrationPM25 = concentration;

  int temp=23;
  long ppmv=(AQI.concentrationPM10*0.0283168/100/1000) * (0.0820*temp)/0.1;
  Serial.print("PPMV => "); Serial.println(ppmv);

  Serial.print("Concentrations => PM2.5: "); Serial.print(AQI.concentrationPM25); Serial.print(" | PM10: "); Serial.println(AQI.concentrationPM10);
  AQI.starttime = millis();

  // Actualise l'AQI de chaque capteur - update AQI for each sensor
  if ( COUNTRY == 0 ) {
    // France
    AQI.AqiPM25 = getATMO( 0, AQI.concentrationPM25 );
    AQI.AqiPM10 = getATMO( 1, AQI.concentrationPM10 );
  } else if ( COUNTRY == 1 ) {
    // Europe
    AQI.AqiPM25 = getACQI( 0, AQI.concentrationPM25 );
    AQI.AqiPM10 = getACQI( 1, AQI.concentrationPM10 );
  } else {
    // USA / China
    AQI.AqiPM25 = getAQI( 0, AQI.concentrationPM25 );
    AQI.AqiPM10 = getAQI( 0, AQI.concentrationPM10 );
  }

  // Actualise l'indice AQI - update AQI index

  AQI.AQI = AQI.AqiPM10;
  AQI.AqiString = updateAQIDisplay(AQI.AQI);

  Serial.print("AQIs => PM25: "); Serial.print(AQI.AqiPM25); Serial.print(" | PM10: "); Serial.println(AQI.AqiPM10);
  Serial.print(" | AQI: "); Serial.println(AQI.AQI); Serial.print(" | Message: "); Serial.println(AQI.AqiString);
}

void DSM501Class::updateAQI24H() {
  // Actualise les mesures - update measurements
  AQI24H.endtime = millis();
  float ratio = AQI24H.lowpulseoccupancyPM10 / (sampletime_24h * 10.0);
  float concentration = 1.1 * pow( ratio, 3) - 3.8 *pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
  if ( sampletime_24h < 3600000 ) { concentration = concentration * ( sampletime_24h / 3600000.0 ); }

  AQI24H.concentrationPM10 = concentration;
  Serial.println("================================= 24H ====================================");
  Serial.print("Ratio => PM10: "); Serial.print(ratio);
  ratio = AQI24H.lowpulseoccupancyPM25 / (sampletime_24h * 10.0);
  concentration = 1.1 * pow( ratio, 3) - 3.8 *pow(ratio, 2) + 520 * ratio + 0.62;
  Serial.print(" | PM2.5: "); Serial.println(ratio);
  if ( sampletime_24h < 3600000 ) { concentration = concentration * ( sampletime_24h / 3600000.0 ); }
  Serial.print("LowPulse => PM10: "); Serial.print(AQI24H.lowpulseoccupancyPM10);Serial.print(" | PM2.5: "); Serial.println(AQI24H.lowpulseoccupancyPM25);

  AQI24H.lowpulseoccupancyPM10 = 0;
  AQI24H.lowpulseoccupancyPM25 = 0;
  AQI24H.concentrationPM25 = concentration;

  int temp=23;
  long ppmv=(AQI24H.concentrationPM10*0.0283168/100/1000) * (0.0820*temp)/0.1;
  Serial.print("PPMV => "); Serial.println(ppmv);

  Serial.print("Concentrations => PM2.5: "); Serial.print(AQI24H.concentrationPM25); Serial.print(" | PM10: "); Serial.println(AQI24H.concentrationPM10);
  AQI24H.starttime = millis();

  // Actualise l'AQI de chaque capteur - update AQI for each sensor
  if ( COUNTRY == 0 ) {
    // France
    AQI24H.AqiPM25 = getATMO( 0, AQI24H.concentrationPM25 );
    AQI24H.AqiPM10 = getATMO( 1, AQI24H.concentrationPM10 );
  } else if ( COUNTRY == 1 ) {
    // Europe
    AQI24H.AqiPM25 = getACQI( 0, AQI24H.concentrationPM25 );
    AQI24H.AqiPM10 = getACQI( 1, AQI24H.concentrationPM10 );
  } else {
    // USA / China
    AQI24H.AqiPM25 = getAQI( 0, AQI24H.concentrationPM25 );
    AQI24H.AqiPM10 = getAQI( 0, AQI24H.concentrationPM10 );
  }

  // Actualise l'indice AQI - update AQI index
  AQI24H.AQI = AQI24H.AqiPM10;
  AQI24H.AqiString = updateAQIDisplay(AQI24H.AQI);

  Serial.print("AQIs => PM25: "); Serial.print(AQI24H.AqiPM25); Serial.print(" | PM10: "); Serial.println(AQI24H.AqiPM10);
  Serial.print(" | AQI: "); Serial.println(AQI24H.AQI); Serial.print(" | Message: "); Serial.println(AQI24H.AqiString);
}

void DSM501Class::updateAQI60M() {
  // Actualise les mesures - update measurements
  AQI60M.endtime = millis();
  float ratio = AQI60M.lowpulseoccupancyPM10 / (sampletime_60m * 10.0);
  float concentration = 1.1 * pow( ratio, 3) - 3.8 *pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
  if ( sampletime_60m < 3600000 ) { concentration = concentration * ( sampletime_60m / 3600000.0 ); }

  AQI60M.concentrationPM10 = concentration;
  Serial.println("================================= 1H ====================================");
  Serial.print("Ratio => PM10: "); Serial.print(ratio);
  ratio = AQI60M.lowpulseoccupancyPM25 / (sampletime_60m * 10.0);
  concentration = 1.1 * pow( ratio, 3) - 3.8 *pow(ratio, 2) + 520 * ratio + 0.62;
  Serial.print(" | PM2.5: "); Serial.println(ratio);
  if ( sampletime_60m < 3600000 ) { concentration = concentration * ( sampletime_60m / 3600000.0 ); }
  Serial.print("LowPulse => PM10: ");Serial.print("\n"); Serial.print(AQI60M.lowpulseoccupancyPM10);Serial.print(" | PM2.5: "); Serial.println(AQI60M.lowpulseoccupancyPM25);

  AQI60M.lowpulseoccupancyPM10 = 0;
  AQI60M.lowpulseoccupancyPM25 = 0;
  AQI60M.concentrationPM25 = concentration;

  int temp=23;
  long ppmv=(AQI60M.concentrationPM10*0.0283168/100/1000) * (0.0820*temp)/0.1;
  Serial.print("PPMV => "); Serial.println(ppmv);

  Serial.print("Concentrations => PM2.5: "); Serial.print(AQI60M.concentrationPM25); Serial.print(" | PM10: "); Serial.println(AQI60M.concentrationPM10);
  AQI60M.starttime = millis();

  // Actualise l'AQI de chaque capteur - update AQI for each sensor
  if ( COUNTRY == 0 ) {
    // France
    AQI60M.AqiPM25 = getATMO( 0, AQI60M.concentrationPM25 );
    AQI60M.AqiPM10 = getATMO( 1, AQI60M.concentrationPM10 );
  } else if ( COUNTRY == 1 ) {
    // Europe
    AQI60M.AqiPM25 = getACQI( 0, AQI60M.concentrationPM25 );
    AQI60M.AqiPM10 = getACQI( 1, AQI60M.concentrationPM10 );
  } else {
    // USA / China
    AQI60M.AqiPM25 = getAQI( 0, AQI60M.concentrationPM25 );
    AQI60M.AqiPM10 = getAQI( 0, AQI60M.concentrationPM10 );
  }

  // Actualise l'indice AQI - update AQI index
  AQI60M.AQI = AQI60M.AqiPM10;
  AQI60M.AqiString = updateAQIDisplay(AQI60M.AQI);

  Serial.print("AQIs => PM25: "); Serial.print(AQI60M.AqiPM25); Serial.print(" | PM10: "); Serial.println(AQI60M.AqiPM10);
  Serial.print(" | AQI: "); Serial.println(AQI60M.AQI); Serial.print(" | Message: "); Serial.println(AQI60M.AqiString);
}
