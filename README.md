# IoT Practical Course - Team Makerspace

This documentation is for the Makerspace-project of the IoT practical course at TUM during the summer semester of 2018. The course was taught by [Vladimir Podolskiy](https://www.caps.in.tum.de/en/people/vladimir-podolskiy/).

The team members were:
* Usama Siddique
* Jonathan Rösner
* Sameh Metias
* Claas Meiners

The documentation is organized in order of dependency, with e.g. the hardware part coming first because the software part depends on it.

## Hardware

The sensorbox is based on a [Wemos D1 Mini](https://wiki.wemos.cc/products:d1:d1_mini) microcontroller, which was given to us by our project partner Matthias Friessnig from [Makerspace](http://maker-space.de/). More detailed documentation about the Wemos D1 Mini can be found on their own [documentation page](https://wiki.wemos.cc/products:d1:d1_mini#documentation). 4 sensors and one actuator are connected to this microcontroller, all of which come from the [SensorKit X40](http://sensorkit.joy-it.net/index.php?title=Hauptseite), which was given to us by our lecturer, Vladimir Podolskiy. The 4 sensors are:
* [KY-026 Flammen-Sensor Modul](http://sensorkit.joy-it.net/index.php?title=KY-026_Flammen-Sensor_Modul) This sensor gives a binary signal on a single signal pin, which is triggered when a threshold of infrared light hitting the sensor is exceeded. The threshold is defined by a potentiometer on the sensor, which can be adjusted by a screw.
* [KY-015 Kombi-Sensor Temperatur+Feuchtigkeit](http://sensorkit.joy-it.net/index.php?title=KY-015_Kombi-Sensor_Temperatur%2BFeuchtigkeit) This sensor measures temperature and humidity (percentage relative heat, %RH), and is connected via a single digital pin as well.
* [KY-018   Fotowiderstand Modul](http://sensorkit.joy-it.net/index.php?title=KY-018_Fotowiderstand_Modul) This sensor is a photo resistor used to measure light intensity. It is the only sensor connected to an analog pin in this setup.
* [KY-002 Erschütterungs-Schalter Modul](http://sensorkit.joy-it.net/index.php?title=KY-002_Ersch%C3%BCtterungs-Schalter_Modul) This sensor, similar to the flame sensor, gives a signal once a certain acceleration threshold is exceeded, although this threshold cannot be adjusted on this sensor. This works through a spring in a metal tube which touches the tube when being accelerated, closing the circuit and giving a signal through a single digital pin.

The only actuator on the box is a passive piezo buzzer ([	
KY-006   Passives Piezo-Buzzer Modul](http://sensorkit.joy-it.net/index.php?title=KY-006_Passives_Piezo-Buzzer_Modul)). Passive in this context means that it beeps whenever power is supplied, and as such it only has two cables, signal and ground, as opposed to an active piezo buzzer which is always connected to voltage and ground and beeps when a signal is given via a third cable.

The fritzing model of the exact wiring required to run the latest version of our code looks like this:

![Complete wiring](https://preview.ibb.co/kBE3Tz/sensorbox_bb.png)

The assembled box looks like this. Note the black reset-button, which was put on the outside for convenience. Also, all sensors that require access to the environment (temperature/humidity, flame and light), as well as the piezo buzzer where placed facing outwards through a hole.

![The whole box](https://preview.ibb.co/hhy2MK/box.png)

## Software

We use Arduino to code the ESP8266. The full Arduino documentation can be found [here](https://www.arduino.cc/en/main/documentation).

In order to use the Arduino IDE with any kind of micro-controllers, it has to be configured first through the IDE's boards manager. For our use case we used the ['Arduino Core ESP8266'](https://wiki.wemos.cc/tutorials:get_started:get_started_in_arduino) board package by Wemos that included the Wemos D1 Mini. The standard usage of the Arduino IDE is to use a mini USB cable that is plugged into the microcontroller in order to flash code. That way the device would be visible in the available ports for flashing. However, since in our use case the board is packed into a closed box, it was impractical to use a cable for every new flash. This is why we used [Arduino Over-The-Air (OTA)](https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html) updates which for our convenience flashes code over Wi-Fi. The process works as follows:
1. Embed the logical code for sensor data collection into the basic OTA template that essentially implements the callbacks needed when a Wi-Fi flash is detected. It also keeps checking for updates in a polling fashion. (Refer to the BasicOTA example under File>Examples in an Arduino IDE)
2. Flash the code for the first time via a USB cable to the ESP. This will deploy an OTA service on the ESP.
3. Now the IDE can detect the OTA service via multicast DNS and the IP-address of the ESP is visible in the ports section and can be chosen for flashing.

**Note:** *Every* subsequent flash has to adapt the BasicOTA template to listen to future Wi-Fi updates. Also note that the ESP has to be physically reset via the hardware button after each update, which is why our box had a reset button on the outside.

### General Code Structure

Like all Arduino sketches, the code is structured in two main functions and multiple secondary functions.
The two main functions are:

1. `void setup()` is the first function that is called after global variables are instantiated, it is used to bring the SBC into the initial state. Here we connect to Wi-Fi, enable OTA flashing, prepare NTP and set pin modes.
2. `void loop()` is called after `setup()` has finished and is repeated consecutively. Any reading, writing and sending of data is done in here. Add a delay at the end of the method to reduce the amount of loops per second. Here we also check for OTA flashing updates and try to reconnect Wi-Fi and MQTT client in case of disconnect.

Additionally, we structured the code into methods for reading and sending sensor data:

1. Reading a sensor is done by calling its corresponding read method, e.g. to read the flame sensor you would call `int readFlame()` which returns the read measurement. Because sensor pins are global variables one does not have to pass this information to the function.
2. Sending the measurement is done by calling the correct send method, e.g. for data of type `int` you would call `void sendIntData(String sensor_id, int value)` with the correct id and value as parameters. Note that because the protocol is also defined as a global variable one does not have to specify this again.


### Global Variables

Because the memory space is really limited on the ESP8266 (4MB) we work heavily with global variables. Nearly no other variables are defined in the process to prevent memory leaking. For example, all readings are stored in global variables and are therefore overwriting upon the next iteration. Following is a list of the most important global variables.

<table>
  <tr>
    <th>Name</th>
    <th>Type</th>
    <th>Defaul Value</th>
    <th>Required</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>TOKEN</td>
    <td>char[]</td>
    <td>N/A</td>
    <td>Yes</td>
    <td>The JWT created by the IoT-core to grant platform access</td>
  </tr>
  <tr>
    <td>DEVICE_ID</td>
    <td>char[]</td>
    <td>N/A</td>
    <td>Yes</td>
    <td>The device id is used to identify the SBC at the IoT-core platform</td>
  </tr>
  <tr>
    <td>PROTOCOL</td>
    <td>char[]</td>
    <td>MQTT</td>
    <td>Yes</td>
    <td>Can be either "MQTT" or "HTTP". Determines which protocol to use to transmit the collected sensor data</td>
  </tr>
  <tr>
    <td>wifi_ssid</td>
    <td>char[]</td>
    <td>N/A</td>
    <td>Yes</td>
    <td>The SSID of the desired Wi-Fi network</td>
  </tr>
  <tr>
    <td>wifi_password</td>
    <td>char[]</td>
    <td>N/A</td>
    <td>Yes</td>
    <td>The password of the desired Wi-Fi network</td>
  </tr>
</table>

### Required Libraries

We use different third party libraries to get all the functionality we need.

[ESP8266WiFi.h](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFi.h)
- Used for all features regarding WIFI access for the ESP8266


[WiFiUdp.h](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiUdp.h)
- Enables UDP connections, which we use for NTP


[NTPClient.h](https://github.com/arduino-libraries/NTPClient)
- Network Time Protocol enables fetching accurate UNIX timestamps from the web


[PubSubClient.h](https://github.com/knolleary/pubsubclient)
- Provides a client for simple publish/subscribe messaging, which we use for MQTT


[ESP8266HTTPClient.h](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient)
- Standard library for making HTTP calls using the ESP8266

[DHT.h](https://github.com/adafruit/DHT-sensor-library/)
- Unified Sensor Library used to read Humidity & Temperature

[ArduinoOTA.h](https://github.com/esp8266/Arduino/blob/master/libraries/ArduinoOTA/ArduinoOTA.h)
- Used to enable OTA flashing of the ESP8266

[ArduinoJson.h](https://github.com/bblanchon/ArduinoJson)
- Used to encode data as JSON

### Using a new Sensor

Use the <b>Sensor</b> struct to define a new sensor. It has two members:

1. <b>id</b> is the sensor id created at the IoT-core
2. <b>pin</b> is the corresponding input or output pin of the ESP8266

Before using a sensor the mode of the corresponding pin has to be specified in the `setup` method.
Each sensor has its own method for reading input data or writing output data. How to actually read the specific sensor can be looked up at the manufacturer's website. Make sure to use a global variable for the reading of the data to save memory space. Use this variable as parameter for the `send` method to send the data using the desired protocol.

For further help see the <b>comments in the code</b> itself.

## Kibana

Kibana is the user interface for the Elastic stack. Kibana integrates with Elasticsearch to visualize the results of searches and allow the creation of charts and dashboards based on the searches. The searches, charts, and dashboard can visualize real‐time or historical log records. Users access Kibana through an http URL from a client web browser. 

### Kibana Discover page:

Using a web browser, either directly from a client or using the VNC client, access Kibana using the public IP address of the IOT core server (```http://<publicIPaddress>:5601```):

 http://iot.pcxd.me:5601/
 
 No user/password credentials are required.
 
 Firstly, an index pattern has to be configured in order to access the data. The index pattern for the sensors data is `<device_id>_<sensor_id>`; for example, if your device_id is `2` and sensor_id is `3` your index will be `2_3`. All of the processed data for this sensor id stored in this index. 
 
 Kibana defaults to a search of all records (*) in the last 15 minutes and returns the results: More complex searches are possible by changing the time ﬁlter and supplying search queries. To change the time ranges, click the clock symbol in the top‐right of the Kibana Discover page.


### Kibana Visualization page:

 Kibana enables near real-time analysis and visualization of streaming data. Allows interactive data exploration and supports cross-filtering. Having multiple chart types such as | Page bar chart, line and scatter plots, histograms, pie charts, maps. The following visualizations are used for our project:
 
 
 
 <table>
  <tr>
    <th>Visualization</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>Line Chart </td>
    <td>This Chart’s Y axis is the metrics axis.</td>
    
  </tr>
  <tr>
    <td>Vertical Bar Chart</td>
    <td>This Chart’s Y axis is the metrics axis</td>
    
  </tr>
 

</table>



### Kibana Dashboard

 After the creation of various visualizations, a dashboard is created. You can arrange various visualizations into one single dashboard. The dashboard can be configured to a dark theme or a Light theme. 
 
 The dashboard consists of a vertical Bar chart and multiple line chart’s. The vertical Bar chart represents the output of the Flame sensor. It will show a bar when flame sensor outputs 1 upon detecting a flame. The line charts represent the output readings of Temperature, Humidity, Vibration and Light sensors. The Y axis represents the output of the sensor while X axis denotes the timestamp in descending order. Thus, a temporal distribution of the sensors readings is visualized. 
 
 We have used the dashboard’s auto refresh feature which refreshes the dashboard after every 5 seconds thus allowing us to see Live data with a delay up to max 5 secs. The time interval can be changed from the dashboard top right corner.