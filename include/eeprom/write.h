#ifndef __EEPROM_WRITE_H
#define __EEPROM_WRITE_H

#include <stdint.h>

#include "eeprom/utils.h"
#include "utils/gpio.h"
#include "utils/i2c.h"

void eeprom_write(mcu_utils_gpio_t pin_wp, uint8_t* buf, uint8_t offset, uint8_t len);

#endif
