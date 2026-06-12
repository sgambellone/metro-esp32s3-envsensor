#include <Arduino.h>
#include <lvgl.h>
#include "ui.h"
#include "config.h"

// Custom ~110px hero font (digits, '.', '-') generated with lv_font_conv from
// Montserrat-Medium — see src/display/font_temp_110.c.
LV_FONT_DECLARE(font_temp_110);

// UTF-8 degree sign as explicit bytes (encoding-independent); split from the
// trailing unit letter so the hex escape can't swallow it.
#if USE_FAHRENHEIT
  #define UNIT_LETTER "F"
#else
  #define UNIT_LETTER "C"
#endif
#define DEG      "\xC2\xB0"
#define DEG_UNIT DEG UNIT_LETTER

static lv_obj_t* s_temp   = nullptr;   // big number, e.g. "83.6"
static lv_obj_t* s_unit   = nullptr;   // "°F"
static lv_obj_t* s_feels  = nullptr;
static lv_obj_t* s_hum    = nullptr;
static lv_obj_t* s_batt   = nullptr;
static lv_obj_t* s_status = nullptr;

void ui_init() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);

  // --- Hero temperature row: big number + small unit, centered together. A
  // content-sized flex row keeps the pair centered as the number width changes.
  lv_obj_t* row = lv_obj_create(scr);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 4, 0);
  // Centered vertically in the band between the top edge and the feels-like line.
  lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 40);

  s_temp = lv_label_create(row);
  lv_obj_set_style_text_color(s_temp, lv_color_hex(COLOR_TEMP), 0);
  lv_obj_set_style_text_font(s_temp, &font_temp_110, 0);
  lv_label_set_text(s_temp, "--");

  s_unit = lv_label_create(row);
  lv_obj_set_style_text_color(s_unit, lv_color_hex(COLOR_TEMP), 0);
  lv_obj_set_style_text_font(s_unit, &lv_font_montserrat_28, 0);
  lv_label_set_text(s_unit, DEG_UNIT);
  lv_obj_set_style_pad_top(s_unit, 18, 0);   // nudge unit down into a superscript spot

  // --- Feels-like (centered, below the hero) ---
  s_feels = lv_label_create(scr);
  lv_obj_set_style_text_color(s_feels, lv_color_hex(COLOR_SECONDARY), 0);
  lv_obj_set_style_text_font(s_feels, &lv_font_montserrat_28, 0);
  lv_label_set_text(s_feels, "Feels like --" DEG);
  lv_obj_align(s_feels, LV_ALIGN_CENTER, 0, 44);

  // --- Bottom row: humidity (left) + battery (right) ---
  s_hum = lv_label_create(scr);
  lv_obj_set_style_text_color(s_hum, lv_color_hex(COLOR_SECONDARY), 0);
  lv_obj_set_style_text_font(s_hum, &lv_font_montserrat_28, 0);
  lv_label_set_text(s_hum, "Hum --%");
  lv_obj_align(s_hum, LV_ALIGN_BOTTOM_LEFT, 8, -6);

  s_batt = lv_label_create(scr);
  lv_obj_set_style_text_color(s_batt, lv_color_hex(COLOR_BATT_OK), 0);
  lv_obj_set_style_text_font(s_batt, &lv_font_montserrat_28, 0);
  lv_label_set_text(s_batt, LV_SYMBOL_BATTERY_FULL " --%");
  lv_obj_align(s_batt, LV_ALIGN_BOTTOM_RIGHT, -8, -6);

  // --- Status line (top-left, small) — Wi-Fi/MQTT state later ---
  s_status = lv_label_create(scr);
  lv_obj_set_style_text_color(s_status, lv_color_hex(0x707070), 0);
  lv_obj_set_style_text_font(s_status, &lv_font_montserrat_14, 0);
  lv_label_set_text(s_status, "");
  lv_obj_align(s_status, LV_ALIGN_TOP_LEFT, 4, 2);
}

static const char* batterySymbol(float pct) {
  if (pct >= 87) return LV_SYMBOL_BATTERY_FULL;
  if (pct >= 62) return LV_SYMBOL_BATTERY_3;
  if (pct >= 37) return LV_SYMBOL_BATTERY_2;
  if (pct >= 12) return LV_SYMBOL_BATTERY_1;
  return LV_SYMBOL_BATTERY_EMPTY;
}

void ui_update(const ClimateReading& c, const BatteryReading& b) {
  if (c.valid) {
    float t = USE_FAHRENHEIT ? c.tempF : c.tempC;
    lv_label_set_text_fmt(s_temp, "%.1f", t);
    lv_label_set_text_fmt(s_hum, "Hum %.0f%%", c.humidity);
    float feels = USE_FAHRENHEIT ? c.feelsLikeF
                                 : (c.feelsLikeF - 32.0f) * 5.0f / 9.0f;
    lv_label_set_text_fmt(s_feels, "Feels like %.0f" DEG, feels);
  } else {
    lv_label_set_text(s_temp, "--");
  }

  if (b.valid) {
    uint32_t col = COLOR_BATT_OK;
    if (b.percent < BATT_CRIT_PCT)      col = COLOR_BATT_CRIT;
    else if (b.percent < BATT_LOW_PCT)  col = COLOR_BATT_LOW;
    lv_obj_set_style_text_color(s_batt, lv_color_hex(col), 0);

    if (b.charging)
      lv_label_set_text_fmt(s_batt, LV_SYMBOL_CHARGE " %.0f%%", b.percent);
    else
      lv_label_set_text_fmt(s_batt, "%s %.0f%%", batterySymbol(b.percent), b.percent);
  }
}

void ui_set_status(const char* msg) {
  if (s_status) lv_label_set_text(s_status, msg);
}
