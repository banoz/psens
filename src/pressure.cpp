#include "pressure.h"
#include <stdio.h>

/*
 * ADC → sensor voltage → psi (all integer arithmetic):
 *
 *   ADC_mv     = adc_raw × 3300 / 4095          (3.3 V ref, 12-bit)
 *   sensor_mv  = ADC_mv  × 3/2                  (inverse 2/3 divider)
 *              = adc_raw × 9900 / 8190
 *
 *   psi_tenths = (sensor_mv − 500) / 2           (0.5 V = 0 psi, 4 V span = 200 psi)
 *
 *   Combined, eliminating sensor_mv:
 *   psi_tenths = (adc_raw × 9900 − 4 095 000) / 16 380
 */
int16_t adc_to_psi_tenths(int adc_raw)
{
    int32_t n = (int32_t)adc_raw * 9900 - 4095000;
    return (int16_t)(n / 16380);
}

/*
 * 1 psi = 0.068948 bar
 * bar_tenths (0.1 bar) = psi_tenths (0.1 psi) × 0.068948
 *                      ≈ psi_tenths × 6895 / 100 000
 *
 * Max intermediate: 2000 × 6895 = 13 790 000 — fits in int32_t.
 * Clamped to display range: −9.9 to 99.9 bar → [−99, 999].
 */
int16_t adc_to_bar_tenths(int adc_raw)
{
    int32_t psi_t = adc_to_psi_tenths(adc_raw);
    int32_t bar_t = psi_t * 6895 / 100000;
    if (bar_t < -99)  bar_t = -99;
    if (bar_t > 999)  bar_t = 999;
    return (int16_t)bar_t;
}

/* Format signed tenths value as "X.X", "-X.X", "XX.X", etc. */
void bar_tenths_to_str(int16_t v, char *buf, size_t len)
{
    if (v < 0) {
        /* Avoid UB when negating: cast through uint16_t before converting back. */
        int16_t a = (int16_t)((uint16_t)(-v));
        snprintf(buf, len, "-%d.%d", (int)(a / 10), (int)(a % 10));
    } else {
        snprintf(buf, len, "%d.%d", (int)(v / 10), (int)(v % 10));
    }
}
