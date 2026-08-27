/* telemetry - gui du lieu len dashboard va nhan lenh dieu khien
 *
 * 1. Chuc nang: dong goi so lieu thanh JSON gui qua Serial, phan tich lenh nhan ve
 * 2. Input : gia tri cam bien, trang thai bom/nang ha; chuoi lenh tu dashboard
 * 3. Output: 1 dong JSON tren Serial; lenh da giai ma (kem tham so)
 * 4. Thu vien: Serial (UART0 - cung cong USB dung de nap code)
 */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TELEMETRY_CMD_MAX   64

typedef enum {
    CMD_NONE = 0,
    CMD_PUMP_ON,
    CMD_PUMP_OFF,
    CMD_LIFT_UP,
    CMD_LIFT_DOWN,
    CMD_LIFT_STOP,
    CMD_CAL_DRY,
    CMD_CAL_WET,
    CMD_STATUS,
    CMD_UNKNOWN
} telemetry_cmd_t;


typedef struct {
    telemetry_cmd_t cmd;
    float           value;      
    uint8_t         index;      
} telemetry_command_t;


typedef struct {

    float temp_c;
    float humidity_air;
    float soil1_percent;
    float soil2_percent;
    uint16_t soil1_raw;
    uint16_t soil2_raw;
    float flow_lpm;
    float total_liters;


    bool  dht_valid;
    bool  soil1_valid;
    bool  soil2_valid;


    bool  pump_running;
    float pump_session_liters;
    uint32_t pump_remaining_ms;
    const char *pump_stop_reason;

    const char *lift_dir;
    const char *lift_pos;
    const char *lift_stop_reason;
    float lift_current_v;

 
    uint32_t uptime_s;
    uint32_t dht_error_count;
    uint32_t dry_run_count;
} telemetry_data_t;


void telemetry_setup(uint32_t baudrate);


void telemetry_send(const telemetry_data_t *data);


void telemetry_log(const telemetry_data_t *data);


bool telemetry_poll_command(telemetry_command_t *out);


void telemetry_send_ack(const char *cmd_name, bool accepted, const char *reason);

const char *telemetry_cmd_str(telemetry_cmd_t cmd);

#ifdef __cplusplus
}
#endif

#endif 
