# psens

M5Stack AtomS3 pressure display/transmitter. Reads an analog 4–20 mA-style
(0.5–4.5 V) pressure sensor on GPIO5, shows the value on the built-in LCD,
exposes it over BLE GATT, and streams it to USB CDC serial.

---

## Hardware assumptions

| Item | Detail |
|------|--------|
| MCU | M5Stack AtomS3 (ESP32-S3) |
| Sensor output | 0.5 V – 4.5 V → 0 – 200 psi |
| ADC pin | GPIO5 (ADC1 channel 4) |
| Voltage divider | 10 kΩ (sensor→ADC) / 20 kΩ (ADC→GND) |

The 10 kΩ / 20 kΩ divider scales the 0.5–4.5 V sensor output to 0.33–3.0 V,
which sits safely within the ESP32-S3 ADC's 3.3 V full-scale range at 11 dB
attenuation.

```
Sensor out ──┬── 10 kΩ ── GPIO5 (ADC)
             └── 20 kΩ ── GND
```

---

## Conversion math

All arithmetic is integer / fixed-point to minimise floating-point usage.

**ADC → psi (in 0.1 psi units)**

```
ADC_mv     = adc_raw × 3300 / 4095          (3.3 V ref, 12-bit → 4095 counts)
sensor_mv  = ADC_mv  × 3/2                  (inverse of 2/3 divider)
           = adc_raw × 9900 / 8190

psi_tenths = (sensor_mv − 500) / 2          (0.5 V = 0 psi, 4 V span = 200 psi)

Combined:
psi_tenths = (adc_raw × 9900 − 4 095 000) / 16 380
```

**psi → bar (in 0.1 bar units)**

```
bar_tenths = psi_tenths × 6895 / 100 000    (1 psi = 0.068948 bar)
```

Result is clamped to the display range −9.9 … 99.9 bar.

**BLE payload**

```
int16_t  big-endian,  scale = 0.1 psi / count
value    = psi_tenths
```

---

## BLE GATT service

| Field | Value |
|-------|-------|
| Device name | `psens` |
| Service UUID | `CC4A6A80-51E0-11E3-B451-0002A5D5C51B` |
| Pressure characteristic UUID | `835AB4C0-51E4-11E3-A5BD-0002A5D5C51B` |
| Properties | READ, NOTIFY |
| Payload | Signed 16-bit integer, big-endian, 0.1 psi / count |

Advertising restarts automatically after a client disconnects.

---

## Serial output format

```
13.8 bar
13.9 bar
…
```

One line per 100 ms sample, printed only when a USB CDC host is connected
(115200 baud).

---

## Build, upload and monitor

```bash
# Install PlatformIO CLI if needed
pip install platformio

# Build
pio run

# Upload (AtomS3 connected via USB-C)
pio run --target upload

# Open serial monitor
pio device monitor
```

---

## Runtime verification checklist

- [ ] **Display** updates every ~100 ms without flicker; shows pressure in bar
      with one decimal place.
- [ ] **BLE** — use a BLE scanner app (e.g. nRF Connect):
  - Device named `psens` appears in scan results.
  - Service UUID `CC4A6A80-…` and pressure characteristic `835AB4C0-…` are
    visible.
  - Characteristic value updates when pressure changes (READ + NOTIFY).
- [ ] **Serial** — open monitor at 115200 baud; pressure readings appear once
      per 100 ms while USB CDC is connected.
