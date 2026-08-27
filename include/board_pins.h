#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include <stdint.h>


#define PIN_UART2_RX          16
#define PIN_UART2_TX          17
#define UART2_BAUDRATE        115200


#define PIN_SOIL_TOMATO       34   
#define PIN_SOIL_LETTUCE      35   


#define PIN_DHT22             25

// YF-S201 luu luong nuoc (ngo ra xung, doc bang ngat) 
// Cam bien cap +5V nhung ngo ra open-collector: PHAI pull-up len 3.3V
// (khong phai 5V) de khong vuot muc logic cua ESP32.
#define PIN_FLOW_SENSOR       27


#define PIN_RELAY_PUMP        26
#define RELAY_ACTIVE_LOW      1


#define PIN_MOTOR_RPWM        18
#define PIN_MOTOR_LPWM        19
#define PIN_MOTOR_R_EN        21
#define PIN_MOTOR_L_EN        22


#define PIN_MOTOR_R_IS        32  
#define PIN_MOTOR_L_IS        33   


#define MOTOR_PWM_CH_R        0
#define MOTOR_PWM_CH_L        2
#define MOTOR_PWM_FREQ_HZ     10000
#define MOTOR_PWM_RES_BITS    10

#define PIN_LIMIT_UP          13
#define PIN_LIMIT_DOWN        14


#define PIN_STATUS_LED        2

#endif 
