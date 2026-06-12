#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// lgfx_device.hpp — LovyanGFX device for the Adafruit 2.8" TFT Shield (PID 1947)
// on the Metro ESP32-S3. DISPLAY ONLY (SPI).
//
//   Display : ILI9341, 240x320, hardware SPI on the ICSP header
//             SCK=39  MOSI=42  MISO=21  DC=9  CS=10  RST=tied to board reset
//
// Touch (FT6206) is intentionally NOT managed here: to keep the single shared
// I2C bus owned by exactly one driver (Arduino Wire, used by the HDC3022 and
// MAX17048 sensors), LovyanGFX does not touch I2C. The FT6206 hardware is
// validated and can be re-added later via Adafruit_FT6206 on Wire if needed.
//
// Pins come from board_pins.h (Phase-0 confirmed).
// ─────────────────────────────────────────────────────────────────────────────
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "board_pins.h"

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel;
  lgfx::Bus_SPI       _bus;

 public:
  LGFX() {
    // --- SPI bus ---
    {
      auto cfg = _bus.config();
      cfg.spi_host    = SPI2_HOST;        // ESP32-S3 FSPI; GPIO matrix routes any pins
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;         // ILI9341 handles 40 MHz
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = PIN_SPI_SCK;      // 39
      cfg.pin_mosi    = PIN_SPI_MOSI;     // 42
      cfg.pin_miso    = PIN_SPI_MISO;     // 21
      cfg.pin_dc      = PIN_TFT_DC;       // 9
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    // --- Panel ---
    {
      auto cfg = _panel.config();
      cfg.pin_cs           = PIN_TFT_CS;  // 10
      cfg.pin_rst          = -1;          // shield reset tied to board RESET
      cfg.pin_busy         = -1;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;
      _panel.config(cfg);
    }

    setPanel(&_panel);
  }
};
