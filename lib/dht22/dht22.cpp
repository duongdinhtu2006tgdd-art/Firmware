#include "dht22.h"
#include <Arduino.h>

#define WAIT_TIMEOUT_US   200

#define BIT_THRESHOLD_US  50


#define TEMP_MIN  (-40.0f)
#define TEMP_MAX  ( 80.0f)
#define HUM_MIN   (  0.0f)
#define HUM_MAX   (100.0f)


static uint32_t wait_while_level(uint8_t pin, uint8_t level) {
    uint32_t start = micros();
    while (digitalRead(pin) == level) {
        if (micros() - start > WAIT_TIMEOUT_US) return 0;
    }
    uint32_t elapsed = micros() - start;
    return (elapsed == 0) ? 1 : elapsed;  
}

static dht22_status_t read_raw(uint8_t pin, uint8_t out[5]) {
    uint8_t data[5] = {0, 0, 0, 0, 0};


    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(2);                  
 
    pinMode(pin, INPUT_PULLUP);

    noInterrupts();


    if (wait_while_level(pin, HIGH) == 0) { interrupts(); return DHT22_ERR_TIMEOUT; }
    if (wait_while_level(pin, LOW)  == 0) { interrupts(); return DHT22_ERR_TIMEOUT; }
    if (wait_while_level(pin, HIGH) == 0) { interrupts(); return DHT22_ERR_TIMEOUT; }


    for (uint8_t i = 0; i < 40; i++) {

        if (wait_while_level(pin, LOW) == 0) { interrupts(); return DHT22_ERR_TIMEOUT; }


        uint32_t high_us = wait_while_level(pin, HIGH);
        if (high_us == 0) { interrupts(); return DHT22_ERR_TIMEOUT; }


        data[i / 8] <<= 1;
        if (high_us > BIT_THRESHOLD_US) data[i / 8] |= 1;
    }

    interrupts();

    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return DHT22_ERR_CHECKSUM;

    for (uint8_t i = 0; i < 5; i++) out[i] = data[i];
    return DHT22_OK;
}

void dht22_setup(dht22_t *dev, uint8_t pin) {
    if (!dev) return;

    dev->pin          = pin;
    dev->temperature  = 0.0f;
    dev->humidity     = 0.0f;
    dev->valid        = false;
    dev->error_count  = 0;

    pinMode(pin, INPUT_PULLUP);


    dev->last_read_ms = millis() - DHT22_MIN_INTERVAL_MS;
}

dht22_status_t dht22_read(dht22_t *dev) {
    if (!dev) return DHT22_ERR_TIMEOUT;

    if (millis() - dev->last_read_ms < DHT22_MIN_INTERVAL_MS) {
        return DHT22_ERR_TOO_SOON;
    }

    uint8_t raw[5];
    dht22_status_t status = DHT22_ERR_TIMEOUT;

    for (uint8_t attempt = 0; attempt < DHT22_MAX_RETRY; attempt++) {
        status = read_raw(dev->pin, raw);
        if (status == DHT22_OK) break;

        delay(20);
    }

    if (status != DHT22_OK) {
        dev->error_count++;
        dev->last_read_ms = millis();  
        return status;
    }

    float humidity = (float)(((uint16_t)raw[0] << 8) | raw[1]) / 10.0f;


    uint16_t t_raw = ((uint16_t)(raw[2] & 0x7F) << 8) | raw[3];
    float temperature = (float)t_raw / 10.0f;
    if (raw[2] & 0x80) temperature = -temperature;


    if (humidity    < HUM_MIN  || humidity    > HUM_MAX ||
        temperature < TEMP_MIN || temperature > TEMP_MAX) {
        dev->error_count++;
        dev->last_read_ms = millis();
        return DHT22_ERR_RANGE;
    }

    dev->humidity     = humidity;
    dev->temperature  = temperature;
    dev->valid        = true;
    dev->last_read_ms = millis();
    return DHT22_OK;
}

const char *dht22_status_str(dht22_status_t status) {
    switch (status) {
        case DHT22_OK:            return "OK";
        case DHT22_ERR_TIMEOUT:   return "TIMEOUT (kiem tra day DATA va pull-up 4.7k)";
        case DHT22_ERR_CHECKSUM:  return "CHECKSUM sai (nhieu, day qua dai)";
        case DHT22_ERR_TOO_SOON:  return "TOO_SOON (chua du 2 giay)";
        case DHT22_ERR_RANGE:     return "RANGE (gia tri ngoai dai do)";
        default:                  return "UNKNOWN";
    }
}
