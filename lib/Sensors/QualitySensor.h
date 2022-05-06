/**
 * @file QualitySensor.h
 * @license MIT
 */

#ifndef QualitySensor_h
#define QualitySensor_h

#include <Arduino.h>

struct SENSOR_CONFIG {
  String sensorName;
  String valuePath;
  String unit;
  //  ['aqi', 'battery', 'carbon_dioxide', 'carbon_monoxide', 'current', 'energy', 'gas', 'humidity', 'illuminance', 'monetary', 'nitrogen_dioxide', 'nitrogen_monoxide', 'nitrous_oxide', 'ozone', 'pm1', 'pm10', 'pm25', 'power', 'power_factor', 'pressure', 'signal_strength', 'sulphur_dioxide', 'temperature', 'timestamp', 'volatile_organic_compounds', 'voltage']
  String deviceClass;
  class QualitySensor &sensor;
};

struct Logger {
  void (*log)(String);
};

void DefaultLog(String message);

const Logger defaultLogger = {
  .log = &DefaultLog
};

class QualitySensor {
public:
  virtual String read() = 0;

  void setLogger(Logger logger) { this->logger = logger; };
  Logger logger = defaultLogger;
};


class TemperatureSensor : public QualitySensor {
public:
  TemperatureSensor(class SHTSensor& sht);
  String read() override;
private:
  class SHTSensor& sht;
};

class HumiditySensor : public QualitySensor {
public:
  HumiditySensor(class SHTSensor& sht);
  String read() override;
private:
  class SHTSensor& sht;
};

#endif
