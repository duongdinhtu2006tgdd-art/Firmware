/* lift - dieu khien dong co nang ha gian tuoi (module BTS7960)
 *
 * 1. Chuc nang: nang/ha gian tuoi, dung tai cong tac hanh trinh, bao ve qua dong
 * 2. Input : chan RPWM/LPWM/R_EN/L_EN, 2 chan limit switch, 2 chan current sense
 * 3. Output: trang thai chuyen dong, vi tri (tren/duoi/giua), dong dien
 * 4. Thu vien: gpio + pwm + adc + timer
 */

#ifndef LIFT_H
#define LIFT_H

#include <stdint.h>
#include <stdbool.h>
#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif


#define LIFT_DEADTIME_MS      300


#define LIFT_RAMP_MS          500


#define LIFT_MAX_TRAVEL_MS    15000UL


#define LIFT_DEFAULT_DUTY     70

#define LIFT_OVERCURRENT_V    0.15f


#define LIFT_INRUSH_IGNORE_MS 800

typedef enum {
    LIFT_DIR_STOP = 0,
    LIFT_DIR_UP,       
    LIFT_DIR_DOWN     
} lift_dir_t;

typedef enum {
    LIFT_POS_UNKNOWN = 0,  
    LIFT_POS_TOP,          
    LIFT_POS_BOTTOM,       
    LIFT_POS_MIDDLE       
} lift_pos_t;

typedef enum {
    LIFT_STOP_NONE = 0,
    LIFT_STOP_MANUAL,        
    LIFT_STOP_LIMIT,         
    LIFT_STOP_TIMEOUT,       
    LIFT_STOP_OVERCURRENT, 
    LIFT_STOP_BLOCKED        
} lift_stop_reason_t;

typedef struct {
    uint8_t pin_rpwm, pin_lpwm;
    uint8_t pin_r_en, pin_l_en;
    uint8_t pin_limit_up, pin_limit_down;
    uint8_t pin_r_is,     pin_l_is;
    uint8_t ch_r, ch_l;          

    lift_dir_t dir;              
    uint8_t    target_duty;      
    uint8_t    current_duty;     

    uint32_t   move_start_ms;    
    uint32_t   deadtime_until_ms;
    lift_stop_reason_t last_stop_reason;

    float      last_current_v;   
    uint32_t   overcurrent_count;
} lift_t;


bool lift_setup(lift_t *dev,
                uint8_t pin_rpwm, uint8_t pin_lpwm,
                uint8_t pin_r_en, uint8_t pin_l_en,
                uint8_t ch_r, uint8_t ch_l,
                uint32_t pwm_freq_hz, uint8_t pwm_res_bits,
                uint8_t pin_limit_up, uint8_t pin_limit_down,
                uint8_t pin_r_is, uint8_t pin_l_is);


bool lift_move(lift_t *dev, lift_dir_t dir, uint8_t duty_percent);


void lift_stop(lift_t *dev, lift_stop_reason_t reason);


bool lift_update(lift_t *dev);


lift_pos_t lift_position(const lift_t *dev);

bool  lift_is_moving(const lift_t *dev);


float lift_current_voltage(const lift_t *dev);

const char *lift_dir_str(lift_dir_t dir);
const char *lift_pos_str(lift_pos_t pos);
const char *lift_stop_reason_str(lift_stop_reason_t reason);

#ifdef __cplusplus
}
#endif

#endif 
