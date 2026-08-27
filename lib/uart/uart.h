/* uart - giao tiep UART2 voi Node 1 (camera / PC chay YOLO)
 * 1. Chuc nang: gui/nhan ban tin dang dong ket thuc boi '\n'
 * 2. Input : baudrate, chan RX/TX, du lieu can gui
 * 3. Output: du lieu gui ra day UART2, 1 dong hoan chinh khi nhan du
 * 4. Thu vien: HardwareSerial (Serial2)
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


#define UART_LINE_MAX      256


#define UART_RX_TIMEOUT_MS 2000

typedef enum {
    UART_RX_IDLE = 0,     // chua nhan du 1 dong
    UART_RX_LINE_READY,   // da co 1 dong hoan chinh trong buffer caller
    UART_RX_OVERFLOW,     // dong dai qua UART_LINE_MAX -> da bo
    UART_RX_TIMEOUT       // qua han cho '\n' -> da bo phan da nhan
} uart_rx_status_t;

// Khoi tao UART2. rx_pin/tx_pin: dung -1 de lay mac dinh (16/17)
void uart_setup(uint32_t baudrate, int8_t rx_pin, int8_t tx_pin);

// Gui chuoi thuan 
void uart_send(const char *data);

// Gui chuoi va tu dong them '\n'
void uart_send_line(const char *data);

// So byte dang cho trong FIFO nhan
uint16_t uart_available(void);


uart_rx_status_t uart_read_line(char *buffer, uint16_t buffer_size, uint16_t *out_len);


void uart_flush_rx(void);

#ifdef __cplusplus
}
#endif

#endif // UART_H
