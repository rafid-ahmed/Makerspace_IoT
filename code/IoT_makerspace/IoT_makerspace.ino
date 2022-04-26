#define RFSENDER_ID     0b11111
#define RFSWITCH_A      0b10000
#define PIN_RFSWITCH_SEND  17

#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <MCP3008.h>
#include <SPI.h>
#include <BrennenstuhlRCS1000N.h>
#include "EmonLib.h"

//initialize mcps
MCP3008 adc1(14, 13, 12, 15); //mcp1
MCP3008 adc2(18, 23, 19, 5);  //mcp2

//initialize current sensor
EnergyMonitor emon1;

//initialize rf switch
BrennenstuhlRCS1000N rfSwitch(PIN_RFSWITCH_SEND, BrennenstuhlRCS1000N::NO_PIN);

//initialize wifi
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiClient client;

//declare flame sensor channels
int flame_sensor2 = 6;            //mcp1 channel 6
int flame_sensor3 = 5;            //mcp1 channel 5
int flame_sensor4 = 4;            //mcp1 channel 4
int flame_sensor5 = 3;            //mcp1 channel 3
int flame_sensor6 = 2;            //mcp1 channel 2
int flame_sensor7 = 1;            //mcp1 channel 1
int flame_sensor8 = 0;            //mcp1 channel 0
int flame_sensor9 = 7;            //mcp2 channel 7
int flame_sensor10 = 6;           //mcp2 channel 6
int flame_sensor11 = 5;           //mcp2 channel 5
int flame_sensor12 = 4;           //mcp2 channel 4
int flame_sensor1 = 3;            //mcp2 channel 3

//flame sensor params
int maxValue = 0;
int flag = 0;
int fs_ID;

//current sensor params
double lastPower = 10.0;

//buzzer params
int freq = 1000;
int channel = 0;
int resolution = 8;

//wifi params
const char* ssid = "userid";
const char* password = "passwd";

//MQTT params   ----need to test with makerspace----
/*const char* mqtt_server = "10.25.172.52";
WiFiClient espClient;
PubSubClient client(espClient);
*/

//elasticsearch params
const char* host_elastic = "elasticsearchIP";
const int port_elastic = 9200;

void setup()
{
  Serial.begin(9600);

  //connect wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  timeClient.begin();

  //setup buzzer
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(21, channel);
  ledcWriteTone(channel, freq);
  ledcWrite(channel, 0);

  //setup current sensor pin
  emon1.current(33, 111.1);

  //setup MQTT  ----need to test with makerspace----
  /*client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }
  */
  
  //start rf switch
  rfSwitch.begin();
  rfSwitch.setSendRepeat(10);
}

//callback function for mqtt  ----need to test with makerspace----
/*
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message recieved in topic: ");
  Serial.println(topic);
  for(int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}
*/

//reconnect mqtt  ----need to test with makerspace----
/*
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
*/

//method to send data to elastic search
void send_message_to_elastic(String hostname, String packet)
{
  //check client connection
  if (!client.connect(host_elastic, port_elastic)) {
    Serial.println("connection failed");
    return;
  }

  //send packet
  long epoch = timeClient.getEpochTime();
  client.print(
             String("PUT /sensors/data/") + epoch + " HTTP/1.1\r\n" +
                    "Host: "+ hostname + "\r\n" +
                    "Connection: close\r\n" +
                    "Accept: */*\r\n" +
                    "Content-Length: " + packet.length() + "\r\n" +
                    "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
                    packet
    );

    //check client availiability
    unsigned long timeout = millis();
    while (client.available() == 0) 
    {
      rfSwitch.loop();
      if (millis() - timeout > 5000) 
      {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
    rfSwitch.loop();
}

//generate timestamp for elasticsearch packet
String getTimeStampString() {
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);
   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);
   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);
   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);
   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);
   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);
   return yearStr + "-" + monthStr + "-" + dayStr + "T" +
        hoursStr + ":" + minuteStr + ":" + secondStr + "Z";
}

