#include "pwm.h"
#include <Arduino.h>

// Luu do phan giai tung kenh de quy doi duty % -> duty raw chinh xac.
// Gia tri 0 nghia la kenh chua duoc init.
static uint8_t s_resolution_bits[PWM_MAX_CHANNELS] = {0};

bool pwm_setup(uint8_t channel, uint8_t pin, uint32_t freq_hz, uint8_t resolution_bits) {
    if (channel >= PWM_MAX_CHANNELS) return false;
    if (resolution_bits == 0 || resolution_bits > 20) return false;
    if (freq_hz == 0) return false;

    // Rang buoc phan cung: freq * 2^bits <= 80 MHz (clock APB).
    // Dung uint64_t/1ULL vi voi 20-bit thi phep nhan tran uint32_t.
    if ((uint64_t)freq_hz * (1ULL << resolution_bits) > 80000000ULL) return false;

    if (ledcSetup(channel, freq_hz, resolution_bits) == 0) return false;
    ledcAttachPin(pin, channel);

    s_resolution_bits[channel] = resolution_bits;
    ledcWrite(channel, 0);
    return true;
}

void pwm_set_duty(uint8_t channel, uint8_t duty_percent) {
    if (channel >= PWM_MAX_CHANNELS) return;
    if (s_resolution_bits[channel] == 0) return; // kenh chua init
    if (duty_percent > 100) duty_percent = 100;

    uint32_t max_duty = (1UL << s_resolution_bits[channel]) - 1UL;
    ledcWrite(channel, (max_duty * duty_percent) / 100UL);
}

void pwm_set_duty_raw(uint8_t channel, uint32_t duty_raw) {
    if (channel >= PWM_MAX_CHANNELS) return;
    if (s_resolution_bits[channel] == 0) return;

    uint32_t max_duty = (1UL << s_resolution_bits[channel]) - 1UL;
    if (duty_raw > max_duty) duty_raw = max_duty;
    ledcWrite(channel, duty_raw);
}

void pwm_stop(uint8_t channel) {
    if (channel >= PWM_MAX_CHANNELS) return;
    ledcWrite(channel, 0);
}

void pwm_detach(uint8_t channel, uint8_t pin) {
    if (channel >= PWM_MAX_CHANNELS) return;

    ledcWrite(channel, 0);
    ledcDetachPin(pin);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    s_resolution_bits[channel] = 0;
}
