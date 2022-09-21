#ifndef _I2C_WRAP_H_
#define _I2C_WRAP_H_

#include <stdio.h>
#include "esp_err.h"
#include "driver/i2c.h"

esp_err_t i2c_init(int portnum, uint8_t sda_pin_num, uint8_t scl_pin_num, uint32_t spd_hz, uint8_t dev_addr_w);
int i2c_read(uint8_t *pbtRx, const size_t szRx, TickType_t ticks_to_wait);

int i2c_write(const uint8_t *pbtTx, const size_t szTx, TickType_t ticks_to_wait);

#endif