#pragma once
// HDC3022 temperature/humidity read + NOAA heat-index ("feels like").
#include <stdint.h>

struct ClimateReading {
  float tempC      = 0;
  float tempF      = 0;
  float humidity   = 0;   // %RH
  float feelsLikeF = 0;   // NOAA heat index in °F
  bool  valid      = false;
};

// Begin the HDC3022 on the shared Wire bus (call Wire.begin() first).
bool climate_begin();

// Trigger an on-demand read. Returns a reading with valid=false on I2C failure.
ClimateReading climate_read();

// NOAA heat index. T in °F, RH in %. Returns feels-like in °F.
float heatIndexF(float T, float RH);
