/* soil - cam bien do am dat
 *
 * 1. Chuc nang: doc do am dat theo %, phat hien cam bien roi day
 * 2. Input : chan ADC, 2 diem hieu chuan (raw kho / raw uot)
 * 3. Output: do am %, gia tri raw, trang thai cam bien
 * 4. Thu vien: adc

 */

#ifndef SOIL_H
#define SOIL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// He so lam min EMA (0.0 - 1.0). Cang nho cang min nhung phan ung cang cham.
#define SOIL_EMA_ALPHA        0.3f

// Vung raw coi la loi day. Cam bien that khong  ra sat 2 dau nay.
#define SOIL_RAW_FAULT_LOW    50
#define SOIL_RAW_FAULT_HIGH   4045

typedef enum {
    SOIL_OK = 0,
    SOIL_ERR_DISCONNECTED,   // raw sat 0 hoac sat 4095 -> nghi tuot day
    SOIL_ERR_NOT_CALIBRATED  // raw_dry == raw_wet -> chua hieu chuan
} soil_status_t;

typedef struct {
    uint8_t  pin;
    uint16_t raw_dry;        // raw khi de trong khong khis
    uint16_t raw_wet;        // raw khi nhung trong nuoc
    uint16_t raw;            // raw doc duoc lan cuoi (da loc 16 mau)
    float    percent;        // do am % sau khi qua EMA
    bool     initialized;    // da co lan doc dau tien chua (de nap EMA)
    const char *name;        // ten de in log
} soil_t;

// Khoi tao. raw_dry PHAI khac raw_wet, va thong thuong raw_dry > raw_wet.
void soil_setup(soil_t *dev, uint8_t pin, uint16_t raw_dry, uint16_t raw_wet,
                const char *name);

// Doc cam bien, cap nhat dev->raw va dev->percent.
// Khi tra ve loi, dev->percent giu gia tri cu (khong dung duoc).
soil_status_t soil_read(soil_t *dev);

// Doi 2 diem hieu chuan luc dang chay (dung khi hieu chuan qua dashboard)
void soil_calibrate(soil_t *dev, uint16_t raw_dry, uint16_t raw_wet);

const char *soil_status_str(soil_status_t status);

#ifdef __cplusplus
}
#endif

#endif // SOIL_H
