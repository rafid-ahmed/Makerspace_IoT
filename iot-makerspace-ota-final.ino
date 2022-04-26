/*-----------External Libraries-----------*/
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"

/*-----------Global variables-----------*/
#define TOKEN "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE1MzI4Nzg4NTQsImlzcyI6ImlvdHBsYXRmb3JtIiwic3ViIjoiNSJ9.O_eNIV8yCUlnAjCXAoKh9ll91zykB_iUvi9T85dx_QqBRNIl7beiTSNqfHUdaJbuFcT2X9aQQX4y5ct4P-nWISqD6LWEye18wus-MiCxT-Ks9zBOdwyhugqbihHcqH5WMY3F36361MQgIHnmRwBaCo0xVQe3TYfViaytfvLjdgMN2I4bwdplqDG4-INu-ywvPUdMVZUIQIhFcR1s0HbSjZrvjxfN8WtnsJe8se9e3A6U7dDBh9UEKoVCTR4rn8_pj0MZSFsMLkW-pgXFWz2j9tJCqDtOJYfZdLoU3pksjpcjIBUfh_lHuEHKl2y3_033EkIojKMdxwG7kgzBCLLFrL9FTnDsrOhr4anRRQezDEsbUxro9DGZZ0U0dhOJG7lOgxUGxdu_T4J7SZxoEOtUKz4RJLOesK6yQIT-1QWqqn4bRWxIa8x6LS9a2B5qRBj3XD8fNqTO1F-n4UrL1USEd4aDFHgwv6raIFN_f9PHZHjV1pZScm8alXAceNjmw7NRIJwYHSfiXLh01IZGZjQ_BI3bGLm8pmcHROBPOXgyRIY3QH66jg3a2nH7XO1FqtfV6lQMn6IxEBgMBzOywQbXVoZaY2kNGk_YOp084qPpSxRgmTp7fWCD-i_AaYNWFCPW2V4Gm7Vh3HoH-YhqUzazTN1anU-SQq0it7RoP12yrkM"
#define DEVICE_ID "5"
#define PROTOCOL "MQTT"
#define IOT_CORE_URL "http://iot.pcxd.me:8083/"

#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "europe.pool.ntp.org"

#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//define WIFI and protocol specific clients
const char wifi_ssid[] = "xxx";
const char wifi_password[] = "xxx";
WiFiClient espClient;
PubSubClient client(espClient);
const char* mqtt_server = "iot.pcxd.me";
char JSONmessageBuffer[105];
HTTPClient http;

//Enable UDP to get timestamps
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
unsigned long t;

/*-----------Sensors-----------*/
struct Sensor {
  String id;
  int pin;
};

//define all sensors
const Sensor tmpSensor = { "6", DHTPIN };
const Sensor flameSensor = { "8", 13 };
const Sensor humiditySensor = { "7", DHTPIN };
const Sensor vibrationSensor = { "10", 0 };
const Sensor lightSensor = { "9", A0 };
const Sensor buzzer = { "99", 14 };
float th_reading[2];
int flame_reading;
int vibration_reading = HIGH;
int light_reading;

/*-----------Sensor readings-----------*/
void readTempAndHumidity(float result[]) {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("Error while reading temperature and humidity sensors");
    result[0] = -1.0;
    result[1] = -1.0;
  } else {
    result[0] = t;
    result[1] = h;
  }
}

int readFlame() {
  int result = digitalRead(flameSensor.pin);
  if (result == 1) {
    for (int i = 0; i < 80; i++) {  // make a sound
      digitalWrite(buzzer.pin, HIGH); // send high signal to buzzer 
      delay(1); // delay 1ms
      digitalWrite(buzzer.pin, LOW); // send low signal to buzzer
      delay(1);
    }
    delay(50);
    for (int j = 0; j < 100; j++) { //make another sound
      digitalWrite(buzzer.pin, HIGH);
      delay(2); // delay 2ms
      digitalWrite(buzzer.pin, LOW);
      delay(2);
    }
  }
  return result;
}

int readLight() {
  return analogRead(lightSensor.pin);
}

int readVibration() {
  return digitalRead(vibrationSensor.pin);
}

/*-----------Send Data-----------*/
void sendIntData(String sensor_id, int value) {
  // 105 is from https://arduinojson.org/v5/assistant/
  StaticJsonBuffer<105> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  //-3600 to go from CET+0 to CET-1
  t = timeClient.getEpochTime() - 3600;
  JSONencoder["timestamp"] = t;
  JSONencoder["sensor_id"] = sensor_id;
  JSONencoder["value"] = value;
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  sendMessage(JSONmessageBuffer);
}

void sendFloatData(String sensor_id, float value) {
  // 105 is from https://arduinojson.org/v5/assistant/
  StaticJsonBuffer<105> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  //-3600 to go from CET+0 to CET-1
  t = timeClient.getEpochTime() - 3600;
  JSONencoder["timestamp"] = t;
  JSONencoder["sensor_id"] = sensor_id;
  JSONencoder["value"] = value;
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  sendMessage(JSONmessageBuffer);
}

void sendMessage(char message[]) {
  if(PROTOCOL == "HTTP") {
    http.begin(IOT_CORE_URL);
    http.addHeader("content-type", "application/json");
    char tokenHeader [sizeof(TOKEN) + 7];
    sprintf(tokenHeader,"Bearer %s",TOKEN);
    http.addHeader("Authorization", tokenHeader);
    int httpCode = http.POST(JSONmessageBuffer);
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
    http.end();
  } else {
    client.publish(DEVICE_ID, message);
  } 
}

void setup() {
  Serial.begin(9600);
  Serial.print("Connecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    ESP.restart();
  }
  Serial.println("!");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";
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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  if (PROTOCOL == "MQTT") {
    client.setServer(mqtt_server, 1883); 
  }
  dht.begin();

  // define the pinMode for all input/output pins
  pinMode(flameSensor.pin, INPUT);
  pinMode(lightSensor.pin, INPUT);
  pinMode(vibrationSensor.pin, INPUT);
  pinMode(buzzer.pin, OUTPUT);
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("mqttmakerspace_1", "JWT", TOKEN)) {
      Serial.println("connected");
    } else {
      Serial.print(" failed, state=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  ArduinoOTA.handle(); //look for incoming OTA
  if (PROTOCOL == "MQTT") {
    if (!client.connecteGet sensor readingsd()) {
      reconnectMQTT(); //reconnect if required
    }
    client.loop();
  }
  timeClient.update(); //update NTP timestamp
  
  if(WiFi.status()== WL_CONNECTED) { 
   //Temperature and humidity
   //th_reading[0] is tmp, th_reading[1] is humidity
   readTempAndHumidity(th_reading);
   if (th_reading[0] != -1 && th_reading[1] != -1) {
    sendFloatData(tmpSensor.id, th_reading[0]);
    sendFloatData(humiditySensor.id, th_reading[1]);
   }
   //Flame
   flame_reading = readFlame();
   sendIntData(flameSensor.id, flame_reading);
   //Vibration
   vibration_reading = readVibration();
   sendIntData(vibrationSensor.id, vibration_reading);
   //Light
   light_reading = readLight();
   sendIntData(lightSensor.id, light_reading);
  } else {
    Serial.println("Error in WiFi connection");   
  }
  delay(1000);
}
