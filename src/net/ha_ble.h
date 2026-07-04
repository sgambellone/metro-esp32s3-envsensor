#pragma once
// Home Assistant via BLE broadcast (BTHome v2), no broker required. Advertises a
// hand-rolled BTHome service-data payload (packet counter, temperature,
// humidity, battery) that HA auto-discovers under Settings -> Devices &
// Services -> BTHome. Mirrors the sibling remote-temp-esp32s3 firmware.
//
// Unlike the MQTT path this needs no Wi-Fi: it's broadcast-only, so the device
// never associates or connects. Advertising runs continuously (non-blocking, so
// LVGL keeps ticking); ble_publish() just refreshes the advertised values.
#include "../sensors/climate.h"
#include "../sensors/battery.h"

// Init NimBLE and start advertising an initial (empty-reading) payload.
void ble_begin();

// Rebuild the BTHome advertisement from the latest readings and bump the packet
// counter so HA treats it as a fresh sample. Skips if the climate read is
// invalid; battery is included only when its reading is valid.
void ble_publish(const ClimateReading& c, const BatteryReading& b);

// True once NimBLE has been initialized and advertising has started.
bool ble_active();
