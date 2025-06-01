#ifndef __ANYSHAKE_MCU_UTILS_I2C_H
#define __ANYSHAKE_MCU_UTILS_I2C_H

#include <Wire.h>
#include <stdbool.h>
#include <stdint.h>

#include "utils/delay.h"
#include "utils/i2c.h"

#define MCU_UTILS_I2C_REG_WIDTH_8 1
#define MCU_UTILS_I2C_REG_WIDTH_16 2

void mcu_utils_i2c_init(bool is_rtos);
void mcu_utils_i2c_end(void);
void mcu_utils_i2c_read(uint8_t address,
                        uint16_t reg,
                        uint8_t* rx_data,
                        uint16_t rx_len,
                        uint8_t reg_width);
void mcu_utils_i2c_write(uint8_t address,
                         uint16_t reg,
                         uint8_t* tx_data,
                         uint16_t tx_len,
                         uint8_t reg_width);

#endif
