#include "gpio.h"
#include <Arduino.h>

void gpio_setup(uint8_t pin, pin_mode_t mode) {
    switch (mode) {
        case PIN_MODE_OUTPUT:
            pinMode(pin, OUTPUT);
            break;
        case PIN_MODE_INPUT_PULLUP:
            pinMode(pin, INPUT_PULLUP);
            break;
        case PIN_MODE_INPUT_PULLDOWN:
            pinMode(pin, INPUT_PULLDOWN);
            break;
        case PIN_MODE_INPUT:
        default:
            pinMode(pin, INPUT);
            break;
    }
}

void gpio_write(uint8_t pin, uint8_t level) {
    digitalWrite(pin, level ? HIGH : LOW);
}

uint8_t gpio_toggle(uint8_t pin) {
    uint8_t next = digitalRead(pin) ? LOW : HIGH;
    digitalWrite(pin, next);
    return next;
}

uint8_t gpio_read(uint8_t pin) {
    return (uint8_t)digitalRead(pin);
}

void gpio_attach_interrupt(uint8_t pin, void (*isr)(void), gpio_edge_t edge) {
    int arduino_edge;
    switch (edge) {
        case GPIO_EDGE_RISING:
            arduino_edge = RISING;
            break;
        case GPIO_EDGE_FALLING:
            arduino_edge = FALLING;
            break;
        case GPIO_EDGE_CHANGE:
        default:
            arduino_edge = CHANGE;
            break;
    }
    attachInterrupt(digitalPinToInterrupt(pin), isr, arduino_edge);
}

void gpio_detach_interrupt(uint8_t pin) {
    detachInterrupt(digitalPinToInterrupt(pin));
}
