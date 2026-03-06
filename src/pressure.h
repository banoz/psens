#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * Pressure conversion helpers.
 *
 * Hardware: 10 kΩ (high-side) / 20 kΩ (low-side) voltage divider
 *           → ADC sees 2/3 of sensor output voltage.
 * Sensor:   0.5 V – 4.5 V → 0 – 200 psi.
 * ADC:      12-bit, 3.3 V reference (4095 counts full-scale).
 */

/** Convert 12-bit ADC reading to pressure in 0.1 psi units (for BLE payload). */
int16_t adc_to_psi_tenths(int adc_raw);

/**
 * Convert 12-bit ADC reading to pressure in 0.1 bar units.
 * Result is clamped to the display range [-99, 999] (−9.9 to 99.9 bar).
 */
int16_t adc_to_bar_tenths(int adc_raw);

/**
 * Render bar_tenths (0.1 bar units) as a null-terminated decimal string.
 * Examples: 138 → "13.8",  -5 → "-0.5",  999 → "99.9".
 * buf must be at least 6 bytes.
 */
void bar_tenths_to_str(int16_t bar_tenths, char *buf, size_t len);
