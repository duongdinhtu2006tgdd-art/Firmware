#include "telemetry.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>


static char     s_cmd_buffer[TELEMETRY_CMD_MAX];
static uint16_t s_cmd_index = 0;

void telemetry_setup(uint32_t baudrate) {
    if (!Serial) Serial.begin(baudrate);
    s_cmd_index = 0;
}

void telemetry_send(const telemetry_data_t *d) {
    if (!d) return;


    Serial.printf(
        "{"
        "\"temp\":%.1f,\"hum\":%.1f,"
        "\"soil\":%.1f,\"soil2\":%.1f,"
        "\"soil_raw\":%u,\"soil2_raw\":%u,"
        "\"flow\":%.2f,\"liters\":%.3f,"
        "\"dht_ok\":%d,\"soil_ok\":%d,\"soil2_ok\":%d,"
        "\"pump\":\"%s\",\"pump_liters\":%.3f,\"pump_left_s\":%lu,"
        "\"pump_stop\":\"%s\","
        "\"lift\":\"%s\",\"lift_pos\":\"%s\",\"lift_stop\":\"%s\","
        "\"lift_amp_v\":%.3f,"
        "\"uptime\":%lu,\"dht_err\":%lu,\"dry_run\":%lu"
        "}\n",
        d->temp_c, d->humidity_air,
        d->soil1_percent, d->soil2_percent,
        d->soil1_raw, d->soil2_raw,
        d->flow_lpm, d->total_liters,
        d->dht_valid ? 1 : 0, d->soil1_valid ? 1 : 0, d->soil2_valid ? 1 : 0,
        d->pump_running ? "ON" : "OFF",
        d->pump_session_liters,
        (unsigned long)(d->pump_remaining_ms / 1000),
        d->pump_stop_reason ? d->pump_stop_reason : "NONE",
        d->lift_dir         ? d->lift_dir         : "STOP",
        d->lift_pos         ? d->lift_pos         : "UNKNOWN",
        d->lift_stop_reason ? d->lift_stop_reason : "NONE",
        d->lift_current_v,
        (unsigned long)d->uptime_s,
        (unsigned long)d->dht_error_count,
        (unsigned long)d->dry_run_count);
}

void telemetry_log(const telemetry_data_t *d) {
    if (!d) return;

    Serial.println("---------------------------------------------");

    if (d->dht_valid) {
        Serial.printf("[DHT22 ] %.1f C   %.1f %% RH\n", d->temp_c, d->humidity_air);
    } else {
        Serial.println("[DHT22 ] chua co so lieu hop le");
    }

    Serial.printf("[SOIL 1] %5.1f %%  raw=%4u  %s\n",
                  d->soil1_percent, d->soil1_raw, d->soil1_valid ? "OK" : "LOI");
    Serial.printf("[SOIL 2] %5.1f %%  raw=%4u  %s\n",
                  d->soil2_percent, d->soil2_raw, d->soil2_valid ? "OK" : "LOI");

    Serial.printf("[FLOW  ] %.2f L/min   tong %.3f L\n",
                  d->flow_lpm, d->total_liters);

    Serial.printf("[PUMP  ] %s", d->pump_running ? "DANG CHAY" : "TAT");
    if (d->pump_running) {
        Serial.printf("  phien nay %.3f L  con %lu s",
                      d->pump_session_liters,
                      (unsigned long)(d->pump_remaining_ms / 1000));
    } else if (d->pump_stop_reason) {
        Serial.printf("  (dung do: %s)", d->pump_stop_reason);
    }
    Serial.println();

    Serial.printf("[LIFT  ] %s  vi tri=%s  dong=%.3fV",
                  d->lift_dir ? d->lift_dir : "STOP",
                  d->lift_pos ? d->lift_pos : "UNKNOWN",
                  d->lift_current_v);
    if (d->lift_stop_reason && strcmp(d->lift_stop_reason, "NONE") != 0) {
        Serial.printf("  (dung do: %s)", d->lift_stop_reason);
    }
    Serial.println();

    Serial.printf("[SYS   ] uptime %lu s   loi DHT %lu   chay kho %lu\n",
                  (unsigned long)d->uptime_s,
                  (unsigned long)d->dht_error_count,
                  (unsigned long)d->dry_run_count);
}


static void parse_command(char *line, telemetry_command_t *out) {
    out->cmd   = CMD_UNKNOWN;
    out->value = 0.0f;
    out->index = 0;

    char *colon = strchr(line, ':');
    char *arg   = NULL;
    if (colon) {
        *colon = '\0';      
        arg = colon + 1;
    }

    if      (strcmp(line, "PUMP_ON")   == 0) out->cmd = CMD_PUMP_ON;
    else if (strcmp(line, "PUMP_OFF")  == 0) out->cmd = CMD_PUMP_OFF;
    else if (strcmp(line, "LIFT_UP")   == 0) out->cmd = CMD_LIFT_UP;
    else if (strcmp(line, "LIFT_DOWN") == 0) out->cmd = CMD_LIFT_DOWN;
    else if (strcmp(line, "LIFT_STOP") == 0) out->cmd = CMD_LIFT_STOP;
    else if (strcmp(line, "CAL_DRY")   == 0) out->cmd = CMD_CAL_DRY;
    else if (strcmp(line, "CAL_WET")   == 0) out->cmd = CMD_CAL_WET;
    else if (strcmp(line, "STATUS")    == 0) out->cmd = CMD_STATUS;

    if (!arg) return;

    if (out->cmd == CMD_PUMP_ON) {
        out->value = (float)atof(arg);
    } else if (out->cmd == CMD_CAL_DRY || out->cmd == CMD_CAL_WET) {
        int idx = atoi(arg);
        out->index = (idx > 0 && idx < 256) ? (uint8_t)idx : 1;
    }
}

bool telemetry_poll_command(telemetry_command_t *out) {
    if (!out) return false;

    while (Serial.available()) {
        char c = (char)Serial.read();

        if (c == '\r' || c == '\n') {
            if (s_cmd_index == 0) continue;  

            s_cmd_buffer[s_cmd_index] = '\0';
            s_cmd_index = 0;
            parse_command(s_cmd_buffer, out);
            return true;
        }

        if (s_cmd_index < TELEMETRY_CMD_MAX - 1) {
            s_cmd_buffer[s_cmd_index++] = c;
        } else {
            s_cmd_index = 0;   
        }
    }

    return false;
}

void telemetry_send_ack(const char *cmd_name, bool accepted, const char *reason) {
    Serial.printf("{\"ack\":\"%s\",\"ok\":%d,\"reason\":\"%s\"}\n",
                  cmd_name ? cmd_name : "?",
                  accepted ? 1 : 0,
                  reason ? reason : "");
}

const char *telemetry_cmd_str(telemetry_cmd_t cmd) {
    switch (cmd) {
        case CMD_PUMP_ON:   return "PUMP_ON";
        case CMD_PUMP_OFF:  return "PUMP_OFF";
        case CMD_LIFT_UP:   return "LIFT_UP";
        case CMD_LIFT_DOWN: return "LIFT_DOWN";
        case CMD_LIFT_STOP: return "LIFT_STOP";
        case CMD_CAL_DRY:   return "CAL_DRY";
        case CMD_CAL_WET:   return "CAL_WET";
        case CMD_STATUS:    return "STATUS";
        case CMD_NONE:      return "NONE";
        case CMD_UNKNOWN:
        default:            return "UNKNOWN";
    }
}
