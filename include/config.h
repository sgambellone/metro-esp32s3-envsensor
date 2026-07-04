#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// config.h — units, intervals, colors, and Home Assistant entity ids.
// Keep all tunables here; keep credentials in secrets.h (gitignored).
// ─────────────────────────────────────────────────────────────────────────────

// --- Units ---
// Default °F (US). Flip to 0 for °C — this is the single-line unit switch.
#define USE_FAHRENHEIT 1

// --- Home Assistant backend selection ---
// Two independent paths to HA. BLE (BTHome v2 broadcast) is the default — no
// broker, native auto-discovery, and it keeps Wi-Fi's radio off. MQTT is wired
// in but OFF by default; flip HA_ENABLE_MQTT to 1 to also publish over Wi-Fi.
// NOTE: enabling BOTH runs Wi-Fi + BLE on the one 2.4GHz radio (coexistence can
// be finicky on the S3) — enable the pair only deliberately.
#define HA_ENABLE_BLE   1
#define HA_ENABLE_MQTT  0

// --- Update cadence (milliseconds) ---
#define SENSOR_READ_INTERVAL_MS   2500    // HDC3022 + battery read + UI refresh
#define MQTT_PUBLISH_INTERVAL_MS  20000   // publish to Home Assistant (MQTT path)
#define BLE_PUBLISH_INTERVAL_MS   20000   // refresh the BTHome advertisement (BLE path)

// --- Display ---
#define TFT_ROTATION  3        // 3 = landscape (320x240), 180° from rot 1 (flipped upright for this mounting)
#define SCREEN_W      320
#define SCREEN_H      240

// --- UI colors (RGB hex) ---
#define COLOR_BG          0x000000   // pure black background
#define COLOR_TEMP        0xFFB000   // warm amber — hero temperature
#define COLOR_SECONDARY   0xB0B0B0   // light gray — humidity / feels-like
#define COLOR_BATT_OK     0x33CC33   // green  — charging / healthy
#define COLOR_BATT_LOW    0xFFAA00   // amber  — low
#define COLOR_BATT_CRIT   0xFF3333   // red    — critical

// Battery thresholds (% state of charge)
#define BATT_LOW_PCT   30
#define BATT_CRIT_PCT  15

// --- Home Assistant device / entity identity (MQTT path) ---
#define HA_DEVICE_ID    "metro-climate-01"
#define HA_DEVICE_NAME  "Room Climate"
#define HA_ENTITY_TEMP   "room_temp"
#define HA_ENTITY_HUM    "room_humidity"
#define HA_ENTITY_FEELS  "room_feelslike"
#define HA_ENTITY_BATT   "room_battery"

// --- BLE / BTHome v2 (broadcast path) ---
// HA keys the device off its MAC + service data; the local name just makes it
// easy to spot in Settings -> Devices & Services -> BTHome.
#define BLE_LOCAL_NAME  "room-climate-01"

// Unit string shown in HA + on screen
#if USE_FAHRENHEIT
  #define TEMP_UNIT_STR "°F"
#else
  #define TEMP_UNIT_STR "°C"
#endif
