/**
 * @file CO2.h
 * @license MIT
 */

#ifndef CO2_h
#define CO2_h

#include <SoftwareSerial.h>
#include "QualitySensor.h"

#ifdef ESP8266
  #define PIN_CO2_RX D3
  #define PIN_CO2_TX D4
#endif
#ifdef ESP32
  #define PIN_CO2_RX 25
  #define PIN_CO2_TX 26
#endif


struct CO2_READ_RESULT {
    int co2 = -1;
    bool success = false;
};

class CO2: public QualitySensor {
public:
    void Init(int rx_pin = PIN_CO2_TX, int tx_pin = PIN_CO2_RX, int baud = 9600);
    String read() override;
    int getCO2_Raw();
    SoftwareSerial *_SoftSerial_CO2;
private:
    char Char_CO2[10];
};

#endif