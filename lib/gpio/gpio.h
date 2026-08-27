/* gpio - dieu khien chan vao/ra + ngat
 *
 * 1. Chuc nang: cau hinh chan, doc/ghi muc logic, gan ngat canh
 * 2. Input : so chan (pin), che do (mode), muc logic, con tro ham ISR
 * 3. Output: muc logic tren chan, goi ISR khi co canh xung
 * 4. Thu vien: pinMode, digitalWrite, digitalRead, attachInterrupt
 *
*/
#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIN_MODE_INPUT = 0,
    PIN_MODE_OUTPUT,
    PIN_MODE_INPUT_PULLUP,
    PIN_MODE_INPUT_PULLDOWN
} pin_mode_t;

typedef enum {
    GPIO_EDGE_RISING = 0,
    GPIO_EDGE_FALLING,
    GPIO_EDGE_CHANGE
} gpio_edge_t;

// Khoi tao 1 chan GPIO voi che do tuong ung
void gpio_setup(uint8_t pin, pin_mode_t mode);

// Ghi muc logic ra chan (level != 0 -> HIGH)
void gpio_write(uint8_t pin, uint8_t level);

// Dao trang thai chan output, tra ve muc logic moi
uint8_t gpio_toggle(uint8_t pin);

// Doc muc logic tren chan
uint8_t gpio_read(uint8_t pin);


void gpio_attach_interrupt(uint8_t pin, void (*isr)(void), gpio_edge_t edge);

// Go bo ngat da gan cho chan
void gpio_detach_interrupt(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif 
