#include "timer.h"
#include <Arduino.h>

void timer_setup(void) {

}

uint32_t timer_millis(void) { return millis(); }
uint32_t timer_micros(void) { return micros(); }

// Phep tru unsigned tu dong wrap dung qua moc tran 2^32
uint32_t timer_elapsed_ms(uint32_t since_ms) { return millis() - since_ms; }
uint32_t timer_elapsed_us(uint32_t since_us) { return micros() - since_us; }

void timer_delay_us(uint32_t us) { delayMicroseconds(us); }
void timer_delay_ms(uint32_t ms) { delay(ms); }

void periodic_timer_start(periodic_timer_t *t, uint32_t interval_ms) {
    if (!t) return;
    t->interval_ms = interval_ms;
    t->last_tick   = millis();
    t->running     = true;
}

void periodic_timer_stop(periodic_timer_t *t) {
    if (!t) return;
    t->running = false;
}

bool periodic_timer_expired(periodic_timer_t *t) {
    if (!t || !t->running) return false;

    if (millis() - t->last_tick >= t->interval_ms) {

        t->last_tick += t->interval_ms;

        if (millis() - t->last_tick >= t->interval_ms) {
            t->last_tick = millis();
        }
        return true;
    }
    return false;
}

void oneshot_timer_start(oneshot_timer_t *t, uint32_t duration_ms) {
    if (!t) return;
    t->duration_ms = duration_ms;
    t->start_ms    = millis();
    t->running     = true;
}

void oneshot_timer_cancel(oneshot_timer_t *t) {
    if (!t) return;
    t->running = false;
}

bool oneshot_timer_is_running(const oneshot_timer_t *t) {
    return t && t->running;
}

bool oneshot_timer_expired(oneshot_timer_t *t) {
    if (!t || !t->running) return false;

    if (millis() - t->start_ms >= t->duration_ms) {
        t->running = false;
        return true;
    }
    return false;
}

uint32_t oneshot_timer_remaining_ms(const oneshot_timer_t *t) {
    if (!t || !t->running) return 0;

    uint32_t elapsed = millis() - t->start_ms;
    return (elapsed >= t->duration_ms) ? 0 : (t->duration_ms - elapsed);
}
