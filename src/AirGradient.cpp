/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT30/31 (Temperature/Humidity Sensor)

MIT License
*/

#define hasPM true
#define hasCO2 true
#define hasSHT true

#define hasDisplay true
#define SCREEN_W 64
#define SCREEN_H 48
#define SCREEN_GEOMETRY GEOMETRY_64_48
#define HALF_W SCREEN_W/2
#define HALF_H SCREEN_H/2
#define updateInterval 5000000

#include <AirGradient.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "SSD1306Wire.h"
#include <Wire.h>

void connectToWiFi();
void connectToMQTT();
void homeassistantAutoDiscovery();
void updateState();
void showTextRectangle(String ln1, String ln2, boolean small);
void showLoading();

boolean connectWiFi = true;
boolean connectMQTT = true;

// Default MQTT settings
String discoveryPrefix = "homeassistant";
String nodeName = "air_gradient";
String mqttServer = "raspberrypi.local";
int mqttPort = 1883;
String mqttUser = "";
String mqttPassword = "";

// Global state
String discoveryTopic = "none";
String stateTopic = "none";
String commandTopic = "none";
int slide = 0;
AirGradient ag = AirGradient();
SSD1306Wire display(0x3c, SDA, SCL, SCREEN_GEOMETRY);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

struct SensorCfg {
  String sensorName;
  String valuePath;
  String unit;
  //  ['aqi', 'battery', 'carbon_dioxide', 'carbon_monoxide', 'current', 'energy', 'gas', 'humidity', 'illuminance', 'monetary', 'nitrogen_dioxide', 'nitrogen_monoxide', 'nitrous_oxide', 'ozone', 'pm1', 'pm10', 'pm25', 'power', 'power_factor', 'pressure', 'signal_strength', 'sulphur_dioxide', 'temperature', 'timestamp', 'volatile_organic_compounds', 'voltage']
  String deviceClass;
};

const SensorCfg sensors[] = {
  {"WiFi Signal Strength", ".wifi", "RSSI", "signal_strength"},
  {"CO2", ".rco2", "ppm", "carbon_dioxide"},
  {"PM2", ".pm02", "µg/m³", "pm25"},
  {"Temperature", ".atmp", "°C", "temperature"},
  {"Humidity", ".rhum", "%", "humidity"}
};

void setup(){
  Serial.begin(115200);

  display.init();
  display.flipScreenVertically();
  // showTextRectangle("Device ID", String(ESP.getChipId(), HEX), true);
  showLoading();

  #if hasPM
  ag.PMS_Init();
  #endif

  #if hasCO2
  ag.CO2_Init();
  #endif

  #if hasSHT
  ag.TMP_RH_Init(0x44);
  #endif

  if (connectWiFi) connectToWiFi();
  if (connectMQTT) connectToMQTT();

  homeassistantAutoDiscovery();
}

void loop() {
  updateState();
  ++slide;
  delay(2000);
}


void updateState() {
  slide = slide % 3;

  String payload = "{\"wifi\":" + String(WiFi.RSSI()) + ",";

  #if hasPM
    Serial.print(".");
    int PM2 = ag.getPM2_Raw();
    payload = payload + "\"pm02\":" + String(PM2);
    if(slide == 0) showTextRectangle("PM2",String(PM2),false);
  #endif

  #if hasCO2
    Serial.print(".");
    if (hasPM) payload=payload+",";
    int CO2 = ag.getCO2_Raw();
    payload = payload + "\"rco2\":" + String(CO2);
    if(slide == 1) showTextRectangle("CO2",String(CO2),false);
  #endif

  #if hasSHT
    Serial.print(".");
    if (hasCO2 || hasPM) payload=payload+",";
    TMP_RH result = ag.periodicFetchData();
    payload = payload + "\"atmp\":" + String(result.t) +   ",\"rhum\":" + String(result.rh);
    if(slide == 2) showTextRectangle(String(result.t), String(result.rh)+"%",false);
  #endif

  payload = payload + "}";

  // send payload
  if (connectWiFi & connectMQTT) {
    Serial.println(".");
    if (client.connected() && WiFi.status() == WL_CONNECTED) {
      Serial.println(stateTopic + " <- " + payload);
      client.publish(stateTopic.c_str(), payload.c_str());
    }
  }
}

// Wifi Manager
void connectToWiFi(){
  WiFiManager wifiManager(Serial);

  WiFiManagerParameter param_mqtt_server("server", "mqtt server", mqttServer.c_str(), 40);
  wifiManager.addParameter(&param_mqtt_server);

  WiFiManagerParameter param_mqtt_port("port", "mqtt port", String(mqttPort).c_str(), 40);
  wifiManager.addParameter(&param_mqtt_port);

  WiFiManagerParameter param_mqtt_user("username", "mqtt user", mqttUser.c_str(), 40);
  wifiManager.addParameter(&param_mqtt_user);

  WiFiManagerParameter param_mqtt_password("password", "mqtt password", mqttPassword.c_str(), 40);
  wifiManager.addParameter(&param_mqtt_password);

  WiFiManagerParameter param_homeassistant_prefix("prefix", "autodiscovery prefix", discoveryPrefix.c_str(), 40);
  wifiManager.addParameter(&param_homeassistant_prefix);

  WiFiManagerParameter param_device_name("name", "device name", nodeName.c_str(), 40);
  wifiManager.addParameter(&param_device_name);

  String HOTSPOT = "AIRGRADIENT-" + String(ESP.getChipId(), HEX);
  wifiManager.setTimeout(120);

  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str())) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
  }

  mqttServer = param_mqtt_server.getValue();
  mqttPort = String(param_mqtt_port.getValue()).toInt();
  mqttUser = param_mqtt_user.getValue();
  mqttPassword = param_mqtt_password.getValue();
  discoveryPrefix = param_homeassistant_prefix.getValue();
  nodeName = param_device_name.getValue();
  Serial.println(String("Got Config\n") + 
    "\tmqttServer = " + mqttServer + "\n"
    "\tmqttPort = " + mqttPort + "\n"
    "\tmqttUser = " + mqttUser + "\n"
    "\tmqttPassword = " + mqttPassword + "\n"
    "\tmqttPort = " + mqttPort + "\n"
    "\tnodeName = " + nodeName
  );
}

