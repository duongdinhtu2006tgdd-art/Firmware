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

// Timer dinh ky: tu dong nap lai moc sau moi lan het chu ky
typedef struct {
    uint32_t interval_ms;
    uint32_t last_tick;
    bool     running;
} periodic_timer_t;

// Timer mot lan (dung lam timeout an toan, vd gioi han thoi gian bom chay)
typedef struct {
    uint32_t duration_ms;
    uint32_t start_ms;
    bool     running;
} oneshot_timer_t;

// Khoi tao he thong timer (Arduino core tu lo millis/micros tu luc boot,
// ham nay giu lai de dong nhat cau truc "moi driver deu co ham setup")
void timer_setup(void);

// --- Moc thoi gian ---
uint32_t timer_millis(void);
uint32_t timer_micros(void);

// So ms/us da troi qua ke tu `since`, an toan qua moc tran
uint32_t timer_elapsed_ms(uint32_t since_ms);
uint32_t timer_elapsed_us(uint32_t since_us);

// Delay chan - CHI dung cho giao thuc bit-bang (DHT22), khong dung trong loop
void timer_delay_us(uint32_t us);
void timer_delay_ms(uint32_t ms);

// --- Timer dinh ky ---
void periodic_timer_start(periodic_timer_t *t, uint32_t interval_ms);
void periodic_timer_stop(periodic_timer_t *t);

// Tra ve true DUNG 1 lan moi khi het chu ky, va tu dong nap lai moc.
// Goi lien tuc trong loop().
bool periodic_timer_expired(periodic_timer_t *t);

// --- Timer mot lan ---
void oneshot_timer_start(oneshot_timer_t *t, uint32_t duration_ms);
void oneshot_timer_cancel(oneshot_timer_t *t);
bool oneshot_timer_is_running(const oneshot_timer_t *t);

// Tra ve true DUNG 1 lan khi het han, sau do tu dung (running = false)
bool oneshot_timer_expired(oneshot_timer_t *t);

// So ms con lai truoc khi het han (0 neu da het hoac chua chay)
uint32_t oneshot_timer_remaining_ms(const oneshot_timer_t *t);

#ifdef __cplusplus
}
#endif

#endif // TIMER_H
