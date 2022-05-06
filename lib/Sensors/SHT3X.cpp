/**
 * @file SHT3X.cpp
 * @license MIT
 */

#include "QualitySensor.h"
#include "SHTSensor.h"

int i = 0, j = 0;

TemperatureSensor::TemperatureSensor(class SHTSensor& sht) : sht(sht) {}

String TemperatureSensor::read() {
    if(i > j) {
        sht.readSample();
        i++;
    }
    if(i == j) { j = i = 0; }
    return String(sht.getTemperature());
}

HumiditySensor::HumiditySensor(class SHTSensor& sht) : sht(sht) {}

String HumiditySensor::read() {
    if(j > i) {
        sht.readSample();
        j++;
    }
    if(i == j) { j = i = 0; }
    return String(sht.getHumidity());
}