void mqttMessageReceived(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  if (String(topic).compareTo("homeassistant/status") == 0) {
    Serial.println("HomeAssistant change detected, re-running autodiscovery");
    homeassistantAutoDiscovery();
  }

  Serial.println();
  Serial.println("-----------------------");

}

// MQTT
void connectToMQTT(){
  client.setServer(mqttServer.c_str(), mqttPort);
  client.setCallback(mqttMessageReceived);

  int i = 0;
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqttUser.c_str(), mqttPassword.c_str())) {
      Serial.println("MQTT is connected");
      return;
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      // Config is bad, disconnect and allow reconfigure.
//      WiFi.disconnect();
      if (++i > 5) ESP.restart();
      delay(2000);
    }
  }
}


//<discovery_prefix>/<component>/[<node_id>/]<object_id>/config
void homeassistantAutoDiscovery() {
  String nodeId = nodeName + "_" + String(ESP.getChipId(), HEX);
  stateTopic = discoveryPrefix + "/sensor/" + nodeId + "/state";
  commandTopic = discoveryPrefix + "/sensor/" + nodeId + "/command";


  const uint8_t num_sensors = sizeof(sensors) / sizeof(sensors[0]);
  Serial.println("Sending autodiscovery information for " + String(num_sensors) + " sensors to MQTT");
  for (uint8_t j = 0; j < num_sensors; j++ ) {
    String sensorName = sensors[j].sensorName;
    String machineSensorName = String(sensorName);
    machineSensorName.toLowerCase();
    machineSensorName.replace(" ", "_");
  
    const String objectId = nodeId + "_" + machineSensorName;
    const String valuePath = sensors[j].valuePath;
    const String unitOfMeasurement = sensors[j].unit;
    const String deviceClass = sensors[j].deviceClass;
      
    const String discoveryTopic = discoveryPrefix + "/sensor/" + objectId + "/config";
    const String discoveryMessage = "{\"dev_cla\": \""+deviceClass+"\", \"uniq_id\":\"sensor."+objectId+"\", \"name\": \""+nodeName+" "+sensorName+"\", \"stat_t\": \""+stateTopic+"\", \"unit_of_meas\": \""+ unitOfMeasurement + "\", \"val_tpl\": \"{{ value_json"+valuePath+" }}\" }";
  
    if (discoveryMessage.length() + discoveryTopic.length() + 10 > 128) {
      Serial.println("WARN: Message too large for MQTT, increasing buffer size");
      client.setBufferSize(discoveryMessage.length() + discoveryTopic.length() + 10);
    }
    
    Serial.println(discoveryTopic + " <- " + discoveryMessage);
    if (!client.publish(discoveryTopic.c_str(), discoveryMessage.c_str())) {
      Serial.println("ERROR: publishing to " + discoveryTopic + " : " + client.state());
    }  
  }

  client.subscribe(commandTopic.c_str());
  client.subscribe("homeassistant/status");
}

void showTextRectangle(String ln1, String ln2, boolean small) {
  #if hasDisplay
    display.clear();
    display.setColor(WHITE);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    if (small) {
      display.setFont(ArialMT_Plain_16);
    } else {
      display.setFont(ArialMT_Plain_24);
    }
    display.drawString(0, 0, ln1);
    display.drawString(0, HALF_H, ln2);

    if (connectWiFi || connectMQTT){ 
      display.setColor(WHITE);
      display.fillTriangle(SCREEN_W - 8, HALF_H + 8, SCREEN_W - 2, HALF_H + 8, SCREEN_W - 5, HALF_H + 12);
      if(WiFi.status() != WL_CONNECTED || !client.connected()) {
        display.setColor(BLACK);
        display.drawLine(SCREEN_W - 8, HALF_H + 8, SCREEN_W - 2, HALF_H + 12);
      }
    }

    display.display();
  #endif
}

// DISPLAY
void showLoading() {
  #if hasDisplay
    display.clear();
    // Draw top and bottom bulbs
    display.fillCircle(HALF_W, SCREEN_H/4, SCREEN_H/4);
    display.fillCircle(HALF_W, 3*(SCREEN_H/4), SCREEN_H/4);
    display.fillRect(HALF_W - 5, HALF_H - 5, 10, 10);
    display.setColor(BLACK);
    // Cut out center to form an outline
    display.fillCircle(HALF_W, SCREEN_H/4, SCREEN_H/4 - 2);
    display.fillCircle(HALF_W, 3*(SCREEN_H/4), SCREEN_H/4 - 2);
    display.fillRect(HALF_W - 4, HALF_H - 4, 8, 8);
    // Cut off top and bottom
    display.fillRect(0, 0, SCREEN_W, 8);
    display.fillRect(0, SCREEN_H - 8, SCREEN_W, 8);
    display.setColor(WHITE);
    // Fill in top and bottom outlines
    display.fillRect(HALF_W/2, 6, HALF_W, 2);
    display.fillRect(HALF_W/2, SCREEN_H - 8, HALF_W, 2);
    display.display();
    delay(1000);
  #endif
}