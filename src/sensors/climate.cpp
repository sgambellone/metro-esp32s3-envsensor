#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_HDC302x.h>
#include "climate.h"
#include "board_pins.h"

static Adafruit_HDC302x hdc;

bool climate_begin() {
  return hdc.begin(I2C_ADDR_HDC3022, &Wire);
}

float heatIndexF(float T, float RH) {
  if (T < 80.0f) return T;  // heat index not meaningful below ~80°F
  float HI = -42.379f + 2.04901523f * T + 10.14333127f * RH
           - 0.22475541f * T * RH - 0.00683783f * T * T - 0.05481717f * RH * RH
           + 0.00122874f * T * T * RH + 0.00085282f * T * RH * RH
           - 0.00000199f * T * T * RH * RH;
  if (RH < 13 && T >= 80 && T <= 112)
    HI -= ((13 - RH) / 4.0f) * sqrtf((17 - fabsf(T - 95.0f)) / 17.0f);
  else if (RH > 85 && T >= 80 && T <= 87)
    HI += ((RH - 85) / 10.0f) * ((87 - T) / 5.0f);
  return HI;
}

ClimateReading climate_read() {
  ClimateReading r;
  double t = 0, rh = 0;
  if (!hdc.readTemperatureHumidityOnDemand(t, rh, TRIGGERMODE_LP0)) {
    return r;  // valid stays false
  }
  r.tempC      = (float)t;
  r.humidity   = (float)rh;
  r.tempF      = r.tempC * 9.0f / 5.0f + 32.0f;
  r.feelsLikeF = heatIndexF(r.tempF, r.humidity);
  r.valid      = true;
  return r;
}
