# CLAUDE.md

Project guidance for Claude Code. Read this fully before writing any code.

---

## 1. What we're building

A wall/desk **room-climate display** built on an Adafruit Metro ESP32-S3 with a 2.8"
touchscreen shield and a precision temp/humidity sensor.

The screen must show, on a **dark (black) high-contrast LVGL UI**:

- **Temperature** — in a *very large* font, readable across a room. This is the hero element.
- **Humidity** (%RH)
- **Feels Like** (computed heat index / apparent temperature)
- **Battery** monitor (% and charging state)

It must also **publish temperature and humidity to Home Assistant** for monitoring/history.

Primary success criteria: glanceable from across the room, stable 24/7, and the temp + humidity
show up as native HA entities.

---

## 2. Hardware (exact parts)

| Part | Adafruit PID | Role | Bus |
|---|---|---|---|
| Adafruit Metro ESP32-S3, 16 MB Flash / 8 MB PSRAM | 5500 | MCU board (Arduino/UNO shield form factor) | — |
| 2.8" TFT Touch Shield (Capacitive) | 1947 | 320×240 ILI9341 display + FT6206 cap-touch | SPI + I2C |
| HDC3022 Precision Temp & Humidity (STEMMA QT) | 5989 | the actual sensor | I2C |
| 3000 mAh LiPo | — | battery backup | JST to Metro |

### Key board facts (verified)
- ESP32-S3 dual-core 240 MHz, native USB-C, Wi-Fi + BLE, **8 MB octal PSRAM** (use it for LVGL buffers).
- **Onboard MAX17048 LiPo fuel gauge at I2C `0x36`** — this is how we read battery %/voltage. No ADC hack needed.
- STEMMA QT connector for I2C with **switchable power**.
- **Get a Rev B board** (anything bought after Nov 2023). Rev A had PSRAM/SPI pin conflicts that break things when PSRAM is enabled. Assume Rev B; if PSRAM misbehaves, suspect Rev A.
- PlatformIO board id: **`adafruit_metro_esp32s3`**.

