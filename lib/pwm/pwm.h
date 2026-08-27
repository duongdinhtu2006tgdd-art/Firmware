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

// Khoi tao 1 kenh PWM gan voi 1 chan.
// channel: 0-15 ; resolution_bits: 1-20 (thuong dung 8 hoac 10)
// Tra ve false neu tham so khong hop le hoac vuot rang buoc f*2^bits <= 80MHz
bool pwm_setup(uint8_t channel, uint8_t pin, uint32_t freq_hz, uint8_t resolution_bits);

// Dat duty cycle theo % (0-100). Duty duoc quy doi theo do phan giai
void pwm_set_duty(uint8_t channel, uint8_t duty_percent);

// Dat duty bang gia tri tho (0 - (2^resolution)-1), dung khi can do min hon 1%
void pwm_set_duty_raw(uint8_t channel, uint32_t duty_raw);

// Tat PWM (duty = 0) nhung van giu cau hinh kenh
void pwm_stop(uint8_t channel);

// Nha chan ra khoi kenh LEDC va keo chan ve LOW
void pwm_detach(uint8_t channel, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif // PWM_H
