/**
 * @file CO2.cpp
 * @brief Shamelessly copied from AirGradient source code
 * 
 * @license MIT
 */

#include "CO2.h"
#include "Arduino.h"
#include <SoftwareSerial.h>

void CO2::Init(int rx_pin, int tx_pin, int baudRate) {
  logger.log("Initializing CO2...");
  _SoftSerial_CO2 = new SoftwareSerial(rx_pin, tx_pin);
  _SoftSerial_CO2->begin(baudRate);

  if(getCO2_Raw() == -1) {
    logger.log("CO2 Sensor Failed to Initialize ");
  }
  else{
    logger.log("CO2 Successfully Initialized. Heating up for 10s");
    delay(10000);
  }
}

String CO2::read() {
  int retryLimit = 5;
  int ctr = 0;
  int result_CO2 = getCO2_Raw();
  while(result_CO2 == -1){
    result_CO2 = getCO2_Raw();
    if((ctr >= retryLimit) || (result_CO2 == -1)){
      Char_CO2[0] = 'N';
      Char_CO2[1] = 'U';
      Char_CO2[2] = 'L';
      Char_CO2[3] = 'L';
      Char_CO2[4] = 0;
      return "NULL";
    }
    ctr++;
  }
  sprintf(Char_CO2, "%d", result_CO2);
  return String(Char_CO2);
}

int CO2::getCO2_Raw(){
  int retry = 0;
    CO2_READ_RESULT result;
    const byte CO2Command[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};
    byte CO2Response[] = {0,0,0,0,0,0,0};

    while(!(_SoftSerial_CO2->available())) {
        retry++;
        // keep sending request until we start to get a response
        _SoftSerial_CO2->write(CO2Command, 7);
        delay(50);
        if (retry > 10) {
            return -1;
        }
    }

    int timeout = 0; 
    
    while (_SoftSerial_CO2->available() < 7) {
        timeout++; 
        if (timeout > 10) {
            while(_SoftSerial_CO2->available())  
            _SoftSerial_CO2->read();
            break;                    
        }
        delay(50);
    }

    for (int i=0; i < 7; i++) {
        int byte = _SoftSerial_CO2->read();
        if (byte == -1) {
            result.success = false;
            return -1;
        }
        CO2Response[i] = byte;
    }  
    int valMultiplier = 1;
    int high = CO2Response[3];                      
    int low = CO2Response[4];                       
    unsigned long val = high*256 + low;  

    return val;
}

