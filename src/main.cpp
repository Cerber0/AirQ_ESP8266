#include <PubSubClient.h>
#include <ESP8266WiFi/src/ESP8266WiFi.h>        // Include the Wi-Fi library
#include <UVidx/UVidx.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <TroykaMQ.h>

#define DHTTYPE DHT22
#define DHTPIN            14

#define SEALEVELPRESSURE_HPA (1013.25)

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete
//SoftwareSerial softSerial(3, 1); // RX, TX
uint32_t delayMS;

char c_uvIndex_max[8];

unsigned long   sampletime_ms1 = 3000;
float temperature, humidity, pressure, altitude;
char* ssid     = "Cerber0";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "C4nc3rb3r0";     // The password of the Wi-Fi network
const char* mqtt_server = "192.168.0.162";

String mq2_lpg, mq2_methane, mq2_smoke, mq2_hydrogen, mq3_alcohol_mgl, mq3_alcohol_ppm, mq135_co2, mq8_hydrogen, mq4_methane, mq6_lpg, mq7_co;

UVidxClass UVix;
DHT_Unified dht(DHTPIN, DHTTYPE);

void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.println((char)payload[0]);
  // Switch on the LED if an 1 was received as first character
      sensors_event_t event;
      
  if ((char)payload[0] != 'U') {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }

    UVix.getUV();
    char c_uvIndex[8]; // Buffer big enough for 7-character float
    dtostrf(uvIndex, 8, 2, c_uvIndex); // Leave room for too large numbers!
    client.publish("/casa/AirQ/sensors/UV", c_uvIndex);
  }
  else if ((char)payload[0] == 'T') {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }

    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println("Error reading temperature!");
    }
    else {
      client.publish("/casa/AirQ/sensors/temperatureDHT22", String(event.temperature).c_str());
    }
  }
  else if ((char)payload[0] == 'H') {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }

    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println("Error reading humidity!");
    }
    else {
      client.publish("/casa/AirQ/sensors/humidity", String(event.relative_humidity).c_str());
    }
  }
  else if ((char)payload[0] == 'G') {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }

    client.publish("/casa/AirQ/sensors/MQ2_LPG", String(mq2_lpg).c_str());
    client.publish("/casa/AirQ/sensors/MQ2_METHANE", String(mq2_methane).c_str());
    client.publish("/casa/AirQ/sensors/MQ2_SMOKE", String(mq2_smoke).c_str());
    client.publish("/casa/AirQ/sensors/MQ2_HYDROGEN", String(mq2_hydrogen).c_str());
    client.publish("/casa/AirQ/sensors/MQ3_ALCOHOL_MGL", String(mq3_alcohol_mgl).c_str());
    client.publish("/casa/AirQ/sensors/MQ3_ALCOHOL_PPM", String(mq3_alcohol_ppm).c_str());
    client.publish("/casa/AirQ/sensors/MQ135_CO2",String(mq135_co2).c_str());
    client.publish("/casa/AirQ/sensors/MQ8_HYDROGEN", String(mq8_hydrogen).c_str());
    client.publish("/casa/AirQ/sensors/MQ4_METHANE", String(mq4_methane).c_str());
    client.publish("/casa/AirQ/sensors/MQ6_LPG", String(mq6_lpg).c_str());
    client.publish("/casa/AirQ/sensors/MQ7_CO",String(mq7_co).c_str());
  }
}

void reconnect() {
  int retries = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    retries++;
    Serial.print("MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("/casa/AirQ/ctrl");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      if (retries>2) {
        retries = 0;
        Serial.println("Resetting ESP");
        ESP.restart(); //ESP.reset();     // Send the IP address of the ESP8266 to the computer
      }
    }
  }
}

void setup() {
  Serial.begin(9600);       // Start the Serial communication to send messages to the computer

  delay(10);
  Serial.println('\n');

  WiFi.begin(ssid, password);             // Connect to the network
  //WiFi.begin("Cerber0", "C4nc3rb3r0");             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);

  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print('.');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("Ready!");

  dht.begin();
  Serial.println("DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");
  Serial.println("------------------------------------");
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
  Serial.print("delayMS = ");Serial.println(delayMS);
  delayMS = 30000;
}

