/* adc - doc gia tri analog (cam bien do am dat, current sense BTS7960)
 *
 * 1. Chuc nang: doc raw ADC, quy doi dien ap, quy doi % do am co loc nhieu
 * 2. Input : so chan ADC, cap hieu chuan raw_dry/raw_wet
 * 3. Output: gia tri raw (0-4095), dien ap (V), do am (%)
 * 4. Thu vien: analogRead, analogReadResolution, analogSetPinAttenuation
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define ADC_SAMPLE_COUNT   16


void adc_setup(uint8_t pin);

// Doc gia tri tho 1 lan: 0 - 4095
uint16_t adc_read_raw(uint8_t pin);

// Doc gia tri tho co loc: lay ADC_SAMPLE_COUNT mau roi lay trung binh
// -> giam nhieu ngau nhien, nen dung cho cam bien do am dat
uint16_t adc_read_raw_filtered(uint8_t pin);


float adc_read_voltage(uint8_t pin);


float adc_read_percent(uint8_t pin, uint16_t raw_dry, uint16_t raw_wet);

#ifdef __cplusplus
}
#endif

#endif
