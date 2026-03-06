#include <Arduino.h>
#include <M5Unified.h>
#include <NimBLEDevice.h>
#include "pressure.h"

// ── Hardware ──────────────────────────────────────────────────────────────────
#define ADC_PIN         5
#define SAMPLE_INTERVAL 100   // ms

// ── BLE ───────────────────────────────────────────────────────────────────────
#define SVC_UUID  "CC4A6A80-51E0-11E3-B451-0002A5D5C51B"
#define CHAR_UUID "835AB4C0-51E4-11E3-A5BD-0002A5D5C51B"

static NimBLECharacteristic *g_pchar      = nullptr;
static bool                  g_connected  = false;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *srv, ble_gap_conn_desc *desc) override {
        (void)srv;
        (void)desc;
        g_connected = true;
    }
    void onDisconnect(NimBLEServer *srv) override {
        (void)srv;
        g_connected = false;
        NimBLEDevice::startAdvertising();   // keep advertising after disconnect
    }
};

// ── Display sprite (flicker-free rendering) ───────────────────────────────────
static M5Canvas canvas(&M5.Display);

static void display_init()
{
    canvas.createSprite(M5.Display.width(), M5.Display.height());
    canvas.setTextDatum(middle_center);
}

static void display_update(int16_t bar_tenths)
{
    char buf[8];
    bar_tenths_to_str(bar_tenths, buf, sizeof(buf));

    int cx = canvas.width()  / 2;
    int cy = canvas.height() / 2;

    canvas.fillScreen(TFT_BLACK);

    // Large pressure value — textSize 5 gives ~30×40 px per glyph on 128×128 display
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(5);
    canvas.drawString(buf, cx, cy - 10);

    // Small, unobtrusive unit label
    canvas.setTextColor(TFT_DARKGREY);
    canvas.setTextSize(2);
    canvas.drawString("bar", cx, cy + 35);

    canvas.pushSprite(0, 0);   // atomic push to display — no flicker
}

// ── BLE ───────────────────────────────────────────────────────────────────────
static void ble_init()
{
    NimBLEDevice::init("psens");

    NimBLEServer *srv = NimBLEDevice::createServer();
    srv->setCallbacks(new ServerCallbacks());

    NimBLEService *svc = srv->createService(SVC_UUID);
    g_pchar = svc->createCharacteristic(
        CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    svc->start();

    NimBLEDevice::getAdvertising()->addServiceUUID(SVC_UUID);
    NimBLEDevice::startAdvertising();
}

static void ble_update(int16_t psi_tenths)
{
    // Big-endian signed 16-bit integer, scale = 0.1 psi / count.
    // Cast to uint16_t before shifting to ensure well-defined logical shift.
    uint16_t u = (uint16_t)psi_tenths;
    uint8_t payload[2] = { (uint8_t)(u >> 8), (uint8_t)(u & 0xFF) };
    g_pchar->setValue(payload, sizeof(payload));
    if (g_connected) {
        g_pchar->notify();
    }
}

// ── Arduino entry points ──────────────────────────────────────────────────────
void setup()
{
    Serial.begin(115200);

    M5.begin();

    // 12-bit ADC; 11 dB attenuation → full-scale ≈ 3.3 V (covers 3.0 V max from divider)
    analogReadResolution(12);
    analogSetPinAttenuation(ADC_PIN, ADC_11db);

    display_init();
    ble_init();

    display_update(0);   // show "0.0 bar" while waiting for first sample
}

void loop()
{
    static uint32_t last_ms       = 0;
    static int16_t  last_bar_t    = INT16_MIN;  // force display on first sample

    uint32_t now = millis();
    if (now - last_ms >= SAMPLE_INTERVAL) {
        last_ms = now;

        int     raw      = analogRead(ADC_PIN);
        int16_t psi_t    = adc_to_psi_tenths(raw);
        int16_t bar_t    = adc_to_bar_tenths(raw);

        ble_update(psi_t);

        if (Serial) {
            char buf[8];
            bar_tenths_to_str(bar_t, buf, sizeof(buf));
            Serial.print(buf);
            Serial.println(" bar");
        }

        if (bar_t != last_bar_t) {
            display_update(bar_t);
            last_bar_t = bar_t;
        }
    }

    M5.update();
}
