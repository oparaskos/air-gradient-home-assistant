#ifndef Connectivity_h
#define Connectivity_h

#include <WiFiManager.h>
#include <PubSubClient.h>

#ifdef ESP8266
    #include <ESP8266WiFi.h>
#endif

#if defined(ESP32)
    #include <WiFi.h>
#endif

struct SENSOR_CONFIG;

void connectToWiFi();
void connectToMQTT();
void homeassistantAutoDiscovery(const SENSOR_CONFIG *sensors, uint8_t num_sensors);
void retryConnectionOrReboot();
void sendPayload(const String& payload);
void updateState(const SENSOR_CONFIG* sensors, int numSensors);
void connect(const SENSOR_CONFIG* sensors, int numSensors);

#endif
