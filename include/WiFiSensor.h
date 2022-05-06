#ifndef WiFiSensor_h
#define WiFiSensor_h

#include <QualitySensor.h>

class WiFiSensor : public QualitySensor {
  String read() override;
};

#endif