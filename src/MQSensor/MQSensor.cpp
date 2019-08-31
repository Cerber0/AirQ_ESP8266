#include "MQSensor.h"

int gas_sensor = 0; //Sensor pin
float m = -0.318; //Slope
float b = 1.133; //Y-Intercept
float R0 = 11.820; //Sensor Resistance in fresh air from previous code
/*****************************Globals***********************************************/

//VARIABLES
float Ro = 10000.0;    // this has to be tuned 10K Ohm
int val = 0;           // variable to store the value coming from the sensor
float valMQ =0.0;
float lastMQ =0.0;
float           LPGCurve[3]  =  {2.3,0.21,-0.47};   //two points are taken from the curve.
//with these two points, a line is formed which is "approximately equivalent"
//to the original curve.
//data format:{ x, y, slope}; point1: (lg200, 0.21), point2: (lg10000, -0.59)
float           COCurve[3]  =  {2.3,0.72,-0.34};    //two points are taken from the curve.
//with these two points, a line is formed which is "approximately equivalent"
//to the original curve.
//data format:{ x, y, slope}; point1: (lg200, 0.72), point2: (lg10000,  0.15)
float           SmokeCurve[3] = {2.3,0.53,-0.44};   //two points are taken from the curve.
//with these two points, a line is formed which is "approximately equivalent"
//to the original curve.
//data format:{ x, y, slope}; point1: (lg200, 0.53), point2:(lg10000,-0.22)


void MQSensorClass::getMQ4() {
  //display.clearDisplay(); //Clear display
  //display.setCursor(0, 5); //Place cursor in (x,y) location
  float sensor_volt; //Define variable for sensor voltage
  float RS_gas; //Define variable for sensor resistance
  float ratio; //Define variable for ratio
  uint16_t sensorValue = analogRead(gas_sensor); //Read analog values of sensor
  if (sensorValue <= 1024){
    Serial.print("Vread ");Serial.println(sensorValue);
    sensor_volt = sensorValue * (5.0 / 1024.0); //Convert analog values to voltage
    Serial.print("V ");Serial.println(sensor_volt);
    RS_gas = ((5.0 * 10.0) / sensor_volt) - 10.0; //Get value of RS in a gas
    //RS_gas = (5.0 - sensor_volt) / sensor_volt;
    ratio = RS_gas / R0;   // Get ratio RS_gas/RS_air

  //double ppm_log = (log10(ratio) - b) / m; //Get ppm value in linear scale according to the the ratio value
  //double ppm = pow(10, ppm_log); //Convert ppm value to log scaleç
    double x = 1538.46 * ratio;
    double ppm = pow(x,-1.709);
    double percentage = ppm / 10000; //Convert to percentage
    Serial.print("ppm => ");
    Serial.print(percentage); //Load screen buffer with percentage value
    Serial.println("%"); //Load screen buffer with "%"
    //display.display(); //Flush characters to screen
 }
  //if (ppm > 2000) {
    //Check if ppm value is greater than 2000
  //  digitalWrite(led, HIGH); //Turn LED on
  //  digitalWrite(buzzer, HIGH); //Turn buzzer on
  //} else {
    //Case ppm is not greater than 2000
    //digitalWrite(led, LOW);
    //Turn LED off
    //digitalWrite(buzzer, LOW);
    //Turn buzzer off
  //}
}

/****************** MQResistanceCalculation ****************************************
Input:   raw_adc - raw value read from adc, which represents the voltage
Output:  the calculated sensor resistance
Remarks: The sensor and the load resistor forms a voltage divider. Given the voltage
         across the load resistor and its resistance, the resistance of the sensor
         could be derived.
************************************************************************************/
float MQSensorClass::MQResistanceCalculation(int raw_adc)
{
    return ( ((float)RL_VALUE*(1023-raw_adc)/raw_adc));
}

/***************************** MQCalibration ****************************************
Input:   mq_pin - analog channel
Output:  Ro of the sensor
Remarks: This function assumes that the sensor is in clean air. It use
         MQResistanceCalculation to calculates the sensor resistance in clean air
         and then divides it with RO_CLEAN_AIR_FACTOR. RO_CLEAN_AIR_FACTOR is about
         10, which differs slightly between different sensors.
************************************************************************************/
float MQSensorClass::MQCalibration(int mq_pin)
{
    int i;
    float val=0;

    for (i=0; i<CALIBARAION_SAMPLE_TIMES; i++) {          //take multiple samples
        val += MQResistanceCalculation(analogRead(mq_pin));
        delay(CALIBRATION_SAMPLE_INTERVAL);
    }
    val = val/CALIBARAION_SAMPLE_TIMES;                   //calculate the average value

    val = val/RO_CLEAN_AIR_FACTOR;                        //divided by RO_CLEAN_AIR_FACTOR yields the Ro
    //according to the chart in the datasheet

    return val;
}
/*****************************  MQRead *********************************************
Input:   mq_pin - analog channel
Output:  Rs of the sensor
Remarks: This function use MQResistanceCalculation to calculate the sensor resistance (Rs).
         The Rs changes as the sensor is in the different concentration of the target
         gas. The sample times and the time interval between samples could be configured
         by changing the definition of the macros.
************************************************************************************/
float MQSensorClass::MQRead(int mq_pin)
{
    int i;
    float rs=0;
    int valRead=0;

    for (i=0; i<READ_SAMPLE_TIMES; i++) {
        valRead = analogRead(mq_pin);
        //Serial.println(valRead);
        rs += MQResistanceCalculation(valRead);
        delay(READ_SAMPLE_INTERVAL);
    }

    rs = rs/READ_SAMPLE_TIMES;

    return rs;
}

/*****************************  MQGetPercentage **********************************
Input:   rs_ro_ratio - Rs divided by Ro
         pcurve      - pointer to the curve of the target gas
Output:  ppm of the target gas
Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm)
         of the line could be derived if y(rs_ro_ratio) is provided. As it is a
         logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic
         value.
************************************************************************************/
int  MQSensorClass::MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
    return (pow(10,( ((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));
}


/*****************************  MQGetGasPercentage **********************************
Input:   rs_ro_ratio - Rs divided by Ro
         gas_id      - target gas type
Output:  ppm of the target gas
Remarks: This function passes different curves to the MQGetPercentage function which
         calculates the ppm (parts per million) of the target gas.
************************************************************************************/
int MQSensorClass::MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
    if ( gas_id == GAS_LPG ) {
        return MQGetPercentage(rs_ro_ratio,LPGCurve);
    } else if ( gas_id == GAS_CO ) {
        return MQGetPercentage(rs_ro_ratio,COCurve);
    } else if ( gas_id == GAS_SMOKE ) {
        return MQGetPercentage(rs_ro_ratio,SmokeCurve);
    }

    return 0;
}


void MQSensorClass::getMQ7(){
  uint32_t valMQ = MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN)/Ro,GAS_CO);
  Serial.println(valMQ);

  Serial.print("LPG:");
  Serial.print(MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN)/Ro,GAS_LPG) );
  Serial.print( "ppm" );
  Serial.print("    ");
  Serial.print("CO:");
  Serial.print(MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN)/Ro,GAS_CO) );
  Serial.print( "ppm" );
  Serial.print("    ");
  Serial.print("SMOKE:");

  Serial.print(MQGetGasPercentage(MQRead(MQ_SENSOR_ANALOG_PIN)/Ro,GAS_SMOKE) );
  Serial.println( "ppm" );

  if (valMQ != lastMQ) {
      Serial.println(valMQ);
      lastMQ = ceil(valMQ);
  }
}
