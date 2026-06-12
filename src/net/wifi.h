#pragma once
// Minimal Wi-Fi station helper: non-blocking connect + auto-reconnect.
#include <stdint.h>

// Start connecting (non-blocking). Returns immediately; poll wifi_connected().
void wifi_begin(const char* ssid, const char* pass);

bool wifi_connected();

// Call periodically; nudges a reconnect if the link drops.
void wifi_loop();
