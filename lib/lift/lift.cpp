#include "lift.h"
#include "gpio.h"
#include "pwm.h"
#include "adc.h"
#include <Arduino.h>


static bool limit_pressed(uint8_t pin) {
    return gpio_read(pin) == 0;
}


static void bridge_off(lift_t *dev) {
    pwm_stop(dev->ch_r);
    pwm_stop(dev->ch_l);
    gpio_write(dev->pin_r_en, 0);
    gpio_write(dev->pin_l_en, 0);
}

bool lift_setup(lift_t *dev,
                uint8_t pin_rpwm, uint8_t pin_lpwm,
                uint8_t pin_r_en, uint8_t pin_l_en,
                uint8_t ch_r, uint8_t ch_l,
                uint32_t pwm_freq_hz, uint8_t pwm_res_bits,
                uint8_t pin_limit_up, uint8_t pin_limit_down,
                uint8_t pin_r_is, uint8_t pin_l_is) {
    if (!dev) return false;

    dev->pin_rpwm = pin_rpwm;   dev->pin_lpwm = pin_lpwm;
    dev->pin_r_en = pin_r_en;   dev->pin_l_en = pin_l_en;
    dev->pin_limit_up = pin_limit_up;
    dev->pin_limit_down = pin_limit_down;
    dev->pin_r_is = pin_r_is;   dev->pin_l_is = pin_l_is;
    dev->ch_r = ch_r;           dev->ch_l = ch_l;

    dev->dir               = LIFT_DIR_STOP;
    dev->target_duty       = 0;
    dev->current_duty      = 0;
    dev->move_start_ms     = 0;
    dev->deadtime_until_ms = 0;
    dev->last_stop_reason  = LIFT_STOP_NONE;
    dev->last_current_v    = 0.0f;
    dev->overcurrent_count = 0;

    gpio_write(pin_r_en, 0);
    gpio_setup(pin_r_en, PIN_MODE_OUTPUT);
    gpio_write(pin_r_en, 0);

    gpio_write(pin_l_en, 0);
    gpio_setup(pin_l_en, PIN_MODE_OUTPUT);
    gpio_write(pin_l_en, 0);

    gpio_setup(pin_limit_up,   PIN_MODE_INPUT_PULLUP);
    gpio_setup(pin_limit_down, PIN_MODE_INPUT_PULLUP);

    adc_setup(pin_r_is);
    adc_setup(pin_l_is);

    bool ok_r = pwm_setup(ch_r, pin_rpwm, pwm_freq_hz, pwm_res_bits);
    bool ok_l = pwm_setup(ch_l, pin_lpwm, pwm_freq_hz, pwm_res_bits);

    pwm_stop(ch_r);
    pwm_stop(ch_l);

    return ok_r && ok_l;
}

bool lift_move(lift_t *dev, lift_dir_t dir, uint8_t duty_percent) {
    if (!dev) return false;
    if (dir == LIFT_DIR_STOP) { lift_stop(dev, LIFT_STOP_MANUAL); return true; }

  
    if (millis() < dev->deadtime_until_ms) return false;


    if (dir == LIFT_DIR_UP && limit_pressed(dev->pin_limit_up)) {
        dev->last_stop_reason = LIFT_STOP_BLOCKED;
        return false;
    }
    if (dir == LIFT_DIR_DOWN && limit_pressed(dev->pin_limit_down)) {
        dev->last_stop_reason = LIFT_STOP_BLOCKED;
        return false;
    }

    if (dev->dir != LIFT_DIR_STOP && dev->dir != dir) {
        lift_stop(dev, LIFT_STOP_MANUAL);
        return false;
    }

    if (duty_percent == 0)   duty_percent = LIFT_DEFAULT_DUTY;
    if (duty_percent > 100)  duty_percent = 100;


    bridge_off(dev);

    dev->dir              = dir;
    dev->target_duty      = duty_percent;
    dev->current_duty     = 0;      
    dev->move_start_ms    = millis();
    dev->last_stop_reason = LIFT_STOP_NONE;

   
    if (dir == LIFT_DIR_UP) {
        gpio_write(dev->pin_r_en, 1);
    } else {
        gpio_write(dev->pin_l_en, 1);
    }

    return true;
}

