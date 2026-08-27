#include <Arduino.h>
#include "board_pins.h"
#include "gpio.h"
#include "adc.h"
#include "pwm.h"
#include "uart.h"
#include "timer.h"
#include "soil.h"
#include "dht22.h"
#include "flow.h"
#include "pump.h"
#include "lift.h"
#include "telemetry.h"

#define SOIL1_RAW_DRY    3200
#define SOIL1_RAW_WET    1300
#define SOIL2_RAW_DRY    3200
#define SOIL2_RAW_WET    1300


#define PUMP_DEFAULT_TARGET_L   2.0f

static soil_t  soil1;      // cam bien khu ca chua
static soil_t  soil2;      // cam bien khu xa lach
static dht22_t dht;
static flow_t  flow;
static pump_t  pump;
static lift_t  lift;

static periodic_timer_t t_sensor;     // doc cam bien + in log
static periodic_timer_t t_json;       // gui JSON cho dashboard
static periodic_timer_t t_heartbeat;  // nhay LED bao con song
static periodic_timer_t t_node1;      // Node 1 giai doan cay (node AI)


static volatile uint32_t g_flow_pulses   = 0;
static volatile uint32_t g_last_pulse_us = 0;

static void IRAM_ATTR flow_sensor_isr(void) {
    uint32_t now = micros();

    if (now - g_last_pulse_us < FLOW_MIN_PULSE_US) return;
    g_last_pulse_us = now;
    g_flow_pulses++;
}


static uint32_t take_flow_pulses(void) {
    noInterrupts();
    uint32_t pulses = g_flow_pulses;
    g_flow_pulses = 0;
    interrupts();
    return pulses;
}

static telemetry_data_t tele;
static char node1_line[UART_LINE_MAX];


static void update_sensors(void) {
    soil_status_t st1 = soil_read(&soil1);
    soil_status_t st2 = soil_read(&soil2);


    dht22_status_t st_dht = dht22_read(&dht);
    if (st_dht != DHT22_OK && st_dht != DHT22_ERR_TOO_SOON) {
        Serial.printf("[DHT22] loi: %s\n", dht22_status_str(st_dht));
    }

    tele.temp_c        = dht.temperature;
    tele.humidity_air  = dht.humidity;
    tele.dht_valid     = dht.valid;
    tele.dht_error_count = dht.error_count;

    tele.soil1_percent = soil1.percent;
    tele.soil1_raw     = soil1.raw;
    tele.soil1_valid   = (st1 == SOIL_OK);
    tele.soil2_percent = soil2.percent;
    tele.soil2_raw     = soil2.raw;
    tele.soil2_valid   = (st2 == SOIL_OK);

    if (st1 != SOIL_OK) Serial.printf("[SOIL1] loi: %s\n", soil_status_str(st1));
    if (st2 != SOIL_OK) Serial.printf("[SOIL2] loi: %s\n", soil_status_str(st2));

    tele.flow_lpm      = flow.rate_lpm;
    tele.total_liters  = flow.total_liters;

    tele.pump_running  = pump_is_running(&pump);
    tele.pump_session_liters = pump_session_liters(&pump, flow.total_liters);
    tele.pump_remaining_ms   = pump_remaining_ms(&pump);
    tele.pump_stop_reason    = pump_stop_reason_str(pump.last_stop_reason);
    tele.dry_run_count       = pump.dry_run_count;

    tele.lift_dir         = lift_dir_str(lift.dir);
    tele.lift_pos         = lift_pos_str(lift_position(&lift));
    tele.lift_stop_reason = lift_stop_reason_str(lift.last_stop_reason);
    tele.lift_current_v   = lift.last_current_v;

    tele.uptime_s = millis() / 1000;
}

static void handle_command(const telemetry_command_t *c) {
    const char *name = telemetry_cmd_str(c->cmd);

    switch (c->cmd) {
        case CMD_PUMP_ON: {
            float target = (c->value > 0.0f) ? c->value : PUMP_DEFAULT_TARGET_L;
            bool ok = pump_start(&pump, 0, target, flow.total_liters);
            telemetry_send_ack(name, ok,
                ok ? "" : (pump_in_cooldown(&pump) ? "dang trong thoi gian nghi"
                                                   : "bom dang chay"));
            break;
        }

        case CMD_PUMP_OFF:
            pump_stop(&pump, PUMP_STOP_MANUAL);
            telemetry_send_ack(name, true, "");
            break;

        case CMD_LIFT_UP: {
            bool ok = lift_move(&lift, LIFT_DIR_UP, 0);
            telemetry_send_ack(name, ok, ok ? "" : "bi tu choi (limit/dead-time)");
            break;
        }

        case CMD_LIFT_DOWN: {
            bool ok = lift_move(&lift, LIFT_DIR_DOWN, 0);
            telemetry_send_ack(name, ok, ok ? "" : "bi tu choi (limit/dead-time)");
            break;
        }

        case CMD_LIFT_STOP:
            lift_stop(&lift, LIFT_STOP_MANUAL);
            telemetry_send_ack(name, true, "");
            break;


        case CMD_CAL_DRY: {
            soil_t *s = (c->index == 2) ? &soil2 : &soil1;
            Serial.printf("[CAL] cam bien %u: raw KHO = %u  "
                          "-> dien so nay vao SOIL%u_RAW_DRY\n",
                          c->index, s->raw, c->index);
            telemetry_send_ack(name, true, "");
            break;
        }

        case CMD_CAL_WET: {
            soil_t *s = (c->index == 2) ? &soil2 : &soil1;
            Serial.printf("[CAL] cam bien %u: raw UOT = %u  "
                          "-> dien so nay vao SOIL%u_RAW_WET\n",
                          c->index, s->raw, c->index);
            telemetry_send_ack(name, true, "");
            break;
        }

        case CMD_STATUS:
            update_sensors();
            telemetry_send(&tele);
            break;

        default:
            telemetry_send_ack("UNKNOWN", false, "lenh khong hop le");
            break;
    }
}