//read power
int senseWatt()
{
  double temp;
  double Irms = emon1.calcIrms(1480);       // Calculate Irms only
  double power = (Irms*230.0 - 120) / 5.0;    // Apparent power
  if(power < 0.0)
    power = 0.0;
  Serial.print("Power: ");
  Serial.println(power);

  //non-blocking delay
  const long interval = 200;
  unsigned long previousMillis = millis();
  unsigned long currentMillis = millis();
  while(true){
    if (currentMillis != millis())
      currentMillis = millis();
    rfSwitch.loop();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      break;
    }
  }

  //handle abnormal single spikes
  if(lastPower < (power / 2.0)){
    temp = lastPower;
    lastPower = power;
    power = temp;
  }
  else{
    lastPower = power;
  }
  
  return int(power);
}

void senseFlame(int sensor, bool mcp, int id)
{
  int intensity;
  if(mcp)
    intensity = (adc1.readADC(sensor)-450)*(-1);
  else
    intensity = (adc2.readADC(sensor)-450)*(-1);

  if(intensity < 0)
    intensity = 0;
    
  maxValue = max(maxValue, intensity);
  
  Serial.print("Flame sensor ");
  Serial.print(id);
  Serial.print(": ");
  Serial.println(intensity);
  
  if(flag == id && intensity < 200){
    ledcWrite(channel, 0);
    flag = 0;
    Serial.print("flag unset: ");
    Serial.println(id);
  }
    
  if (intensity > 200)
  {
    //non-blocking delay (for checking if actual fire)
    const long interval = 2000;
    unsigned long previousMillis = millis();
    unsigned long currentMillis = millis();
    while(true){
      if (currentMillis != millis())
        currentMillis = millis();
      rfSwitch.loop();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        break;
      }
    }
    
    if(mcp)
      intensity = (adc1.readADC(sensor)-450)*(-1);
    else
      intensity = (adc2.readADC(sensor)-450)*(-1);
    
    if(intensity < 0)
      intensity = 0;
    
    if(flag == 0 && intensity > 200){
      ledcWrite(channel, 255);
      flag = id;
      rfSwitch.sendSwitchOff(RFSENDER_ID, RFSWITCH_A);
      Serial.print("flag set: ");
      Serial.println(id);
      
    }
  }

  //non-blocking delay
  const long interval = 200;
  unsigned long previousMillis = millis();
  unsigned long currentMillis = millis();
  while(true){
    if (currentMillis != millis())
      currentMillis = millis();
    rfSwitch.loop();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      break;
    }
  }
}

void loop()
{
  fs_ID = 0;
  maxValue = 0;
  
  senseFlame(flame_sensor1, false, ++fs_ID);
  senseFlame(flame_sensor2, true, ++fs_ID);
  senseFlame(flame_sensor3, true, ++fs_ID);
  senseFlame(flame_sensor4, true, ++fs_ID);
  senseFlame(flame_sensor5, true, ++fs_ID);
  senseFlame(flame_sensor6, true, ++fs_ID);
  senseFlame(flame_sensor7, true, ++fs_ID);
  senseFlame(flame_sensor8, true, ++fs_ID);
  senseFlame(flame_sensor9, false, ++fs_ID);
  senseFlame(flame_sensor10, false, ++fs_ID);
  senseFlame(flame_sensor11, false, ++fs_ID);
  senseFlame(flame_sensor12, false, ++fs_ID);

  //update time
  while(!timeClient.update()) {
    timeClient.forceUpdate();
    rfSwitch.loop();
  }
  
  int power = senseWatt();
  String packet = String("{\"timeStamp\" : \"") + getTimeStampString() + "\", \"sensor\" : " + String(maxValue) + //prepare elasticsearch packet
                  ", \"power\" : " + String(power) +"}\r\n\r\n";
  send_message_to_elastic("IoT",packet);

  //send message to mqtt   ----need to test with makerspace----
  /*client.publish("test/esp32_flame", String(maxValue));
  client.publish("test/esp32_power", String(power));
  client.loop();
  */
  
  rfSwitch.loop();
}
