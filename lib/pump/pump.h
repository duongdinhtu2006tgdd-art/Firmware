/* pump - dieu khien bom nuoc qua relay
 *
 * 1. Chuc nang: bat/tat bom, gioi han thoi gian chay, phat hien chay kho
 * 2. Input : chan relay, muc active, gioi han thoi gian, du lieu luu luong
 * 3. Output: trang thai bom, ly do dung, tong thoi gian chay
 * 4. Thu vien: gpio + timer
 */

#ifndef PUMP_H
#define PUMP_H

#include <stdint.h>
#include <stdbool.h>
#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PUMP_MAX_RUN_MS        180000UL   


#define PUMP_MIN_OFF_MS        30000UL    


#define PUMP_DRY_RUN_MS        5000UL

typedef enum {
    PUMP_STOP_NONE = 0,        
    PUMP_STOP_MANUAL,          
    PUMP_STOP_TARGET_REACHED,  
    PUMP_STOP_TIMEOUT,         
    PUMP_STOP_DRY_RUN          
} pump_stop_reason_t;

typedef struct {
    uint8_t  pin;
    bool     active_low;       
    bool     running;          

    oneshot_timer_t run_limit; 
    uint32_t start_ms;        
    uint32_t stop_ms;         
    uint32_t last_pulse_ms;   
    bool     ever_stopped;     

    float    target_liters;    
    float    liters_at_start;  

    pump_stop_reason_t last_stop_reason;
    uint32_t total_run_ms;     
    uint32_t dry_run_count;    
} pump_t;


void pump_setup(pump_t *dev, uint8_t pin, bool active_low);


bool pump_start(pump_t *dev, uint32_t max_run_ms, float target_liters,
                float current_liters);


void pump_stop(pump_t *dev, pump_stop_reason_t reason);


bool pump_update(pump_t *dev, float current_liters, bool flow_has_pulse);

bool  pump_is_running(const pump_t *dev);


float pump_session_liters(const pump_t *dev, float current_liters);


uint32_t pump_remaining_ms(const pump_t *dev);


bool pump_in_cooldown(const pump_t *dev);

const char *pump_stop_reason_str(pump_stop_reason_t reason);

#ifdef __cplusplus
}
#endif

#endif 
