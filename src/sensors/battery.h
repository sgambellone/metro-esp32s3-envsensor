#pragma once
// MAX17048 LiPo fuel gauge (onboard, I2C 0x36).

struct BatteryReading {
  float percent    = 0;    // 0..100 %
  float volts      = 0;    // cell voltage
  float chargeRate = 0;    // %/hr; >0 charging, <0 discharging
  bool  charging   = false;
  bool  valid      = false;
};

// Begin the MAX17048 on the shared Wire bus (call Wire.begin() first).
bool battery_begin();

// Read state of charge. Returns valid=false if the gauge isn't responding.
BatteryReading battery_read();
