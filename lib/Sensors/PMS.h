/**
 * @file PMS.h
 * @license MIT
 */

#ifndef PMS_h
#define PMS_h

#include <stdint.h>
#include "QualitySensor.h"
#include <SoftwareSerial.h>

#ifdef ESP8266
  #define PIN_PMS_TX D5
  #define PIN_PMS_RX D6
#endif
#ifdef ESP32
  #define PIN_PMS_TX 19
  #define PIN_PMS_RX 18
#endif

class Stream;

class PMS : public QualitySensor {
public:
    bool Init(int pin_rx = PIN_PMS_RX, int pin_tx = PIN_PMS_TX, int baud = 9600);
    static const uint16_t SINGLE_RESPONSE_TIME = 1000;
    static const uint16_t TOTAL_RESPONSE_TIME = 1000 * 10;
    static const uint16_t STEADY_RESPONSE_TIME = 1000 * 30;

    static const uint16_t BAUD_RATE = 9600;

    struct DATA {
      // Standard Particles, CF=1
      uint16_t PM_SP_UG_1_0;
      uint16_t PM_SP_UG_2_5;
      uint16_t PM_SP_UG_10_0;

      // Atmospheric environment
      uint16_t PM_AE_UG_1_0;
      uint16_t PM_AE_UG_2_5;
      uint16_t PM_AE_UG_10_0;
    };

    void setStream(Stream&);
    void sleep();
    void wakeUp();
    void activeMode();
    void passiveMode();

    void requestRead();
    bool read_PMS(DATA& data);
    bool readUntil(DATA& data, uint16_t timeout = SINGLE_RESPONSE_TIME);
    String read() override;
    int getPM2_Raw();

private:
    enum STATUS { STATUS_WAITING, STATUS_OK, STATUS_ERROR };
    enum MODE { MODE_ACTIVE, MODE_PASSIVE };

    uint8_t _payload[12];
    Stream* _stream;
    DATA* _data;
    STATUS _PMSstatus;
    MODE _mode = MODE_ACTIVE;

    uint8_t _index = 0;
    uint16_t _frameLen;
    uint16_t _checksum;
    uint16_t _calculatedChecksum;
    SoftwareSerial *_SoftSerial_PMS;
    void loop();
    char Char_PM2[10];
};

#endif