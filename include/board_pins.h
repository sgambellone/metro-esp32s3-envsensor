#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// board_pins.h — confirmed GPIO numbers for the Adafruit Metro ESP32-S3 (PID 5500)
// with the 2.8" Cap-Touch TFT Shield (PID 1947).
//
// Phase 0 result (CLAUDE.md §3): on this board the Arduino header labels D2..D13
// map 1:1 to the ESP32-S3 GPIO numbers. Verified against the official pinout PDF:
//   docs/hardware/Adafruit-Metro-ESP32-S3/Adafruit Metro ESP32-S3 Pinout.pdf
// (header "D9" == GPIO9, header "D10" == GPIO10).
// ─────────────────────────────────────────────────────────────────────────────

// --- Display (ILI9341 over hardware SPI on the ICSP header) ---
#define PIN_TFT_DC   9    // header D9
#define PIN_TFT_CS   10   // header D10
// Default SPI bus pins — a plain SPI.begin() lands here automatically:
#define PIN_SPI_SCK  39
#define PIN_SPI_MOSI 42
#define PIN_SPI_MISO 21

// --- microSD on the shield (unused) ---
#define PIN_SD_CS    4    // header D4

// --- I2C bus (shared by all three devices) ---
#define PIN_I2C_SDA  47
#define PIN_I2C_SCL  48

// --- Onboard NeoPixel ---
#define PIN_NEOPIXEL 46

// --- I2C device addresses (one bus, no conflicts) ---
#define I2C_ADDR_MAX17048 0x36   // onboard LiPo fuel gauge
#define I2C_ADDR_FT6206   0x38   // capacitive touch on shield
#define I2C_ADDR_HDC3022  0x44   // temp/humidity (STEMMA QT)
