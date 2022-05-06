#include "Platform.h"

#include <Arduino.h>

uint32_t getChipId() {
  #ifdef ESP32
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    return mac[5] << 16 | mac[4] << 8 | mac[3];
  #endif
  #ifdef ESP8266
    return ESP.getChipId();
  #endif
}
