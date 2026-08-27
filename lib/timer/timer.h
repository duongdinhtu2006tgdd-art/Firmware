/* timer - do thoi gian va dinh thoi non-blocking
 * 1. Chuc nang: lay moc thoi gian, timer dinh ky, timer mot lan (timeout)
 * 2. Input : chu ky / thoi han (ms)
 * 3. Output: co bao het chu ky, so ms da troi qua
 * 4. Thu vien: millis(), micros(), delayMicroseconds()
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    uint32_t interval_ms;
    uint32_t last_tick;
    bool     running;
} periodic_timer_t;


typedef struct {
    uint32_t duration_ms;
    uint32_t start_ms;
    bool     running;
} oneshot_timer_t;


void timer_setup(void);


uint32_t timer_millis(void);
uint32_t timer_micros(void);


uint32_t timer_elapsed_ms(uint32_t since_ms);
uint32_t timer_elapsed_us(uint32_t since_us);


void timer_delay_us(uint32_t us);
void timer_delay_ms(uint32_t ms);

void periodic_timer_start(periodic_timer_t *t, uint32_t interval_ms);
void periodic_timer_stop(periodic_timer_t *t);

bool periodic_timer_expired(periodic_timer_t *t);


void oneshot_timer_start(oneshot_timer_t *t, uint32_t duration_ms);
void oneshot_timer_cancel(oneshot_timer_t *t);
bool oneshot_timer_is_running(const oneshot_timer_t *t);

bool oneshot_timer_expired(oneshot_timer_t *t);


uint32_t oneshot_timer_remaining_ms(const oneshot_timer_t *t);

#ifdef __cplusplus
}
#endif

#endif 
