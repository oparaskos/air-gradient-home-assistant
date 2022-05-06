/*
Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT3x (Temperature/Humidity Sensor)

MIT License
*/
#include <Wire.h>
#include "Platform.h"
#include "Connectivity.h"
#include "SHTSensor.h"
#include <CO2.h>
#include <PMS.h>
#include <QualitySensor.h>
#include "WiFiSensor.h"

void updateState();

SHTSensor sht;

CO2 co2;
PMS pms;
WiFiSensor wifiSensor;
TemperatureSensor temp(sht);
HumiditySensor rhum(sht);

const SENSOR_CONFIG sensors[] = {
  {"WiFi Signal Strength", "wifi", "RSSI", "signal_strength", wifiSensor},
  {"CO2", "rco2", "ppm", "carbon_dioxide", co2},
  {"PM2", "pm02", "µg/m³", "pm25", pms},
  {"Temperature", "atmp", "°C", "temperature", temp},
  {"Humidity", "rhum", "%", "humidity", rhum}
};

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pms.Init();
  co2.Init();
  sht.init();
  
  connect(sensors, sizeof(sensors) / sizeof(SENSOR_CONFIG));
}

void loop() {
  updateState(sensors, sizeof(sensors) / sizeof(SENSOR_CONFIG));
  delay(2000);
}
