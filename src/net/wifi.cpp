#include <Arduino.h>
#include <WiFi.h>
#include "wifi.h"

static const char* s_ssid = nullptr;
static const char* s_pass = nullptr;
static uint32_t    s_lastRetry = 0;

// Start connecting to Wi-Fi in station mode (non-blocking — returns immediately).
// Stashes the credentials for wifi_loop() reconnects, disables modem sleep for
// lower MQTT latency, and enables the core's auto-reconnect.
void wifi_begin(const char* ssid, const char* pass) {
  s_ssid = ssid;
  s_pass = pass;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);            // keep latency low for MQTT
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, pass);
}

// True when the station is associated and has an IP.
bool wifi_connected() {
  return WiFi.status() == WL_CONNECTED;
}

// Call periodically. If the link is down, force a reconnect attempt at most once
// every 10 s (a backstop on top of the core's own auto-reconnect).
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
