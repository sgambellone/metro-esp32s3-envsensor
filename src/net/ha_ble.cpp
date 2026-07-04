// BLE BTHome v2 broadcast path to Home Assistant. See ha_ble.h.
//
// BTHome v2 puts one "Service Data - 16-bit UUID" AD structure (UUID 0xFCD2)
// into the advertisement; NimBLE prepends the AD length/type + UUID, so we only
// hand it the bytes AFTER the UUID: a device-info byte then ascending object_id
// TLVs. Temperature is always sent in °C (BTHome's fixed unit; HA converts for
// display) regardless of the screen's USE_FAHRENHEIT setting.
//
// Payload reference (see sibling remote-temp-esp32s3 HANDOVER.md §5):
//   40              device info: BTHome v2, unencrypted, non-trigger
//   00 CC           obj 0x00  packet counter (uint8)
//   01 BB           obj 0x01  battery %      (uint8)      [only if valid]
//   02 TT TT        obj 0x02  temperature    (sint16 LE, x0.01 °C)
//   03 HH HH        obj 0x03  humidity       (uint16 LE, x0.01 %RH)
#include "ha_ble.h"

#include <Arduino.h>
#include <math.h>
#include <NimBLEDevice.h>
#include "../../include/config.h"

// ---- BTHome v2 constants -------------------------------------------------
#define BTHOME_SVC_UUID   0xFCD2   // little-endian on the wire -> D2 FC
#define BTHOME_DEV_INFO   0x40     // v2, unencrypted, non-trigger
#define BTHOME_OBJ_PACKET 0x00     // uint8  packet counter
#define BTHOME_OBJ_BATT   0x01     // uint8  battery %
#define BTHOME_OBJ_TEMP   0x02     // sint16 x0.01 degC
#define BTHOME_OBJ_HUMID  0x03     // uint16 x0.01 %RH

static bool    s_active  = false;
static uint8_t s_packet  = 0;   // BTHome object 0x00 — makes each advert "new"

// Push a freshly-built service-data buffer into the running advertisement.
// Advertising stays continuous; we briefly stop to swap the payload in, which
// is non-blocking (no delay) so LVGL keeps ticking.
//
// The 31-byte BLE advertisement budget is tight: flags (3) + the full BTHome
// service data (15 for an 11-byte payload) already needs 18 bytes, and the
// local name AD adds 2 + strlen. A name longer than ~11 chars (ours is 15,
// "room-climate-01") would push the packet past 31 bytes and NimBLE silently
// drops the service data — HA then sees only the packet counter, no temp/hum.
// So the name goes in the SCAN RESPONSE (its own 31-byte packet), leaving the
// main advertisement lean and the BTHome measurements always intact.
static void setAdvPayload(const uint8_t* svc, size_t len) {
  NimBLEAdvertisementData advData;
  advData.setFlags(0x06);  // LE General Discoverable, BR/EDR not supported
  advData.setServiceData(NimBLEUUID((uint16_t)BTHOME_SVC_UUID), svc, len);

  NimBLEAdvertisementData scanData;
  scanData.setName(BLE_LOCAL_NAME);

  NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
  pAdv->stop();
  pAdv->setAdvertisementData(advData);
  pAdv->setScanResponseData(scanData);
  pAdv->enableScanResponse(true);
  pAdv->start();
}

void ble_begin() {
  Serial.print(F("[BLE] NimBLEDevice::init ... "));
  Serial.flush();
  NimBLEDevice::init(BLE_LOCAL_NAME);
  Serial.println(F("ok"));
  Serial.flush();

  // Advertise an initial payload (counter only) so the device shows up in HA
  // even before the first sensor cycle completes.
  uint8_t svc[3];
  svc[0] = BTHOME_DEV_INFO;
  svc[1] = BTHOME_OBJ_PACKET;
  svc[2] = s_packet;
  setAdvPayload(svc, sizeof(svc));

  s_active = true;
  Serial.printf("[BLE] BTHome advertising as \"%s\"\n", BLE_LOCAL_NAME);
}

void ble_publish(const ClimateReading& c, const BatteryReading& b) {
  if (!s_active) return;
  if (!c.valid) {
    Serial.println(F("[BLE] climate invalid, skipping advert refresh"));
    return;
  }

  // BTHome sends °C; HA handles the F/C display conversion.
  int16_t  tRaw = (int16_t)lround(c.tempC / 0.01);
  uint16_t hRaw = (uint16_t)lround(c.humidity / 0.01);
  s_packet++;

  // Build ascending-object_id payload. Battery (0x01) is optional so we track
  // the length rather than using a fixed-size buffer.
  uint8_t svc[11];
  size_t  n = 0;
  svc[n++] = BTHOME_DEV_INFO;

  svc[n++] = BTHOME_OBJ_PACKET;
  svc[n++] = s_packet;

  if (b.valid) {
    uint8_t pct = (uint8_t)lround(constrain(b.percent, 0.0f, 100.0f));
    svc[n++] = BTHOME_OBJ_BATT;
    svc[n++] = pct;
  }

  svc[n++] = BTHOME_OBJ_TEMP;
  svc[n++] = (uint8_t)(tRaw & 0xFF);
  svc[n++] = (uint8_t)((tRaw >> 8) & 0xFF);

  svc[n++] = BTHOME_OBJ_HUMID;
  svc[n++] = (uint8_t)(hRaw & 0xFF);
  svc[n++] = (uint8_t)((hRaw >> 8) & 0xFF);

  setAdvPayload(svc, n);
  Serial.printf("[BLE] advert refreshed: %.2fC %.2f%%RH (pkt %u)\n",
                c.tempC, c.humidity, s_packet);
}

bool ble_active() {
  return s_active;
}
