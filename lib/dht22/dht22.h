/* dht22 - doc nhiet do va do am khong khi (cam bien AM2302/DHT22)
 *
 * 1. Chuc nang: doc nhiet do (C) va do am khong khi (%) qua giao thuc 1-wire
 * 2. Input : so chan DATA
 * 3. Output: nhiet do, do am, ma loi
 * 4. Thu vien: tu bit-bang bang gpio + timer (khong dung thu vien ngoai)
 */

#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Chu ky lay mau toi thieu cua DHT22 (datasheet: 2 giay)
#define DHT22_MIN_INTERVAL_MS   2000

// So lan thu lai khi doc that bai
#define DHT22_MAX_RETRY         3

typedef enum {
    DHT22_OK = 0,
    DHT22_ERR_TIMEOUT,      // cam bien khong phan hoi -> kiem tra day, pull-up
    DHT22_ERR_CHECKSUM,     // nhan du 40 bit nhung checksum sai -> nhieu
    DHT22_ERR_TOO_SOON,     // goi lai truoc 2 giay -> tra ve gia tri cu
    DHT22_ERR_RANGE         // gia tri ngoai dai do hop le
} dht22_status_t;

typedef struct {
    uint8_t  pin;
    float    temperature;    
    float    humidity;       
    uint32_t last_read_ms;   
    bool     valid;          
    uint32_t error_count;    
} dht22_t;

// Khoi tao. Goi 1 lan trong setup().
void dht22_setup(dht22_t *dev, uint8_t pin);


dht22_status_t dht22_read(dht22_t *dev);

const char *dht22_status_str(dht22_status_t status);

#ifdef __cplusplus
}
#endif

#endif 
