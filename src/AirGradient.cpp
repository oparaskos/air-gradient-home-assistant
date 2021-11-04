/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT30/31 (Temperature/Humidity Sensor)

MIT License
*/

#include <AirGradient.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "SSD1306Wire.h"
#include <Wire.h>

void showTextRectangle(String ln1, String ln2, boolean small);
void connectToWiFi();
void connectToMQTT();
void homeassistantAutoDiscovery();
void updateState();

// set sensors that you do not use to false
boolean hasPM = true;
boolean hasCO2 = true;
boolean hasSHT = true;
boolean hasDisplay = true;

// set to true if you want to connect to wifi.
boolean connectWiFi = true;
boolean connectMQTT = true;

String discoveryPrefix = "homeassistant";
String nodeName = "air_gradient";
String mqttServer = "raspberrypi.local";
int mqttPort = 1883;
String mqttUser = "";
String mqttPassword = "";


String discoveryTopic = "none";
String stateTopic = "none";
String commandTopic = "none";

int slide = 0;
int updateInterval = 5000000; //1s

AirGradient ag = AirGradient();
SSD1306Wire display(0x3c, SDA, SCL);
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


//void ICACHE_RAM_ATTR onTimerISR() {
//  updateState();
//  ++slide;
//  timer1_write(updateInterval);
//}

void setup(){
  Serial.begin(115200);

  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(), HEX), true);

  if (hasPM) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);

  if (connectWiFi) connectToWiFi();
  if (connectMQTT) connectToMQTT();

  homeassistantAutoDiscovery();
//  timer1_attachInterrupt(onTimerISR);
//  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
//  timer1_write(updateInterval);
}

void loop() {
  updateState();
  ++slide;
  delay(2000);
}


void updateState() {
  slide = slide % 3;

  String payload = "{\"wifi\":" + String(WiFi.RSSI()) + ",";

  if (hasPM) {
    Serial.print(".");
    int PM2 = ag.getPM2_Raw();
    payload = payload + "\"pm02\":" + String(PM2);
    if(slide == 0) showTextRectangle("PM2",String(PM2),false);
  }

  if (hasCO2) {
    Serial.print(".");
    if (hasPM) payload=payload+",";
    int CO2 = ag.getCO2_Raw();
    payload = payload + "\"rco2\":" + String(CO2);
    if(slide == 1) showTextRectangle("CO2",String(CO2),false);
  }

  if (hasSHT) {
    Serial.print(".");
    if (hasCO2 || hasPM) payload=payload+",";
    TMP_RH result = ag.periodicFetchData();
    payload = payload + "\"atmp\":" + String(result.t) +   ",\"rhum\":" + String(result.rh);
    if(slide == 2) showTextRectangle(String(result.t), String(result.rh)+"%",false);
  }

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

// DISPLAY
void showTextRectangle(String ln1, String ln2, boolean small) {
  if (hasDisplay) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    if (small) {
      display.setFont(ArialMT_Plain_16);
    } else {
      display.setFont(ArialMT_Plain_24);
    }
    display.drawString(32, 16, ln1);
    display.drawString(32, 36, ln2);
    display.display();
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
  for (int i = 0; i < length; i++) {
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


  Serial.println("Sending autodiscovery to MQTT");
  for (int j = 0; j < sizeof(sensors)/sizeof(sensors[0]); j++ ) {
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
