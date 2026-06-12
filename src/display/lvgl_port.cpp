#include <Arduino.h>
#include <lvgl.h>
#include "lvgl_port.h"
#include "config.h"

// Partial-render draw buffers: 60 lines tall, double-buffered, in PSRAM.
// 320 * 60 * 2 bytes = 38,400 bytes each (~75 KB total) — trivial vs 8 MB PSRAM.
static constexpr int      BUF_LINES = 60;
static constexpr size_t   BUF_BYTES = (size_t)SCREEN_W * BUF_LINES * 2;  // RGB565

static LGFX*         s_lcd  = nullptr;
static lv_display_t* s_disp = nullptr;

// Push a rendered area to the panel. LVGL stores RGB565 little-endian; the SPI
// panel wants big-endian, so writePixels(..., swap=true) byte-swaps on the fly.
static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  const uint32_t w = lv_area_get_width(area);
  const uint32_t h = lv_area_get_height(area);
  s_lcd->startWrite();
  s_lcd->setAddrWindow(area->x1, area->y1, w, h);
  s_lcd->writePixels(reinterpret_cast<uint16_t*>(px_map), w * h, true);
  s_lcd->endWrite();
  lv_display_flush_ready(disp);
}

static uint32_t tick_cb() { return millis(); }

bool lvgl_port_init(LGFX* panel) {
  s_lcd = panel;

  lv_init();
  lv_tick_set_cb(tick_cb);

  s_disp = lv_display_create(SCREEN_W, SCREEN_H);
  lv_display_set_flush_cb(s_disp, flush_cb);

  uint8_t* buf1 = static_cast<uint8_t*>(ps_malloc(BUF_BYTES));
  uint8_t* buf2 = static_cast<uint8_t*>(ps_malloc(BUF_BYTES));
  if (!buf1 || !buf2) {
    Serial.println(F("lvgl_port: PSRAM draw-buffer alloc FAILED"));
    return false;
  }
  lv_display_set_buffers(s_disp, buf1, buf2, BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);
  Serial.printf("lvgl_port: %u-byte x2 draw buffers in PSRAM (free PSRAM now %u)\n",
                (unsigned)BUF_BYTES, ESP.getFreePsram());

  // Touch input device intentionally omitted — see lgfx_device.hpp. The single
  // I2C bus is owned by Arduino Wire (sensors) to avoid dual-master conflicts.

  return true;
}
