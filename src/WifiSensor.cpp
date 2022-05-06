#include "WiFiSensor.h"
#include "Connectivity.h"

String WiFiSensor::read() {
    return String(WiFi.RSSI());
}