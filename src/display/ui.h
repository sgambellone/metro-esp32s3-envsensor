#pragma once
// Climate display UI: black theme, big temperature hero, humidity / feels-like /
// battery, plus a small status line for Wi-Fi/MQTT state.
#include "../sensors/climate.h"
#include "../sensors/battery.h"

void ui_init();
void ui_update(const ClimateReading& c, const BatteryReading& b);
void ui_set_status(const char* msg);
