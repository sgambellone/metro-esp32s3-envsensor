#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoHA.h>
#include "ha_mqtt.h"
#include "config.h"

static WiFiClient wifiClient;
static HADevice    device(HA_DEVICE_ID);
static HAMqtt      mqtt(wifiClient, device);

// Four published entities. PrecisionP1 = one decimal, P0 = integer.
static HASensorNumber haTemp (HA_ENTITY_TEMP,  HASensorNumber::PrecisionP1);
static HASensorNumber haHum  (HA_ENTITY_HUM,   HASensorNumber::PrecisionP1);
static HASensorNumber haFeels(HA_ENTITY_FEELS, HASensorNumber::PrecisionP1);
static HASensorNumber haBatt (HA_ENTITY_BATT,  HASensorNumber::PrecisionP0);

// Configure the HA device + four entities (name, device class, unit) and start
// the MQTT client. ArduinoHA then auto-connects and reconnects on its own; HA
// creates the entities via MQTT discovery on first connect. A null/empty
// user/pass means an anonymous broker connection.
void ha_begin(const char* host, uint16_t port, const char* user, const char* pass) {
  device.setName(HA_DEVICE_NAME);
  device.setSoftwareVersion("1.0.0");
  device.setManufacturer("DIY");
  device.setModel("Metro ESP32-S3 Climate");
  device.enableSharedAvailability();   // one availability topic for all entities
  device.enableLastWill();             // mark offline cleanly if the device drops

  haTemp.setName("Temperature");
  haTemp.setDeviceClass("temperature");
  haTemp.setUnitOfMeasurement(TEMP_UNIT_STR);

  haHum.setName("Humidity");
  haHum.setDeviceClass("humidity");
  haHum.setUnitOfMeasurement("%");

  haFeels.setName("Feels Like");
  haFeels.setDeviceClass("temperature");
  haFeels.setUnitOfMeasurement(TEMP_UNIT_STR);

  haBatt.setName("Battery");
  haBatt.setDeviceClass("battery");
  haBatt.setUnitOfMeasurement("%");

  // username/password may be nullptr for an anonymous broker.
  const char* u = (user && user[0]) ? user : nullptr;
  const char* p = (pass && pass[0]) ? pass : nullptr;
  mqtt.begin(host, port, u, p);
}

// Pump the MQTT client (handles connect/reconnect and message I/O). Must be
// called frequently from the main loop.
void ha_loop() {
  mqtt.loop();
}

// Publish the latest readings to Home Assistant (no-op if not connected).
// Temperature/feels-like honor USE_FAHRENHEIT; invalid readings are skipped.
// ArduinoHA suppresses the MQTT message when a value is unchanged.
void ha_publish(const ClimateReading& c, const BatteryReading& b) {
  if (!mqtt.isConnected()) return;
  if (c.valid) {
    haTemp.setValue(USE_FAHRENHEIT ? c.tempF : c.tempC);
    haHum.setValue(c.humidity);
    haFeels.setValue(USE_FAHRENHEIT ? c.feelsLikeF
                                    : (c.feelsLikeF - 32.0f) * 5.0f / 9.0f);
  }
  if (b.valid) haBatt.setValue(b.percent);
}

// True once the MQTT client has an active session with the broker.
bool ha_connected() {
  return mqtt.isConnected();
}
