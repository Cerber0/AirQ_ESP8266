#include <PubSubClient.h>
#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <UVidx/UVidx.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <TroykaMQ.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SimpleTimer.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <AutoConnect.h>

typedef ESP8266WebServer WiFiWebServer;

#define PARAM_FILE      "/param.json"
#define AUX_SETTING_URI "/mqtt_setting"
#define AUX_SAVE_URI    "/mqtt_save"
#define HOME_URI    "/"

#define DHTTYPE DHT22
#define DHTPIN            14
#define RESET_PIN 7

WiFiWebServer Server;
WiFiClient espClient;
PubSubClient client(espClient);

AutoConnect Portal(Server);
AutoConnectConfig Config;

long lastMsg = 0;
char msg[50];
int value = 0;

SimpleTimer timer;
int myNum = 5;          //Number of times to call the repeatMe function
int myInterval = 30000;  //time between funciton calls in millis

bool stringComplete = false;  // whether the string is complete
//SoftwareSerial softSerial(3, 1); // RX, TX
uint32_t delayMS;

char c_uvIndex_max[8];

unsigned long   sampletime_ms1 = 3000;
String temperature, humidity;
//char* ssid     = "Cerber0";         // The SSID (name) of the Wi-Fi network you want to connect to
//const char* password = "C4nc3rb3r0";     // The password of the Wi-Fi network
String mqtt_server = "192.168.0.162";

String mq2_lpg, mq2_methane, mq2_smoke, mq2_hydrogen, mq9_co, mq9_methane, mq9_lpg, mq135_co2, mq8_hydrogen, mq6_lpg;

UVidxClass UVix;
DHT_Unified dht(DHTPIN, DHTTYPE);

// JSON definition of AutoConnectAux.
// Multiple AutoConnectAux can be defined in the JSON array.
// In this example, JSON is hard-coded to make it easier to understand
// the AutoConnectAux API. In practice, it will be an external content
// which separated from the sketch, as the mqtt_RSSI_FS example shows.
static const char AUX_mqtt_setting[] PROGMEM = R"raw(
[
  {
    "title": "MQTT Setting",
    "uri": "/mqtt_setting",
    "menu": true,
    "element": [
      {
        "name": "style",
        "type": "ACStyle",
        "value": "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}"
      },
      {
        "name": "header",
        "type": "ACText",
        "value": "<h2>MQTT broker settings</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "mqttserver",
        "type": "ACInput",
        "value": "",
        "label": "Server",
        "pattern": "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$",
        "placeholder": "MQTT broker server"
      },
      {
        "name": "channelId_sensors",
        "type": "ACInput",
        "label": "Sensors Channel ID",
        "pattern": "^(\/[a-zA-Z0-9]([a-zA-Z0-9-])*)*$"
      },
      {
        "name": "channelid_ctrl",
        "type": "ACInput",
        "label": "Control Channel ID",
        "pattern": "^(\/[a-zA-Z0-9]([a-zA-Z0-9-])*)*$"
      },
      {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "hostname",
        "type": "ACInput",
        "value": "",
        "label": "ESP host name",
        "pattern": "^([a-zA-Z0-9]([a-zA-Z0-9-])*[a-zA-Z0-9]){1,24}$"
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save&amp;Start",
        "uri": "/mqtt_save"
      },
      {
        "name": "discard",
        "type": "ACSubmit",
        "value": "Discard",
        "uri": "/_ac"
      }
    ]
  },
  {
    "title": "MQTT Setting",
    "uri": "/mqtt_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parameters",
        "type": "ACText"
      }
    ]
  }
]
)raw";

static const char PROGMEM rootHtml[] = R"(
  <!DOCTYPE html>
  <html>
  <head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
  <h2 style="align:center;color:#3366ff;">AirQ</h2>
  <p>Temperature : {{temp}} ºC</p>
  <p>Humidity : {{hum}} %</p>
  </body>
  </html>
)";

String  channelId_sensors;
String  channelId_ctrl;
String  hostName;

String s_mq2_lpg, s_mq2_methane, s_mq2_smoke, s_mq2_hydrogen, s_mq9_methane, s_mq9_co;
String s_mq135_co2, s_mq8_hydrogen, s_mq6_lpg, s_mq9_lpg, s_humidity, s_temperature, s_uv;

String getTemperature(PageArgument& args) {
  return temperature;
}

String getHumidity(PageArgument& args) {
  return humidity;
}