void serialEvent() {
  mq2_lpg = "";
  mq2_methane = "";
  mq2_smoke = "";
  mq2_hydrogen = "";
  mq3_alcohol_mgl = "";
  mq3_alcohol_ppm = "";
  mq135_co2 = "";

  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
  Serial.print("Message received: ");
  Serial.println(inputString);

  while (!inputString.equals("")) {
    mq2_lpg = inputString.substring(0, inputString.indexOf(','));
    Serial.print("mq2_lpg: ");Serial.println(mq2_lpg);
    inputString = inputString.substring(mq2_lpg.length() + 1);
    int i = 1;
    while (i < 12 )
    {
      i++;

      switch (i) {
        case 2:
          mq2_methane = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq2_methane.length() + 1);
          Serial.print("MQ2_Methane: ");Serial.println(mq2_methane);
          break;
        case 3:
          mq2_smoke = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq2_smoke.length() + 1);
          Serial.print("MQ2_Smoke: ");Serial.println(mq2_smoke);
          break;
        case 4:
          mq2_hydrogen = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq2_hydrogen.length() + 1);
          Serial.print("MQ2_Hydrogen: ");Serial.println(mq2_hydrogen);
          break;
        case 5:
          mq3_alcohol_mgl = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq3_alcohol_mgl.length() + 1);
          Serial.print("MQ3_Alcohol mgl: ");Serial.println(mq3_alcohol_mgl);
          break;
        case 6:
          mq3_alcohol_ppm = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq3_alcohol_ppm.length() + 1);
          Serial.print("MQ3_Alcohol ppm: ");Serial.println(mq3_alcohol_ppm);
          break;
        case 7:
          mq135_co2 = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq135_co2.length() + 1);
          Serial.print("MQ135_CO2: ");Serial.println(mq135_co2);
          break;
        case 8:
          mq8_hydrogen = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq8_hydrogen.length() + 1);
          Serial.print("MQ8_Hydrogen: ");Serial.println(mq8_hydrogen);
          break;
        case 9:
          mq4_methane = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq4_methane.length() + 1);
          Serial.print("MQ4_Methane mgl: ");Serial.println(mq4_methane);
          break;
        case 10:
          mq6_lpg = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq6_lpg.length() + 1);
          Serial.print("MQ6_LPG ppm: ");Serial.println(mq6_lpg);
          break;
        case 11:
          mq7_co = inputString.substring(0, inputString.indexOf('\n'));
          inputString = inputString.substring(mq7_co.length() + 1);
          Serial.print("MQ7_CO: ");Serial.println(mq7_co);
          break;
      }
    }
    Serial.print("MSG: ");Serial.println(inputString);
      client.publish("/casa/AirQ/sensors/MQ2_LPG", String(mq2_lpg).c_str());
      client.publish("/casa/AirQ/sensors/MQ2_METHANE", String(mq2_methane).c_str());
      client.publish("/casa/AirQ/sensors/MQ2_SMOKE", String(mq2_smoke).c_str());
      client.publish("/casa/AirQ/sensors/MQ2_HYDROGEN", String(mq2_hydrogen).c_str());
      client.publish("/casa/AirQ/sensors/MQ3_ALCOHOL_MGL", String(mq3_alcohol_mgl).c_str());
      client.publish("/casa/AirQ/sensors/MQ3_ALCOHOL_PPM", String(mq3_alcohol_ppm).c_str());
      client.publish("/casa/AirQ/sensors/MQ135_CO2",String(mq135_co2).c_str());
      client.publish("/casa/AirQ/sensors/MQ8_HYDROGEN", String(mq8_hydrogen).c_str());
      client.publish("/casa/AirQ/sensors/MQ4_METHANE", String(mq4_methane).c_str());
      client.publish("/casa/AirQ/sensors/MQ6_LPG", String(mq6_lpg).c_str());
      client.publish("/casa/AirQ/sensors/MQ7_CO",String(mq7_co).c_str());
  }
}

void loop() {
  if (!client.connected()) {
  reconnect();
  }
//  UVix.getUV();
//  char c_uvIndex[8]; // Buffer big enough for 7-character float
//  dtostrf(uvIndex, 8, 2, c_uvIndex); // Leave room for too large numbers!
//  client.publish("/casa/AirQ/sensors/UV", c_uvIndex);

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    client.publish("/casa/AirQ/sensors/temperatureDHT22", String(event.temperature).c_str());
    Serial.print("Temperature: ");
    Serial.print(event.temperature);
    Serial.println(" *C");
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    client.publish("/casa/AirQ/sensors/humidity", String(event.relative_humidity).c_str());
    Serial.print("Humidity: ");
    Serial.print(event.relative_humidity);
    Serial.println("%");
  }

  UVix.getUV();
  char c_uvIndex[8]; // Buffer big enough for 7-character float
  dtostrf(uvIndex, 8, 2, c_uvIndex); // Leave room for too large numbers!
  Serial.print("UV ix : ");
  Serial.println(c_uvIndex);
  client.publish("/casa/AirQ/sensors/UV", String(c_uvIndex).c_str());

  serialEvent();
  delay(delayMS);
  client.loop();
}
