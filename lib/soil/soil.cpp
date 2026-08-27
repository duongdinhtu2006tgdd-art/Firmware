#include "soil.h"
#include "adc.h"
#include <Arduino.h>

void soil_setup(soil_t *dev, uint8_t pin, uint16_t raw_dry, uint16_t raw_wet,
                const char *name) {
    if (!dev) return;

    dev->pin         = pin;
    dev->raw_dry     = raw_dry;
    dev->raw_wet     = raw_wet;
    dev->raw         = 0;
    dev->percent     = 0.0f;
    dev->initialized = false;
    dev->name        = name ? name : "soil";

    adc_setup(pin);
}

soil_status_t soil_read(soil_t *dev) {
    if (!dev) return SOIL_ERR_NOT_CALIBRATED;

    if (dev->raw_dry == dev->raw_wet) return SOIL_ERR_NOT_CALIBRATED;

    uint16_t raw = adc_read_raw_filtered(dev->pin);
    dev->raw = raw;


    if (raw < SOIL_RAW_FAULT_LOW || raw > SOIL_RAW_FAULT_HIGH) {
        return SOIL_ERR_DISCONNECTED;
    }

    float percent = adc_read_percent(dev->pin, dev->raw_dry, dev->raw_wet);
    if (percent < 0.0f) return SOIL_ERR_NOT_CALIBRATED;  

    if (!dev->initialized) {

        dev->percent     = percent;
        dev->initialized = true;
    } else {
        dev->percent = SOIL_EMA_ALPHA * percent
                     + (1.0f - SOIL_EMA_ALPHA) * dev->percent;
    }

    return SOIL_OK;
}

void soil_calibrate(soil_t *dev, uint16_t raw_dry, uint16_t raw_wet) {
    if (!dev) return;

    dev->raw_dry     = raw_dry;
    dev->raw_wet     = raw_wet;
    dev->initialized = false;   
}

const char *soil_status_str(soil_status_t status) {
    switch (status) {
        case SOIL_OK:               return "OK";
        case SOIL_ERR_DISCONNECTED: return "DISCONNECTED (kiem tra day tin hieu)";
        case SOIL_ERR_NOT_CALIBRATED: return "NOT_CALIBRATED (raw_dry == raw_wet)";
        default:                    return "UNKNOWN";
    }
}