/*PageElement rootElm(rootHtml, {{"temp", getTemperature}, {"hum", getHumidity}});
PageBuilder root("/", {rootElm});*/

void genTopics() {
   s_mq2_lpg = channelId_sensors + "/MQ2_LPG";
   s_mq2_methane = channelId_sensors + "/MQ2_METHANE";
   s_mq2_smoke = channelId_sensors + "/MQ2_SMOKE";
   s_mq2_hydrogen = channelId_sensors + "/MQ2_HYDROGEN";
   s_mq9_co = channelId_sensors + "/MQ9_CO";
   s_mq9_methane = channelId_sensors + "/MQ9_METHANE";
   s_mq9_lpg = channelId_sensors + "/MQ9_LPG";
   s_mq135_co2 = channelId_sensors + "/MQ135_CO2";
   s_mq8_hydrogen = channelId_sensors + "/MQ8_HYDROGEN";
   //s_mq4_methane = channelId_sensors + "/MQ4_METHANE";
   s_mq6_lpg = channelId_sensors + "/MQ6_LPG";
   //s_mq7_co = channelId_sensors + "/MQ7_CO";
   s_humidity = channelId_sensors + "/humidity";
   s_temperature = channelId_sensors + "/temperatureDHT22";
   s_uv = channelId_sensors + "/UV";
}

void getParams(AutoConnectAux& aux) {
  mqtt_server = aux["mqttserver"].value;
  mqtt_server.trim();
  channelId_sensors = aux["channelId_sensors"].value;
  channelId_sensors.trim();
  channelId_ctrl = aux["channelid_ctrl"].value;
  channelId_ctrl.trim();
  hostName = aux["hostname"].value;
  hostName.trim();
}

// Load parameters saved with  saveParams from SPIFFS into the
// elements defined in /mqtt_setting JSON.
String loadParams(AutoConnectAux& aux, PageArgument& args) {
  (void)(args);
  File param = SPIFFS.open(PARAM_FILE, "r");
  if (param) {
    if (aux.loadElement(param)) {
      getParams(aux);
      Serial.println(PARAM_FILE " loaded");
    }
    else
      Serial.println(PARAM_FILE " failed to load");
    param.close();
  }
  else {
    Serial.println(PARAM_FILE " open failed");
#ifdef ARDUINO_ARCH_ESP32
    Serial.println("If you get error as 'SPIFFS: mount failed, -10025', Please modify with 'SPIFFS.begin(true)'.");
#endif
  }
  return String("");
}

// Save the value of each element entered by '/mqtt_setting' to the
// parameter file. The saveParams as below is a callback function of
// /mqtt_save. When invoking this handler, the input value of each
// element is already stored in '/mqtt_setting'.
// In Sketch, you can output to stream its elements specified by name.
String saveParams(AutoConnectAux& aux, PageArgument& args) {
  // The 'where()' function returns the AutoConnectAux that caused
  // the transition to this page.
  AutoConnectAux&   mqtt_setting = *Portal.aux(Portal.where());
  getParams(mqtt_setting);
  AutoConnectInput& mqttserver = mqtt_setting["mqttserver"].as<AutoConnectInput>();

  // The entered value is owned by AutoConnectAux of /mqtt_setting.
  // To retrieve the elements of /mqtt_setting, it is necessary to get
  // the AutoConnectAux object of /mqtt_setting.
  File param = SPIFFS.open(PARAM_FILE, "w");
  mqtt_setting.saveElement(param, { "mqttserver", "channelId_sensors", "channelid_ctrl", "hostname" });
  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText&  echo = aux["web"].as<AutoConnectText>();
  echo.value = "Server: " + mqtt_server;
  echo.value += mqttserver.isValid() ? String(" (OK)") : String(" (ERR)");
  echo.value += "<br>Channel Sensors ID: " + channelId_sensors + "<br>";
  echo.value += "<br>Channel Ctrl ID: " + channelId_ctrl + "<br>";
  echo.value += "ESP host name: " + hostName + "<br>";

  return String("");
}

/*void handleRoot() {
  ESP8266WebServer& IntServer = Portal.host();
  IntServer.send(200, "text/html", "Temperature : "+temperature);
}


void handleNotFound() {
  ESP8266WebServer& IntServer = Portal.host();
  IntServer.send(404, "text/html", "Unknown.");
}*/

