/* flow - do luu luong nuoc (cam bien YF-S201)
 * 1. Chuc nang: dem xung -> quy doi lit da tuoi va luu luong tuc thoi (L/min)
 * 2. Input : so chan tin hieu, he so xung/lit
 * 3. Output: tong lit, L/min, tong so xung
 * 4. Thu vien: gpio (ngat) + timer
 */

#ifndef FLOW_H
#define FLOW_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// He so danh dinh YF-S201: 450 xung = 1 lit (F = 7.5 * Q)
#define FLOW_PULSES_PER_LITER   450.0f

// Bo qua xung den gan hon muc nay (chong nhieu).
// 1000 us tuong ung 1000 xung/giay = ~133 L/min, vuot xa kha nang cua
// YF-S201 (max 30 L/min) nen khong the la xung that.
#define FLOW_MIN_PULSE_US       1000

// Bom dang ON ma khong co xung nao trong khoang nay -> nghi chay kho
#define FLOW_DRY_RUN_TIMEOUT_MS 5000

typedef struct {
    uint8_t  pin;
    float    pulses_per_liter;  // he so hieu chuan
    uint32_t total_pulses;      // tong xung tu luc reset
    float    total_liters;      // tong lit da chay
    float    rate_lpm;          // luu luong tuc thoi (L/min)
    uint32_t last_update_ms;    // moc lan cap nhat truoc
    uint32_t last_pulse_ms;     // moc xung cuoi cung (de phat hien chay kho)
} flow_t;


void flow_setup(flow_t *dev, uint8_t pin, float pulses_per_liter);


void flow_update(flow_t *dev, uint32_t pulse_count);


void flow_reset_total(flow_t *dev);


bool flow_is_dry_run(const flow_t *dev, bool pump_on);

#ifdef __cplusplus
}
#endif

#endif 
