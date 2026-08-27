#include "pump.h"
#include "gpio.h"
#include <Arduino.h>

// Ghi muc logic tuong ung voi trang thai mong muon, xu ly ca 2 kieu relay
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

    // Ghi muc AN TOAN vao thanh ghi output TRUOC khi doi chan sang OUTPUT.
    // Neu lam nguoc, chan ra LOW vai chuc us -> relay low-active dong tiep
    // diem -> bom giat 1 nhip moi lan reset ESP32.
    relay_apply(dev, false);
    gpio_setup(pin, PIN_MODE_OUTPUT);
    relay_apply(dev, false);
}

bool pump_start(pump_t *dev, uint32_t max_run_ms, float target_liters,
                float current_liters) {
    if (!dev) return false;
    if (dev->running) return false;          // dang chay roi
    if (pump_in_cooldown(dev)) return false; // chua het thoi gian nghi

    if (max_run_ms == 0 || max_run_ms > PUMP_MAX_RUN_MS) {
        max_run_ms = PUMP_MAX_RUN_MS;
    }

    dev->running          = true;
    dev->start_ms         = millis();
    dev->last_pulse_ms    = millis();   // nap moc de lop 2 khong bao gia
    dev->target_liters    = target_liters;
    dev->liters_at_start  = current_liters;
    dev->last_stop_reason = PUMP_STOP_NONE;

    // Lop 1: gioi han thoi gian, doc lap voi moi cam bien
    oneshot_timer_start(&dev->run_limit, max_run_ms);

    relay_apply(dev, true);
    return true;
}

void pump_stop(pump_t *dev, pump_stop_reason_t reason) {
    if (!dev) return;

    relay_apply(dev, false);   // tat truoc, ghi so lieu sau

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

    // --- Lop 1: gioi han thoi gian ---
    // Kiem tra dau tien va khong phu thuoc cam bien nao: neu cam bien loi
    // het thi lop nay van cat duoc bom.
    if (oneshot_timer_expired(&dev->run_limit)) {
        pump_stop(dev, PUMP_STOP_TIMEOUT);
        return true;
    }

    // --- Lop 2: phat hien chay kho ---
    // Dung moc thoi gian rieng trong struct (khong dung flow_t) de 2 lop
    // doc lap nhau. Phai la field cua dev, khong duoc dung bien static cua
    // ham: neu sau nay co 2 bom thi chung se dung chung 1 moc -> sai.
    if (flow_has_pulse) dev->last_pulse_ms = millis();

    // Bo qua trong PUMP_DRY_RUN_MS dau: nuoc can thoi gian di tu bom
    // den cam bien, chua co xung ngay khong co nghia la chay kho.
    uint32_t running_ms = millis() - dev->start_ms;
    if (running_ms > PUMP_DRY_RUN_MS) {
        uint32_t since_pulse = millis() - dev->last_pulse_ms;
        if (since_pulse > PUMP_DRY_RUN_MS) {
            dev->dry_run_count++;
            pump_stop(dev, PUMP_STOP_DRY_RUN);
            return true;
        }
    }

    // --- Dat muc tieu lit ---
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
    if (!dev->ever_stopped) return false;   // chua tung chay -> khong phai nghi
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