void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.println((char)payload[0]);
  // Switch on the LED if an 1 was received as first character
      sensors_event_t event;

  if ((char)payload[0] == 'U') {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
      Serial.println((char)payload[i]);
    }

    UVix.getUV();
    char c_uvIndex[8]; // Buffer big enough for 7-character float
    dtostrf(uvIndex, 8, 2, c_uvIndex); // Leave room for too large numbers!
    client.publish(String(s_uv).c_str(), c_uvIndex);
  }
  else if ((char)payload[0] == 'T') {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
      Serial.println((char)payload[i]);
    }

    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println("Error reading temperature!");
    }
    else {
      temperature = String(event.temperature).c_str();
      client.publish(String(s_temperature).c_str(), temperature.c_str());
    }
  }
  else if ((char)payload[0] == 'H') {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
      Serial.println((char)payload[i]);
    }

    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println("Error reading humidity!");
    }
    else {
      humidity = String(event.relative_humidity).c_str();
      client.publish(String(s_humidity).c_str(), humidity.c_str());
    }
  }
  else if ((char)payload[0] == 'R') {
    Serial.println("Resetting ESP");
//    digitalWrite(RESET_PIN, HIGH);
    ESP.restart(); //ESP.reset();
  }
  else if ((char)payload[0] == 'G') {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
      Serial.println((char)payload[i]);
    }

    client.publish(String(s_mq2_lpg).c_str(), String(mq2_lpg).c_str());
    client.publish(String(s_mq2_methane).c_str(), String(mq2_methane).c_str());
    client.publish(String(s_mq2_smoke).c_str(), String(mq2_smoke).c_str());
    client.publish(String(s_mq2_hydrogen).c_str(), String(mq2_hydrogen).c_str());
    client.publish(String(s_mq9_co).c_str(), String(mq9_co).c_str());
    client.publish(String(s_mq9_methane).c_str(), String(mq9_methane).c_str());
    client.publish(String(s_mq9_lpg).c_str(), String(mq9_lpg).c_str());
    client.publish(String(s_mq135_co2).c_str(), String(mq135_co2).c_str());
    client.publish(String(s_mq8_hydrogen).c_str(), String(mq8_hydrogen).c_str());
    //client.publish(String(s_mq4_methane).c_str(), String(mq4_methane).c_str());
    client.publish(String(s_mq6_lpg).c_str(), String(mq6_lpg).c_str());
    //client.publish(String(s_mq7_co).c_str(),String(mq7_co).c_str());
  }
}

