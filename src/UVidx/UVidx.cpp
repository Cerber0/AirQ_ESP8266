#include "UVidx.h"

uint32_t lastSend =0;
float lastUV = -1;
uint16_t uvIndexValue [12] = { 50, 227, 318, 408, 503, 606, 696, 795, 881, 976, 1079, 1170};
uint32_t SLEEP_TIME = 3000; // Sleep time between reads (in milliseconds)
float uvIndex;

void UVidxClass::getUV(){
  uint32_t currentTime = millis();

    uint16_t uv = analogRead(UV_SENSOR_ANALOG_PIN);// Get UV value
    //Serial.print("UV read => ");Serial.println(uv);
    if (uv>1170) {
        uv=1170;
    }

    //Serial.print("UV Analog reading: ");
    //Serial.println(uv);

    int i;
    for (i = 0; i < 12; i++) {
        if (uv <= uvIndexValue[i]) {
            uvIndex = i;
            break;
        }
    }

    //calculate 1 decimal if possible
    if (i>0) {
        float vRange=uvIndexValue[i]-uvIndexValue[i-1];
        float vCalc=uv-uvIndexValue[i-1];
        uvIndex+=(1.0/vRange)*vCalc-1.0;
    }

    //Serial.print("UVI: ");
    //Serial.println(uvIndex,2);

    //Send value to gateway if changed, or at least every 5 minutes
    if ((uvIndex != lastUV)||(currentTime-lastSend >= 5UL*60UL*1000UL)) {
        lastUV = uvIndex;
    }
    //Serial.print("UV => ");Serial.println(uvIndex);
    //delay(SLEEP_TIME);
    //return uvIndex;
}
