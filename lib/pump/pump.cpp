#include "pump.h"
#include "gpio.h"
#include <Arduino.h>


static void relay_apply(pump_t *dev, bool on) {
    uint8_t level = dev->active_low ? (on ? 0 : 1) : (on ? 1 : 0);
    gpio_write(dev->pin, level);
}

void pump_setup(pump_t *dev, uint8_t pin, bool active_low) {
    if (!dev) return;

    dev->pin              = pin;
    dev->active_low       = active_low;
    dev->running          = false;
    dev->start_ms         = 0;
    dev->stop_ms          = 0;
    dev->last_pulse_ms    = 0;
    dev->ever_stopped     = false;
    dev->target_liters    = 0.0f;
    dev->liters_at_start  = 0.0f;
    dev->last_stop_reason = PUMP_STOP_NONE;
    dev->total_run_ms     = 0;
    dev->dry_run_count    = 0;
    oneshot_timer_cancel(&dev->run_limit);


    relay_apply(dev, false);
    gpio_setup(pin, PIN_MODE_OUTPUT);
    relay_apply(dev, false);
}

bool pump_start(pump_t *dev, uint32_t max_run_ms, float target_liters,
                float current_liters) {
    if (!dev) return false;
    if (dev->running) return false;         
    if (pump_in_cooldown(dev)) return false; 

    if (max_run_ms == 0 || max_run_ms > PUMP_MAX_RUN_MS) {
        max_run_ms = PUMP_MAX_RUN_MS;
    }

    dev->running          = true;
    dev->start_ms         = millis();
    dev->last_pulse_ms    = millis();  
    dev->target_liters    = target_liters;
    dev->liters_at_start  = current_liters;
    dev->last_stop_reason = PUMP_STOP_NONE;


    oneshot_timer_start(&dev->run_limit, max_run_ms);

    relay_apply(dev, true);
    return true;
}

void pump_stop(pump_t *dev, pump_stop_reason_t reason) {
    if (!dev) return;

    relay_apply(dev, false);   

    if (dev->running) {
        dev->total_run_ms += millis() - dev->start_ms;
    }

    dev->running          = false;
    dev->stop_ms          = millis();
    dev->ever_stopped     = true;
    dev->last_stop_reason = reason;
    oneshot_timer_cancel(&dev->run_limit);
}

bool pump_update(pump_t *dev, float current_liters, bool flow_has_pulse) {
    if (!dev || !dev->running) return false;


    if (oneshot_timer_expired(&dev->run_limit)) {
        pump_stop(dev, PUMP_STOP_TIMEOUT);
        return true;
    }


    if (flow_has_pulse) dev->last_pulse_ms = millis();


    uint32_t running_ms = millis() - dev->start_ms;
    if (running_ms > PUMP_DRY_RUN_MS) {
        uint32_t since_pulse = millis() - dev->last_pulse_ms;
        if (since_pulse > PUMP_DRY_RUN_MS) {
            dev->dry_run_count++;
            pump_stop(dev, PUMP_STOP_DRY_RUN);
            return true;
        }
    }


    if (dev->target_liters > 0.0f) {
        if (pump_session_liters(dev, current_liters) >= dev->target_liters) {
            pump_stop(dev, PUMP_STOP_TARGET_REACHED);
            return true;
        }
    }

    return false;
}

bool pump_is_running(const pump_t *dev) {
    return dev && dev->running;
}

float pump_session_liters(const pump_t *dev, float current_liters) {
    if (!dev) return 0.0f;

    float liters = current_liters - dev->liters_at_start;
    return (liters > 0.0f) ? liters : 0.0f;
}

uint32_t pump_remaining_ms(const pump_t *dev) {
    if (!dev || !dev->running) return 0;
    return oneshot_timer_remaining_ms(&dev->run_limit);
}

bool pump_in_cooldown(const pump_t *dev) {
    if (!dev) return false;
    if (!dev->ever_stopped) return false;  
    if (dev->running) return false;

    return (millis() - dev->stop_ms) < PUMP_MIN_OFF_MS;
}

const char *pump_stop_reason_str(pump_stop_reason_t reason) {
    switch (reason) {
        case PUMP_STOP_NONE:           return "NONE";
        case PUMP_STOP_MANUAL:         return "MANUAL";
        case PUMP_STOP_TARGET_REACHED: return "TARGET_REACHED";
        case PUMP_STOP_TIMEOUT:        return "TIMEOUT (thuat toan loi?)";
        case PUMP_STOP_DRY_RUN:        return "DRY_RUN (khong co nuoc!)";
        default:                       return "UNKNOWN";
    }
}