### Display shield facts (verified)
- Display controller: **ILI9341**, 320×240, SPI. Has its own RAM buffer.
- Touch controller (this is the **capacitive** variant): **FT6206 at I2C `0x38`** (library `Adafruit_FT6206`). Touch IRQ line is **not** connected by default — poll it, don't rely on interrupts.
- Display uses **hardware SPI via the 2×3 ICSP header**, plus two GPIOs: **DC on Arduino header pin "D9"**, **CS on header pin "D10"**. microSD CS is on "D4" (we don't use the SD card).
- The shield level-shifts and is fine on the Metro's 3.3 V logic.

### I2C bus map (everything shares ONE bus — no address conflicts)
| Address | Device |
|---|---|
| `0x36` | MAX17048 fuel gauge (onboard) |
| `0x38` | FT6206 capacitive touch (on shield) |
| `0x44` | HDC3022 temp/humidity (STEMMA QT, default addr) |

---

## 3. ⚠️ Phase 0 — confirm the display GPIO numbers BEFORE coding the driver

On the ESP32-S3, **an Arduino header pin labeled "D9"/"D10" is NOT guaranteed to equal GPIO 9/10.**
The shield routes DC→header-D9 and CS→header-D10; we need the *actual GPIO* those header pins map to
on the Metro variant. Do **not** hardcode `9`/`10` and hope.

Get the real numbers from the installed core variant file (do this first, it's a 1-minute step):

```bash
# Path appears after the first `pio run` pulls the framework. Adjust version dir as needed.
find ~/.platformio/packages/framework-arduinoespressif32* \
  -path '*variants/adafruit_metro_esp32s3/pins_arduino.h' -exec cat {} \;
```

Look for the `D0..D13` / SPI defines. Cross-check against the Adafruit "PrettyPins" diagram on
the Metro ESP32-S3 product/learn page if anything is ambiguous.

**What we already know for certain (hardware SPI on the ICSP header / the Metro's default SPI bus):**
- `SCK  = GPIO39`
- `MOSI = GPIO42`
- `MISO = GPIO21`

Because those are the board's *default* SPI bus pins AND they're what the shield's ICSP header uses,
a library that calls plain `SPI.begin()` will land on the right pins automatically. You only need to
nail down **DC (D9)** and **CS (D10)** GPIO numbers. Record them in `include/board_pins.h` once
confirmed, and reference that header everywhere.

---

## 4. Tech stack

- **Build system:** PlatformIO, `framework = arduino`, `platform = espressif32`.
- **Graphics driver:** **LovyanGFX** (primary). It bundles ILI9341 + FT6206 in one config and integrates cleanly with LVGL (DMA-friendly).
  - **Fallback if LovyanGFX config fights you:** `Adafruit_ILI9341` + `Adafruit_FT6206` + `Adafruit_GFX`, with a thin LVGL flush/read callback. This path uses the default `SPI` bus (already = GPIO39/42/21), so you only supply CS+DC — very robust. Switch to this if LovyanGFX bring-up stalls more than ~30 min.
- **UI:** **LVGL** (target the current 9.x line). lv_conf.h MUST match the LVGL major version you pin.
- **Sensor:** `Adafruit_HDC302x`.
- **Battery:** `Adafruit_MAX1704X`.
- **Home Assistant:** MQTT with auto-discovery via **ArduinoHA** (`dawidchyrzynski/home-assistant-integration`). Requires an MQTT broker — the Mosquitto add-on in HA is the normal choice.

> Versions move. At session start, check the latest stable releases of LVGL, LovyanGFX, ArduinoHA,
> and the Adafruit libs, then **pin exact versions** in `platformio.ini`. Don't float on `^` for
> LVGL specifically — a minor bump can break lv_conf.h.

---

## 5. Starting `platformio.ini`

Treat as a starting point; fill in confirmed versions.

```ini
[env:adafruit_metro_esp32s3]
platform = espressif32
board = adafruit_metro_esp32s3
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder

; --- 16MB flash + 8MB OCTAL PSRAM ---
board_build.flash_size = 16MB
board_build.partitions = default_16MB.csv
board_build.arduino.memory_type = qio_opi   ; octal PSRAM
build_flags =
    -DBOARD_HAS_PSRAM
    -DLV_CONF_INCLUDE_SIMPLE
    -I include            ; so lv_conf.h in include/ is found
    -DLV_LVGL_H_INCLUDE_SIMPLE

lib_deps =
    lvgl/lvgl @ <pin exact, e.g. 9.2.x>
    lovyan03/LovyanGFX @ <pin exact>
    adafruit/Adafruit HDC302x @ <pin exact>
    adafruit/Adafruit MAX1704X @ <pin exact>
    adafruit/Adafruit BusIO
    dawidchyrzynski/home-assistant-integration @ <pin exact>
    bblanchon/ArduinoJson
```

Notes:
- Allocate LVGL draw buffers in **PSRAM** (`ps_malloc` / LVGL custom alloc) since you have 8 MB — don't starve internal RAM.
- If the build can't find `lv_conf.h`, the include path / `LV_CONF_*` flags are the usual culprit.

---

## 6. LVGL UI requirements

### Theme & contrast
- Screen background: **pure black** (`0x000000`).
- High contrast text. Suggested: temperature in **white or warm-amber**, secondary readings in a lighter gray. Use a small accent color for battery state (green charging / amber low / red critical).
- Use LVGL's dark theme as a base (`lv_theme_default_init(..., dark = true, ...)`) and then override per-widget styles for the black bg + big font.

### Layout (320×240 landscape)
- **Temperature dominates** the screen — center it, take ~60% of vertical space.
- Below/around it: Humidity, Feels Like, Battery in a smaller row.
- Default unit **°F** (user is in the US); make the unit a single config constant so °C is a one-line change.

### Big font for "across the room"
- LVGL's built-in Montserrat tops out at **48 px** — too small for the hero temp.
- Generate a **custom large font** (≈96–140 px) with the LVGL font converter, restricted to the glyphs you need (`0-9 . - ° C F %`) to keep flash small. Enable it in lv_conf.h and assign it to the temp label.
- Quick-start shortcut: enable `LV_FONT_MONTSERRAT_48` to get pixels on screen first, then swap in the custom big font once the pipeline works.

### Update cadence
- Refresh sensor + UI roughly every **2–5 s** (HDC3022 is fast; no need to hammer it). Call `lv_timer_handler()` frequently in `loop()`.

---

## 7. Sensors — API specifics (verified signatures)

### HDC3022 (temp/humidity)
```cpp
#include <Adafruit_HDC302x.h>
Adafruit_HDC302x hdc;            // default
// In setup(): begin returns bool
hdc.begin(0x44, &Wire);
// Read (values are double, Celsius + %RH):
double tempC = 0, rh = 0;
hdc.readTemperatureHumidityOnDemand(tempC, rh, TRIGGERMODE_LP0);
```
- Address `0x44`. Keep the white PTFE filter cover ON.

### MAX17048 (battery, onboard at 0x36)
```cpp
#include <Adafruit_MAX1704X.h>
Adafruit_MAX17048 maxlipo;
maxlipo.begin();                 // shares Wire bus
float pct  = maxlipo.cellPercent();   // 0..100
float volts = maxlipo.cellVoltage();  // V
float rate = maxlipo.chargeRate();    // %/hr; >0 ≈ charging, <0 ≈ discharging
```
- Use `chargeRate()` sign (and/or `cellVoltage()` ~>4.15 V) to infer charging state. No pack-size config needed (MAX17048 is model-based).

### I2C bring-up gotcha
- All three devices share `Wire`. Run an I2C scan in early bring-up and confirm you see `0x36`, `0x38`, `0x44`.
- **If `0x44` (HDC3022) is missing:** the STEMMA QT port has switchable power. The Adafruit core normally enables it automatically, but if the QT device doesn't appear, check the Metro variant's I2C-power-enable pin and drive it HIGH before `Wire.begin()`. (The shield's FT6206 at `0x38` is wired through the header, not the QT switch, so it can appear even when QT power is off — that's the tell.)

---

## 8. "Feels Like" calculation

Indoors there's no wind, so use the **NOAA heat index** (apparent temperature). It's only meaningful
when it's warm; below ~80 °F (26.7 °C) feels-like ≈ actual temp.

```cpp
// T in °F, RH in %. Returns feels-like in °F.
float heatIndexF(float T, float RH) {
  if (T < 80.0f) return T;  // heat index not meaningful below ~80F
  float HI = -42.379 + 2.04901523*T + 10.14333127*RH
           - 0.22475541*T*RH - 0.00683783*T*T - 0.05481717*RH*RH
           + 0.00122874*T*T*RH + 0.00085282*T*RH*RH - 0.00000199*T*T*RH*RH;
  // low-humidity / high-humidity adjustments
  if (RH < 13 && T >= 80 && T <= 112)
      HI -= ((13 - RH) / 4.0) * sqrt((17 - fabs(T - 95.0)) / 17.0);
  else if (RH > 85 && T >= 80 && T <= 87)
      HI += ((RH - 85) / 10.0) * ((87 - T) / 5.0);
  return HI;
}
```
- Convert HDC's Celsius reading to °F for this, or work in °C with the equivalent formula — just be consistent with the displayed unit.

---

## 9. Home Assistant integration (MQTT auto-discovery)

Use **ArduinoHA** so entities appear in HA automatically (no manual `configuration.yaml` editing).

Shape of it:
```cpp
#include <WiFi.h>
#include <ArduinoHA.h>

WiFiClient client;
HADevice device("metro-climate-01");          // unique id
HAMqtt mqtt(client, device);

HASensorNumber haTemp("room_temp", HASensorNumber::PrecisionP1);
HASensorNumber haHum ("room_humidity", HASensorNumber::PrecisionP1);
HASensorNumber haFeels("room_feelslike", HASensorNumber::PrecisionP1);
HASensorNumber haBatt("room_battery", HASensorNumber::PrecisionP0);

// setup(): set device class / unit so HA renders them nicely
haTemp.setDeviceClass("temperature");  haTemp.setUnitOfMeasurement("°F");
haHum.setDeviceClass("humidity");      haHum.setUnitOfMeasurement("%");
haBatt.setDeviceClass("battery");      haBatt.setUnitOfMeasurement("%");
// connect WiFi, then: mqtt.begin(MQTT_HOST, MQTT_USER, MQTT_PASS);

// loop(): mqtt.loop();  then publish on an interval:
haTemp.setValue(tempF);  haHum.setValue(rh);  haFeels.setValue(feels);  haBatt.setValue(pct);
```
- Requirements on the HA side: an MQTT broker (Mosquitto add-on) + the MQTT integration enabled. The user supplies broker host/user/pass.
- Temperature and humidity are the must-haves; feels-like and battery are nice-to-haves — wire all four.
- Decouple cadence: UI can refresh every ~2 s; MQTT publishes every ~15–30 s is plenty for HA history.

---

## 10. Project structure (suggested)

```
.
├── CLAUDE.md
├── platformio.ini
├── .gitignore                 # MUST ignore include/secrets.h
├── include/
│   ├── lv_conf.h              # matches pinned LVGL major version
│   ├── board_pins.h           # confirmed GPIOs from Phase 0 (DC, CS, SPI, I2C)
│   ├── config.h               # units, intervals, entity ids, colors
│   ├── secrets.h              # gitignored (real creds)
│   └── secrets.h.example      # committed template
└── src/
    ├── main.cpp               # setup/loop, scheduler
    ├── display/
    │   ├── lgfx_device.hpp    # LovyanGFX LGFX subclass (or Adafruit fallback)
    │   ├── lvgl_port.cpp/.h   # buffers (PSRAM), flush_cb, touch read_cb, tick
    │   └── ui.cpp/.h          # screen, styles (black/dark), big-temp label, etc.
    ├── sensors/
    │   ├── climate.cpp/.h     # HDC3022 read + feels-like
    │   └── battery.cpp/.h     # MAX17048
    └── net/
        ├── wifi.cpp/.h
        └── ha_mqtt.cpp/.h     # ArduinoHA entities + publish
```

---

## 11. Secrets

Never commit credentials.

`include/secrets.h.example` (committed):
```cpp
#pragma once
#define WIFI_SSID   "your-ssid"
#define WIFI_PASS   "your-pass"
#define MQTT_HOST   "192.168.1.x"
#define MQTT_USER   "mqtt-user"
#define MQTT_PASS   "mqtt-pass"
```
Add `include/secrets.h` to `.gitignore`. First task in any fresh clone: copy the example to `secrets.h`.

---

## 12. Build / flash / monitor

```bash
pio run                              # build
pio run -t upload                    # flash (board enters native-USB DFU automatically)
pio device monitor                   # serial @115200, with exception decoder
pio run -t upload -t monitor         # flash + monitor
```
- If upload fails: double-tap reset to force the UF2/ROM bootloader, or hold BOOT while tapping RESET.

---

## 13. Suggested build order (milestones)

Build incrementally and verify each before moving on — don't write the whole firmware then debug a black screen.

1. **Phase 0:** Confirm DC/CS GPIOs (section 3). Write `board_pins.h`.
2. **I2C scan:** Print bus, confirm `0x36 / 0x38 / 0x44` all present.
3. **Display bring-up:** Solid color fills + a test string via LovyanGFX (no LVGL yet). Get orientation right (landscape, text upright).
4. **Touch bring-up:** Print FT6206 coordinates on tap; verify axis mapping matches the display rotation.
5. **LVGL hello:** LVGL + flush/touch callbacks, buffers in PSRAM, a label that updates.
6. **Sensors:** HDC3022 + MAX17048 reading real values to serial, then onto the screen.
7. **Final UI:** Black theme, huge temp font, humidity / feels-like / battery laid out and styled.
8. **Wi-Fi + HA:** Connect, register entities, confirm temp + humidity appear in Home Assistant.
9. **Hardening:** Reconnect logic for Wi-Fi/MQTT, sensor-read failure handling (show last-good / a fault glyph), sane update intervals.

---

## 14. Conventions & guardrails

- Keep all confirmed pin numbers in `board_pins.h` — never sprinkle magic GPIO literals through the code.
- Keep units, intervals, colors, and HA entity ids in `config.h`.
- Prefer non-blocking timing (millis-based scheduler) over `delay()` in `loop()` so LVGL stays responsive.
- LVGL draw buffers → PSRAM. Watch internal-RAM/heap headroom; log free heap during bring-up.
- When unsure about a hardware detail, **verify against the live variant file / Adafruit PrettyPins**, don't assume. The single most likely source of "nothing on screen" is a wrong DC/CS GPIO.

---

## 15. Reference links

- Metro ESP32-S3 (5500): https://learn.adafruit.com/adafruit-metro-esp32-s3 — pinouts & MAX17048 (0x36)
- 2.8" Cap-Touch Shield (1947): https://learn.adafruit.com/adafruit-2-8-tft-touch-shield-v2 — ILI9341 + FT6206 (0x38), DC=D9 / CS=D10
- HDC302x (5989): https://learn.adafruit.com/adafruit-hdc3021-precision-temperature-humidity-sensor — lib `Adafruit_HDC302x`, addr 0x44
- HDC302x library: https://github.com/adafruit/Adafruit_HDC302x
- MAX1704X library: https://github.com/adafruit/Adafruit_MAX1704X
- LovyanGFX: https://github.com/lovyan03/LovyanGFX
- LVGL: https://docs.lvgl.io  •  font converter: https://lvgl.io/tools/fontconverter
- ArduinoHA (HA MQTT): https://github.com/dawidchyrzynski/arduino-home-assistant
- PlatformIO board page: https://docs.platformio.org/en/latest/boards/espressif32/adafruit_metro_esp32s3.html

---

## 16. Build status & resume notes  _(updated 2026-06-12)_

### Done & verified on hardware
- **Milestone 0-1 — scaffold.** `platformio.ini`, `board_pins.h`, `config.h`, `lv_conf.h`,
  `secrets.h.example`, `.gitignore`. Libraries **pinned** (see below).
- **Milestone 2 — I2C scan.** All three devices confirmed live on the shared bus
  (`0x36 / 0x38 / 0x44`). PSRAM confirmed working (~8.38 MB free).
- **Milestone 3-4 — display + touch.** LovyanGFX ILI9341 up; colors correct; orientation
  fixed at **rotation 3** (`TFT_ROTATION` in `config.h`). FT6206 touch validated.
- **Milestone 5 — LVGL.** PSRAM double draw buffers, flush + millis tick. Renders cleanly.
- **Milestone 6-7 — sensors + UI.** HDC3022 + MAX17048 reading real values; black UI with a
  **custom ~110px hero temperature font** (`src/display/font_temp_110.c`), feels-like, humidity,
  battery. Layout tuned and approved.

### Code-complete but NOT yet flashed/tested
- **Milestone 8 — Wi-Fi + Home Assistant.** `src/net/wifi.*` + `src/net/ha_mqtt.*` written and
  the full firmware **compiles** (Flash ~22%, RAM ~45%). Not run yet — needs real credentials
  and a broker.

### TO RESUME (next session, in order)
1. Put real values in **`include/secrets.h`** (gitignored): Wi-Fi SSID/pass, `MQTT_HOST`,
   `MQTT_USER`/`MQTT_PASS`. Ensure an **MQTT broker** is running (Mosquitto add-on in HA) and the
   MQTT integration is enabled.
2. `pio run -t upload`, then `pio device monitor`. Confirm Wi-Fi connects and the four entities
   (Temperature, Humidity, Feels Like, Battery) appear in HA via auto-discovery.
3. **Milestone 9 — hardening:** verify Wi-Fi/MQTT auto-reconnect, show last-good / fault glyph on
   sensor read failure, confirm sane intervals, long-run soak.

### Key decisions / deviations from the original plan
- **Phase 0 result:** on this board the Arduino header pins map 1:1 to GPIO — **DC = GPIO9,
  CS = GPIO10** (verified from the pinout PDF). Default SPI = SCK39/MOSI42/MISO21, I2C = SDA47/SCL48.
- **Library deps corrected:** ArduinoHA depends on **`knolleary/PubSubClient`**, NOT ArduinoJson
  (it ships its own serializer). ArduinoJson was dropped from `lib_deps`.
- **Pinned versions:** lvgl 9.5.0 · LovyanGFX 1.2.21 · Adafruit HDC302x 1.0.3 ·
  Adafruit MAX1704X 1.0.3 · Adafruit BusIO 1.17.4 · home-assistant-integration 2.1.0 ·
  PubSubClient 2.8.
- **Touch dropped from the running firmware.** To keep the single I2C bus owned by exactly one
  driver (Arduino `Wire`, used by the sensors) and avoid dual-master flakiness on arduino-esp32
  3.x, LovyanGFX is **display-only** (`lgfx_device.hpp`). FT6206 hardware is validated and can be
  re-added later via `Adafruit_FT6206` on `Wire` (e.g. for a unit toggle / brightness).
- **LVGL `%f` fix:** `LV_USE_STDLIB_SPRINTF = LV_STDLIB_CLIB` in `lv_conf.h` — LVGL's *builtin*
  sprintf doesn't support float specifiers (symptom: labels showed a literal `f`).
- **Hero font:** generated with `npx lv_font_conv` from LVGL's bundled `Montserrat-Medium.ttf`,
  ~110px, glyphs `0-9 . -` only → `src/display/font_temp_110.c` (`font_temp_110`).

### ⚠️ Windows toolchain gotcha (IMPORTANT)
`pio` upload/monitor can crash mid-flash with **`UnicodeEncodeError` (cp1252)** — esptool emits
non-cp1252 chars (progress blocks) that the Windows console can't encode, killing the flash and
leaving the board half-programmed / port-hopping. **Fix:** force UTF-8. A user-wide
`setx PYTHONIOENCODING utf-8` was applied; also belt-and-suspenders prefix commands with
`$env:PYTHONUTF8='1'`. If a flash ever corrupts and the board hops COM ports, recover by manually
entering ROM download mode (hold **BOOT**, tap **RESET**, release BOOT) for a stable port. Normal
flashing now auto-resets over the native USB-Serial/JTAG with no button press.