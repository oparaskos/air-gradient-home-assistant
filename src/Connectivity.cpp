#include "Connectivity.h"
#include "Platform.h"
#include <QualitySensor.h>

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
WiFiClient wifiClient;
PubSubClient client(wifiClient);

/**
 * @brief Connect to the WiFi and then to the MQTT broker.
 * 
 * @param sensors the sensors to register with homeassistant
 * @param numSensors the number of sensors in the sensors array
 */
void connect(const SENSOR_CONFIG* sensors, int numSensors) {
  if (connectWiFi) connectToWiFi();
  if (connectMQTT) connectToMQTT();
  homeassistantAutoDiscovery(sensors, numSensors);
}

void retryConnectionOrReboot() {
  int i = 0, j = 0;
  while(i < 3 && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, retrying in 5 seconds, WIFI_STATUS=" + String(WiFi.status()));
    delay(5000);
    WiFi.reconnect();
    ++i;
  }

  if(j < 3 && WiFi.status() == WL_CONNECTED) {
    while(!client.connected()) {
      Serial.print("MQTT not connected, retrying in 5 seconds, MQTT_STATE" + String(client.state()));
      delay(5000);
      connectToMQTT();
      j++;
    }
  }

  if (WiFi.status() != WL_CONNECTED || !client.connected()) {
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected was the router restarted?, WIFI_STATUS=" + String(WiFi.status()));
    }
    if(!client.connected()) {
      Serial.println("Can't reach MQTT Broker, is it down?, MQTT_STATE" + String(client.state()));
    }
    Serial.println("Rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  } else {
    Serial.println("Connection re-established after " + String(i) + " WiFi attempts, and " + String(j) + " MQTT attempts.");
    Serial.println("Continuing...");
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

  String HOTSPOT = "AIRGRADIENT-" + String(getChipId(), HEX);
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
    Serial.println("HomeAssistant change detected, re-starting to reconnect autodiscovery");
    ESP.restart();
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
void homeassistantAutoDiscovery(const SENSOR_CONFIG *sensors, uint8_t num_sensors) {
  String nodeId = nodeName + "_" + String(getChipId(), HEX);
  stateTopic = discoveryPrefix + "/sensor/" + nodeId + "/state";
  commandTopic = discoveryPrefix + "/sensor/" + nodeId + "/command";


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
    const String discoveryMessage = "{\"dev_cla\": \""+deviceClass+"\", \"uniq_id\":\"sensor."+objectId+"\", \"name\": \""+nodeName+" "+sensorName+"\", \"stat_t\": \""+stateTopic+"\", \"unit_of_meas\": \""+ unitOfMeasurement + "\", \"val_tpl\": \"{{ value_json."+valuePath+" }}\" }";
  
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


void sendPayload(const String& payload) {
  if (connectWiFi & connectMQTT) {
    if (client.connected() && WiFi.status() == WL_CONNECTED) {
      Serial.println(stateTopic + " <- " + payload);
      client.publish(stateTopic.c_str(), payload.c_str());
    } else {
      Serial.println("ERROR: not connected to MQTT or WiFi");
      retryConnectionOrReboot();
    }
  } else {
    Serial.println("WiFi or MQTT not connected");
  }
}
void updateState(const SENSOR_CONFIG* sensors, int numSensors) {
  String payload = "{";
  for (int i = 0; i < numSensors; i++) {
    if (i > 0) {
      payload += ",";
    }
    String value = sensors[i].sensor.read();
    payload += "\"" + sensors[i].valuePath + "\":\"" + value + "\"";
  }
  payload += "}";
  sendPayload(payload);
}