void lift_stop(lift_t *dev, lift_stop_reason_t reason) {
    if (!dev) return;

    bridge_off(dev);

    dev->dir              = LIFT_DIR_STOP;
    dev->target_duty      = 0;
    dev->current_duty     = 0;
    dev->last_stop_reason = reason;


    dev->deadtime_until_ms = millis() + LIFT_DEADTIME_MS;
}

bool lift_update(lift_t *dev) {
    if (!dev || dev->dir == LIFT_DIR_STOP) return false;

    uint32_t elapsed = millis() - dev->move_start_ms;


    if (dev->dir == LIFT_DIR_UP && limit_pressed(dev->pin_limit_up)) {
        lift_stop(dev, LIFT_STOP_LIMIT);
        return true;
    }
    if (dev->dir == LIFT_DIR_DOWN && limit_pressed(dev->pin_limit_down)) {
        lift_stop(dev, LIFT_STOP_LIMIT);
        return true;
    }


    dev->last_current_v = lift_current_voltage(dev);
    if (elapsed > LIFT_INRUSH_IGNORE_MS &&
        dev->last_current_v > LIFT_OVERCURRENT_V) {
        dev->overcurrent_count++;
        lift_stop(dev, LIFT_STOP_OVERCURRENT);
        return true;
    }


    if (elapsed > LIFT_MAX_TRAVEL_MS) {
        lift_stop(dev, LIFT_STOP_TIMEOUT);
        return true;
    }

    if (dev->current_duty < dev->target_duty) {
        uint32_t ramped = (uint32_t)dev->target_duty * elapsed / LIFT_RAMP_MS;
        uint8_t duty = (ramped >= dev->target_duty)
                       ? dev->target_duty
                       : (uint8_t)ramped;

        if (duty != dev->current_duty) {
            dev->current_duty = duty;
            pwm_set_duty(dev->dir == LIFT_DIR_UP ? dev->ch_r : dev->ch_l, duty);
        }
    }

    return false;
}

lift_pos_t lift_position(const lift_t *dev) {
    if (!dev) return LIFT_POS_UNKNOWN;

    bool up   = limit_pressed(dev->pin_limit_up);
    bool down = limit_pressed(dev->pin_limit_down);


    if (up && down) return LIFT_POS_UNKNOWN;
    if (up)   return LIFT_POS_TOP;
    if (down) return LIFT_POS_BOTTOM;
    return LIFT_POS_MIDDLE;
}

bool lift_is_moving(const lift_t *dev) {
    return dev && dev->dir != LIFT_DIR_STOP;
}

float lift_current_voltage(const lift_t *dev) {
    if (!dev) return 0.0f;


    if (dev->dir == LIFT_DIR_UP)   return adc_read_voltage(dev->pin_r_is);
    if (dev->dir == LIFT_DIR_DOWN) return adc_read_voltage(dev->pin_l_is);
    return 0.0f;
}

const char *lift_dir_str(lift_dir_t dir) {
    switch (dir) {
        case LIFT_DIR_UP:   return "UP";
        case LIFT_DIR_DOWN: return "DOWN";
        case LIFT_DIR_STOP:
        default:            return "STOP";
    }
}

const char *lift_pos_str(lift_pos_t pos) {
    switch (pos) {
        case LIFT_POS_TOP:    return "TOP";
        case LIFT_POS_BOTTOM: return "BOTTOM";
        case LIFT_POS_MIDDLE: return "MIDDLE";
        case LIFT_POS_UNKNOWN:
        default:              return "UNKNOWN";
    }
}

const char *lift_stop_reason_str(lift_stop_reason_t reason) {
    switch (reason) {
        case LIFT_STOP_NONE:         return "NONE";
        case LIFT_STOP_MANUAL:       return "MANUAL";
        case LIFT_STOP_LIMIT:        return "LIMIT (het hanh trinh)";
        case LIFT_STOP_TIMEOUT:      return "TIMEOUT (co khi bi ket?)";
        case LIFT_STOP_OVERCURRENT:  return "OVERCURRENT (qua tai!)";
        case LIFT_STOP_BLOCKED:      return "BLOCKED (da o dau hanh trinh)";
        default:                     return "UNKNOWN";
    }
}
