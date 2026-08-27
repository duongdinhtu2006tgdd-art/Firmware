/* pwm - xuat PWM dieu khien dong co nang ha (thongg qua BTS7960)
 * 1. Chuc nang: cau hinh kenh LEDC, dat duty theo %, dung PWM
 * 2. Input : channel LEDC, chan, tan so (Hz), do phan giai (bit), duty (%)
 * 3. Output: xung PWM tren chan tuong ung
 * 4. Thu vien: ledcSetup, ledcAttachPin, ledcWrite, ledcDetachPin
 */

#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PWM_MAX_CHANNELS   16

bool pwm_setup(uint8_t channel, uint8_t pin, uint32_t freq_hz, uint8_t resolution_bits);

void pwm_set_duty(uint8_t channel, uint8_t duty_percent);


void pwm_set_duty_raw(uint8_t channel, uint32_t duty_raw);


void pwm_stop(uint8_t channel);

void pwm_detach(uint8_t channel, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif 
