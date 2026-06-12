#pragma once
// LVGL <-> LovyanGFX glue: PSRAM draw buffers, flush callback, touch read
// callback, and the millis() tick source. Call lvgl_port_init() once after
// lcd.init()/setRotation(), then call lv_timer_handler() regularly in loop().
#include "lgfx_device.hpp"

// Initialize LVGL and bind it to an already-initialized LGFX panel.
// Returns true on success (draw buffers allocated in PSRAM).
bool lvgl_port_init(LGFX* panel);
