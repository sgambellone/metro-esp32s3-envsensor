#include <Arduino.h>
#include <WiFi.h>
#include "wifi.h"

static const char* s_ssid = nullptr;
static const char* s_pass = nullptr;
static uint32_t    s_lastRetry = 0;

void wifi_begin(const char* ssid, const char* pass) {
  s_ssid = ssid;
  s_pass = pass;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);            // keep latency low for MQTT
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, pass);
}

bool wifi_connected() {
  return WiFi.status() == WL_CONNECTED;
}

void wifi_loop() {
  if (WiFi.status() == WL_CONNECTED) return;
  // Belt-and-suspenders on top of setAutoReconnect: force a retry every 10 s.
  uint32_t now = millis();
  if (now - s_lastRetry > 10000) {
    s_lastRetry = now;
    WiFi.disconnect();
    WiFi.begin(s_ssid, s_pass);
  }
}
