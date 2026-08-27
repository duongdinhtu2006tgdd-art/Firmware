#include "adc.h"
#include <Arduino.h>

#define ADC_MAX_RAW   4095.0f
#define ADC_VREF      3.3f

void adc_setup(uint8_t pin) {
    pinMode(pin, INPUT);
    analogReadResolution(12);

    analogSetPinAttenuation(pin, ADC_11db);
}

uint16_t adc_read_raw(uint8_t pin) {
    return (uint16_t)analogRead(pin);
}

uint16_t adc_read_raw_filtered(uint8_t pin) {

    uint32_t sum = 0;
    for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; i++) {
        sum += (uint32_t)analogRead(pin);
    }
    return (uint16_t)(sum / ADC_SAMPLE_COUNT);
}

float adc_read_voltage(uint8_t pin) {
    return ((float)adc_read_raw_filtered(pin) * ADC_VREF) / ADC_MAX_RAW;
}

float adc_read_percent(uint8_t pin, uint16_t raw_dry, uint16_t raw_wet) {

    if (raw_dry == raw_wet) return -1.0f;

    float raw = (float)adc_read_raw_filtered(pin);
    float percent = ((float)raw_dry - raw) * 100.0f / ((float)raw_dry - (float)raw_wet);

    if (percent < 0.0f)   percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    return percent;
}