void reconnect() {
  int retries = 0;

  // Loop until we're reconnected
  while (!client.connected()) {
    retries++;
    Serial.print("MQTT connection...");
    // Attempt to connect

    char buffer[10]="";
    sprintf(buffer, "espAirQ_%i", ESP.getChipId());

    if (client.connect(buffer)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(String(channelId_ctrl).c_str());
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

void serialEvent() {
  mq2_lpg = "";
  mq2_methane = "";
  mq2_smoke = "";
  mq2_hydrogen = "";
  mq9_co = "";
  mq9_methane = "";
  mq9_lpg = "";
  mq135_co2 = "";
  String inputString = "";         // a String to hold incoming data

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
    while (i < 10 )
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
          mq9_co = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq9_co.length() + 1);
          Serial.print("MQ9_CO mgl: ");Serial.println(mq9_co);
          break;
        case 6:
          mq9_methane = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq9_methane.length() + 1);
          Serial.print("MQ9_Methane mgl: ");Serial.println(mq9_methane);
          break;
        case 7:
          mq9_lpg = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq9_lpg.length() + 1);
          Serial.print("MQ9 LPG ppm: ");Serial.println(mq9_lpg);
          break;
        case 8:
          mq135_co2 = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq135_co2.length() + 1);
          Serial.print("MQ135_CO2: ");Serial.println(mq135_co2);
          break;
        case 9:
          mq8_hydrogen = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq8_hydrogen.length() + 1);
          Serial.print("MQ8_Hydrogen: ");Serial.println(mq8_hydrogen);
          break;
        case 10:
        /*  mq4_methane = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq4_methane.length() + 1);
          Serial.print("MQ4_Methane mgl: ");Serial.println(mq4_methane);
          break;
        case 11:*/
          mq6_lpg = inputString.substring(0, inputString.indexOf(','));
          inputString = inputString.substring(mq6_lpg.length() + 1);
          Serial.print("MQ6_LPG ppm: ");Serial.println(mq6_lpg);
          break;
        /*case 12:
          mq7_co = inputString.substring(0, inputString.indexOf('\n'));
          inputString = inputString.substring(mq7_co.length() + 1);
          Serial.print("MQ7_CO: ");Serial.println(mq7_co);
          break;*/
      }
    }
    if (!mq6_lpg.length() == 0) {
    Serial.print("MSG: ");Serial.println(inputString);
    client.publish(String(s_mq2_lpg).c_str(), String(mq2_lpg).c_str());
    client.publish(String(s_mq2_methane).c_str(), String(mq2_methane).c_str());
    client.publish(String(s_mq2_smoke).c_str(), String(mq2_smoke).c_str());
    client.publish(String(s_mq2_hydrogen).c_str(), String(mq2_hydrogen).c_str());
    client.publish(String(s_mq9_co).c_str(), String(mq9_co).c_str());
    client.publish(String(s_mq9_methane).c_str(), String(mq9_methane).c_str());
    client.publish(String(s_mq9_lpg).c_str(), String(mq9_lpg).c_str());
    client.publish(String(s_mq135_co2).c_str(),String(mq135_co2).c_str());
    client.publish(String(s_mq8_hydrogen).c_str(), String(mq8_hydrogen).c_str());
    //client.publish(String(s_mq4_methane).c_str(), String(mq4_methane).c_str());
    client.publish(String(s_mq6_lpg).c_str(), String(mq6_lpg).c_str());
    //client.publish(String(s_mq7_co).c_str(),String(mq7_co).c_str());
  }
    }
    }

    void getSensors() {
        sensors_event_t event;
        dht.temperature().getEvent(&event);
        if (isnan(event.temperature)) {
          Serial.println("Error reading temperature!");
        }
        else {
          temperature = String(event.temperature).c_str();
          client.publish(String(s_temperature).c_str(), temperature.c_str());
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
          humidity = String(event.relative_humidity).c_str();
          client.publish(String(s_humidity).c_str(), humidity.c_str());
          Serial.print("Humidity: ");
          Serial.print(humidity);
          Serial.println("%");
        }

        UVix.getUV();
        char c_uvIndex[8]; // Buffer big enough for 7-character float
        dtostrf(uvIndex, 8, 2, c_uvIndex); // Leave room for too large numbers!
        Serial.print("UV ix : ");
        Serial.println(c_uvIndex);
        client.publish(String(s_uv).c_str(), String(c_uvIndex).c_str());
        serialEvent();
    }

    void setup() {
//      pinMode(RESET_PIN, INPUT);
      Serial.begin(9600);       // Start the Serial communication to send messages to the computer

      delay(10);
      Serial.println('\n');
      SPIFFS.begin();

      if (Portal.load(FPSTR(AUX_mqtt_setting))) {
        AutoConnectAux& mqtt_setting = *Portal.aux(AUX_SETTING_URI);
        PageArgument  args;
        loadParams(mqtt_setting, args);
        if (hostName.length()) {
          Config.hostName = hostName;
          Serial.println("hostname set to " + Config.hostName);
        }
        Config.bootUri = AC_ONBOOTURI_HOME;
        Config.title = "AirQ";
        Config.autoReconnect = true;
        Portal.config(Config);


        Portal.on(AUX_SETTING_URI, loadParams);
        Portal.on(AUX_SAVE_URI, saveParams);
      }
      else
        Serial.println("load error");
      //root.insert(Server);
      if (Portal.begin()) {
        /*ESP8266WebServer& IntServer = Portal.host();
        IntServer.on("/", handleRoot);
        Portal.onNotFound(handleNotFound);    // For only onNotFound.*/
        Serial.println("Connection established!");
        Serial.print("IP address:\t");
        Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
      }

      genTopics();
      // Port defaults to 8266
      //ArduinoOTA.setPort(8266);
      // Hostname defaults to esp8266-[ChipID]

      ArduinoOTA.setHostname(String(Config.hostName).c_str());

      // No authentication by default
      ArduinoOTA.setPassword("AirQadmin");

      // Password can be set with it's md5 value as well
      // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
      // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

      ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
        } else { // U_SPIFFS
          type = "filesystem";
        }

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      });

      ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
      });

      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      });

      ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
          Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
          Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
          Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
          Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
          Serial.println("End Failed");
        }
      });

      Serial.println("Initialize ArduinoOTA");
      ArduinoOTA.begin();

      client.setServer(mqtt_server.c_str(), 1883);
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
      timer.setInterval(myInterval, getSensors);
    }


void loop() {
  Portal.handleClient();
  if (!client.connected()) {
  reconnect();
  }

  ArduinoOTA.handle();
  timer.run();
  client.loop();
}
