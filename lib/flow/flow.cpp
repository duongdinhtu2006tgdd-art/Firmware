#include "flow.h"
#include <Arduino.h>

void flow_setup(flow_t *dev, uint8_t pin, float pulses_per_liter) {
    if (!dev) return;

    dev->pin              = pin;
    dev->pulses_per_liter = (pulses_per_liter > 0.0f)
                            ? pulses_per_liter
                            : FLOW_PULSES_PER_LITER;
    dev->total_pulses     = 0;
    dev->total_liters     = 0.0f;
    dev->rate_lpm         = 0.0f;
    dev->last_update_ms   = millis();
    dev->last_pulse_ms    = millis();
}

void flow_update(flow_t *dev, uint32_t pulse_count) {
    if (!dev) return;

    uint32_t now = millis();
    uint32_t dt_ms = now - dev->last_update_ms;   // unsigned: wrap dung
    dev->last_update_ms = now;

    dev->total_pulses += pulse_count;
    dev->total_liters += (float)pulse_count / dev->pulses_per_liter;

    if (pulse_count > 0) {
        dev->last_pulse_ms = now;
    }


    if (dt_ms == 0) return;


    float liters = (float)pulse_count / dev->pulses_per_liter;
    dev->rate_lpm = liters * 60000.0f / (float)dt_ms;
}

void flow_reset_total(flow_t *dev) {
    if (!dev) return;

    dev->total_pulses = 0;
    dev->total_liters = 0.0f;
    dev->rate_lpm     = 0.0f;
}

bool flow_is_dry_run(const flow_t *dev, bool pump_on) {
    if (!dev || !pump_on) return false;

    return (millis() - dev->last_pulse_ms) > FLOW_DRY_RUN_TIMEOUT_MS;
}
