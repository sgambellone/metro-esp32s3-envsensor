// ─────────────────────────────────────────────────────────────────────────────
// Milestone 8-9 — full firmware: climate UI + Wi-Fi + Home Assistant.
//
//   * HDC3022 temp/humidity + heat-index, MAX17048 battery
//   * Black LVGL UI (big amber hero temp)
//   * Wi-Fi (auto-reconnect) + HA MQTT auto-discovery (temp/hum/feels/battery)
//
// Cadence: UI/sensors every SENSOR_READ_INTERVAL_MS, MQTT every
// MQTT_PUBLISH_INTERVAL_MS. Single I2C owner = Arduino Wire.
//
// Copy include/secrets.h.example -> include/secrets.h and fill in credentials.
// ─────────────────────────────────────────────────────────────────────────────
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>

#include "display/lgfx_device.hpp"
#include "display/lvgl_port.h"
#include "display/ui.h"
#include "sensors/climate.h"
#include "sensors/battery.h"
#include "net/wifi.h"
#include "net/ha_mqtt.h"
#include "board_pins.h"
#include "config.h"
#include "secrets.h"

static LGFX lcd;

static ClimateReading g_climate;
static BatteryReading g_battery;

// Refresh the small on-screen status label (top-left) to reflect the current
// Wi-Fi and Home Assistant connection state. Called once per sensor cycle.
static void updateStatusLine() {
  if (!wifi_connected())       ui_set_status(LV_SYMBOL_WIFI "  connecting...");
  else if (!ha_connected())    ui_set_status(LV_SYMBOL_WIFI "  HA: connecting...");
  else                         ui_set_status(LV_SYMBOL_WIFI "  HA " LV_SYMBOL_OK);
}

// One-time bring-up, in order: serial, display + LVGL, sensors on the shared I2C
// bus, then start Wi-Fi (non-blocking) and the Home Assistant MQTT client.
// Halts hard if LVGL fails to initialize (nothing else can work without it).
void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 1500) { delay(10); }
  Serial.println(F("\n=== Metro ESP32-S3 climate display ==="));

  // Display + LVGL
  lcd.init();
  lcd.setRotation(TFT_ROTATION);
  lcd.setBrightness(255);
  if (!lvgl_port_init(&lcd)) {
    Serial.println(F("LVGL init failed — halting."));
    while (true) { delay(1000); }
  }
  ui_init();

  // Sensors on the shared I2C bus
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);
  bool climateOk = climate_begin();
  bool batteryOk = battery_begin();
  Serial.printf("climate_begin=%d  battery_begin=%d\n", climateOk, batteryOk);
  if (!climateOk) ui_set_status("HDC3022 not found");

  // Network: Wi-Fi (non-blocking) then Home Assistant MQTT
  wifi_begin(WIFI_SSID, WIFI_PASS);
  ha_begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
  updateStatusLine();
}

// Non-blocking main scheduler: pumps LVGL and the network stacks on every pass,
// reads the sensors + refreshes the UI on SENSOR_READ_INTERVAL_MS, and publishes
// to Home Assistant on the slower MQTT_PUBLISH_INTERVAL_MS. The latest readings
// are cached in g_climate/g_battery so the publish step can reuse them.
void loop() {
  lv_timer_handler();
  wifi_loop();
  ha_loop();

  // Sensor read + UI refresh
  static uint32_t lastSensor = 0;
  static bool     first      = true;
  if (first || (millis() - lastSensor >= SENSOR_READ_INTERVAL_MS)) {
    first      = false;
    lastSensor = millis();

    g_climate = climate_read();
    g_battery = battery_read();

    if (g_climate.valid)
      Serial.printf("T=%.1fF  RH=%.0f%%  feels=%.1fF\n",
                    g_climate.tempF, g_climate.humidity, g_climate.feelsLikeF);
    else
      Serial.println(F("climate read FAILED"));

    if (g_battery.valid)
      Serial.printf("Batt %.0f%%  %.2fV  %s\n",
                    g_battery.percent, g_battery.volts,
                    g_battery.charging ? "CHARGING" : "discharging");

    ui_update(g_climate, g_battery);
    updateStatusLine();
  }

  // MQTT publish (slower cadence)
  static uint32_t lastPublish = 0;
  if (ha_connected() && (millis() - lastPublish >= MQTT_PUBLISH_INTERVAL_MS)) {
    lastPublish = millis();
    ha_publish(g_climate, g_battery);
    Serial.println(F("[HA] published"));
  }

  delay(5);
}
