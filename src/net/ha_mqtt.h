#pragma once
// Home Assistant MQTT auto-discovery via ArduinoHA. Registers four entities
// (temperature, humidity, feels-like, battery) on one device.
#include <stdint.h>
#include "../sensors/climate.h"
#include "../sensors/battery.h"

// Configure entities and start the MQTT client (auto-connects/reconnects).
void ha_begin(const char* host, uint16_t port, const char* user, const char* pass);

// Pump the MQTT client; call frequently from loop().
void ha_loop();

// Publish the latest readings (skips fields whose reading is invalid).
void ha_publish(const ClimateReading& c, const BatteryReading& b);

bool ha_connected();