void setup() {
    telemetry_setup(115200);
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Node 2 - he tuoi thong minh ===");

    timer_setup();

    gpio_setup(PIN_STATUS_LED, PIN_MODE_OUTPUT);


    soil_setup(&soil1, PIN_SOIL_TOMATO,  SOIL1_RAW_DRY, SOIL1_RAW_WET, "tomato");
    soil_setup(&soil2, PIN_SOIL_LETTUCE, SOIL2_RAW_DRY, SOIL2_RAW_WET, "lettuce");


    dht22_setup(&dht, PIN_DHT22);

    // --- Cam bien luu luong ---
    flow_setup(&flow, PIN_FLOW_SENSOR, FLOW_PULSES_PER_LITER);
    gpio_setup(PIN_FLOW_SENSOR, PIN_MODE_INPUT_PULLUP);
    gpio_attach_interrupt(PIN_FLOW_SENSOR, flow_sensor_isr, GPIO_EDGE_RISING);


    pump_setup(&pump, PIN_RELAY_PUMP, RELAY_ACTIVE_LOW ? true : false);

    // --- Dong co nang ha (BTS7960) ---
    bool lift_ok = lift_setup(&lift,
                              PIN_MOTOR_RPWM, PIN_MOTOR_LPWM,
                              PIN_MOTOR_R_EN, PIN_MOTOR_L_EN,
                              MOTOR_PWM_CH_R, MOTOR_PWM_CH_L,
                              MOTOR_PWM_FREQ_HZ, MOTOR_PWM_RES_BITS,
                              PIN_LIMIT_UP, PIN_LIMIT_DOWN,
                              PIN_MOTOR_R_IS, PIN_MOTOR_L_IS);
    Serial.printf("[LIFT] setup: %s   vi tri hien tai: %s\n",
                  lift_ok ? "OK" : "FAIL (kiem tra tham so PWM)",
                  lift_pos_str(lift_position(&lift)));

 
    uart_setup(UART2_BAUDRATE, PIN_UART2_RX, PIN_UART2_TX);
    uart_send_line("{\"node\":2,\"status\":\"boot\"}");


    periodic_timer_start(&t_sensor,    2000);   // = chu ky toi thieu cua DHT22
    periodic_timer_start(&t_json,      1000);
    periodic_timer_start(&t_heartbeat,  500);
    periodic_timer_start(&t_node1,     5000);

    Serial.println("Lenh: PUMP_ON[:lit] PUMP_OFF LIFT_UP LIFT_DOWN LIFT_STOP");
    Serial.println("      CAL_DRY:1 CAL_WET:1 STATUS");
    Serial.println("San sang.\n");
}

void loop() {

    uint32_t pulses = take_flow_pulses();
    flow_update(&flow, pulses);

    if (pump_update(&pump, flow.total_liters, pulses > 0)) {
        Serial.printf("[PUMP] tu dong dung: %s\n",
                      pump_stop_reason_str(pump.last_stop_reason));
    }

    if (lift_update(&lift)) {
        Serial.printf("[LIFT] tu dong dung: %s  (vi tri %s)\n",
                      lift_stop_reason_str(lift.last_stop_reason),
                      lift_pos_str(lift_position(&lift)));
    }


    telemetry_command_t cmd;
    if (telemetry_poll_command(&cmd)) {
        handle_command(&cmd);
    }


    uint16_t len = 0;
    switch (uart_read_line(node1_line, sizeof(node1_line), &len)) {
        case UART_RX_LINE_READY:
            Serial.printf("[NODE1] %s\n", node1_line);
            break;
        case UART_RX_OVERFLOW:
            Serial.println("[NODE1] loi: ban tin dai qua gioi han, da bo");
            break;
        case UART_RX_TIMEOUT:
            Serial.println("[NODE1] loi: qua han cho ky tu ket dong, da bo");
            break;
        case UART_RX_IDLE:
        default:
            break;
    }


    if (periodic_timer_expired(&t_heartbeat)) {
        gpio_toggle(PIN_STATUS_LED);
    }

    if (periodic_timer_expired(&t_sensor)) {
        update_sensors();
        telemetry_log(&tele);
    }

    if (periodic_timer_expired(&t_json)) {
        telemetry_send(&tele);
    }

    if (periodic_timer_expired(&t_node1)) {
        uart_send_line("{\"node\":2,\"req\":\"stage\"}");
    }
}
