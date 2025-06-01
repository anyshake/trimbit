#include "utils/i2c.h"

void mcu_utils_i2c_init(bool is_rtos) {
    Wire.begin();
    Wire.setClock(400000);
    mcu_utils_delay_ms(100, is_rtos);
}

void mcu_utils_i2c_end(void) {
    Wire.end();
}

void mcu_utils_i2c_read(uint8_t address,
                        uint16_t reg,
                        uint8_t* rx_data,
                        uint16_t rx_len,
                        uint8_t reg_width) {
    Wire.beginTransmission(address);
    if (reg_width == MCU_UTILS_I2C_REG_WIDTH_16) {
        Wire.write((uint8_t)((reg >> 8) & 0xFF));
    }
    Wire.write((uint8_t)(reg & 0xFF));
    Wire.endTransmission(false);

    Wire.requestFrom((int)address, (int)rx_len);
    uint16_t index = 0;
    while (Wire.available() && index < rx_len) {
        rx_data[index++] = Wire.read();
    }
}

void mcu_utils_i2c_write(uint8_t address,
                         uint16_t reg,
                         uint8_t* tx_data,
                         uint16_t tx_len,
                         uint8_t reg_width) {
    Wire.beginTransmission(address);
    if (reg_width == MCU_UTILS_I2C_REG_WIDTH_16) {
        Wire.write((uint8_t)((reg >> 8) & 0xFF));
    }
    Wire.write((uint8_t)(reg & 0xFF));

    for (uint16_t i = 0; i < tx_len; ++i) {
        Wire.write(tx_data[i]);
    }

    Wire.endTransmission();
}
