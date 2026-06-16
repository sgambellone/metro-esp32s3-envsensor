#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MAX1704X.h>
#include "battery.h"

static Adafruit_MAX17048 maxlipo;
static bool s_ok = false;

// Initialize the onboard MAX17048 fuel gauge on the shared Wire bus. Call
// Wire.begin() first. Caches success in s_ok so reads can fail fast if absent.
bool battery_begin() {
  s_ok = maxlipo.begin(&Wire);
  return s_ok;
}

// Read state of charge, voltage, and charge rate, and infer charging state
// (positive charge rate or a high resting voltage). Sanity-checks the values and
// returns valid=false if the gauge looks absent/wedged (e.g. 0 V or absurd %).
BatteryReading battery_read() {
  BatteryReading r;
  if (!s_ok) return r;

  r.percent    = maxlipo.cellPercent();
  r.volts      = maxlipo.cellVoltage();
  r.chargeRate = maxlipo.chargeRate();

  // Sanity-check: a wedged/absent gauge tends to read 0 V or absurd %.
  if (r.volts < 2.0f || r.percent < 0.0f || r.percent > 110.0f) {
    return r;  // valid stays false
  }

  // Infer charging from charge-rate sign (with a small deadband) or a high
  // resting voltage (USB present, pack topped up).
  r.charging = (r.chargeRate > 0.5f) || (r.volts > 4.18f);
  r.valid    = true;
  if (r.percent > 100.0f) r.percent = 100.0f;
  return r;
}
