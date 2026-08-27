#include "uart.h"
#include <Arduino.h>
#include <string.h>

// Serial (UART0) rieng cho debug qua USB
static HardwareSerial *s_uart = &Serial2;

// Buffer gom tung byte thanh 1 dong hoan chinh
static char     s_rx_buffer[UART_LINE_MAX];
static uint16_t s_rx_index = 0;
static uint32_t s_rx_start_ms = 0;   // moc thoi gian nhan byte dau tien cua dong

void uart_setup(uint32_t baudrate, int8_t rx_pin, int8_t tx_pin) {
    s_uart->begin(baudrate, SERIAL_8N1, rx_pin, tx_pin);
    s_rx_index = 0;
    s_rx_start_ms = 0;
}

void uart_send(const char *data) {
    if (data) s_uart->print(data);
}

void uart_send_line(const char *data) {
    if (data) s_uart->println(data);
}

uint16_t uart_available(void) {
    int n = s_uart->available();
    return (n > 0) ? (uint16_t)n : 0;
}

uart_rx_status_t uart_read_line(char *buffer, uint16_t buffer_size, uint16_t *out_len) {
    if (!buffer || buffer_size == 0) return UART_RX_IDLE;

    while (s_uart->available()) {
        char c = (char)s_uart->read();

        if (s_rx_index == 0) s_rx_start_ms = millis();

        if (c == '\r') continue; 

        if (c == '\n') {
            uint16_t len = s_rx_index;
            s_rx_index = 0;

            if (len == 0) continue; 

            
            uint16_t copy_len = (len < buffer_size - 1) ? len : (buffer_size - 1);
            memcpy(buffer, s_rx_buffer, copy_len);
            buffer[copy_len] = '\0';

            if (out_len) *out_len = copy_len;
            return UART_RX_LINE_READY;
        }

        if (s_rx_index < UART_LINE_MAX - 1) {
            s_rx_buffer[s_rx_index++] = c;
        } else {
            
            s_rx_index = 0;
            if (out_len) *out_len = 0;
            return UART_RX_OVERFLOW;
        }
    }


    if (s_rx_index > 0 && (millis() - s_rx_start_ms) > UART_RX_TIMEOUT_MS) {
        s_rx_index = 0;
        if (out_len) *out_len = 0;
        return UART_RX_TIMEOUT;
    }

    if (out_len) *out_len = 0;
    return UART_RX_IDLE;
}

void uart_flush_rx(void) {
    while (s_uart->available()) s_uart->read();
    s_rx_index = 0;
    s_rx_start_ms = 0;
}
