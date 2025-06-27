#include <Arduino.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "utils/delay.h"
#include "utils/gpio.h"
#include "utils/i2c.h"
#include "utils/led.h"
#include "utils/spi.h"
#include "utils/uart.h"

#include "ads1262/cmds/stop.h"
#include "ads1262/regs/inpmux.h"
#include "ads1262/regs/interface.h"
#include "ads1262/regs/mode_0.h"
#include "ads1262/regs/mode_2.h"
#include "ads1262/regs/ofcal.h"

#include "ssd1306/display.h"
#include "ssd1306/font.h"
#include "ssd1306/utils.h"

#include "eeprom/read.h"
#include "eeprom/utils.h"
#include "eeprom/write.h"

#include "calib.h"
#include "settings.h"

#define EEPROM_OFFSET_DEVICE_ID 0
#define EEPROM_OFFSET_CALIB_STATUS 4
#define EEPROM_OFFSET_OFCAL_CHANNEL_1 5
#define EEPROM_OFFSET_FSCAL_CHANNEL_1 8
#define EEPROM_OFFSET_OFCAL_CHANNEL_2 11
#define EEPROM_OFFSET_FSCAL_CHANNEL_2 14
#define EEPROM_OFFSET_OFCAL_CHANNEL_3 17
#define EEPROM_OFFSET_FSCAL_CHANNEL_3 20

typedef struct {
    uint8_t calib_status;
    uint8_t channel_1[3];
    uint8_t channel_2[3];
    uint8_t channel_3[3];
} adc_calibration_data_t;

void peri_init(void) {
    mcu_utils_gpio_init(false);
    mcu_utils_gpio_mode(MCU_BOOT1_PIN, MCU_UTILS_GPIO_MODE_INPUT);
    mcu_utils_gpio_mode(OPTIONS_B1_PIN, MCU_UTILS_GPIO_MODE_INPUT);
    mcu_utils_gpio_mode(OPTIONS_B2_PIN, MCU_UTILS_GPIO_MODE_INPUT);
    mcu_utils_gpio_mode(OPTIONS_B3_PIN, MCU_UTILS_GPIO_MODE_INPUT);
    mcu_utils_gpio_mode(MCU_STATE_PIN, MCU_UTILS_GPIO_MODE_OUTPUT);

    mcu_utils_i2c_init(false);
    mcu_utils_spi_init(false);
    mcu_utils_uart_init(9600, false);

    ads1262_init(ADS1262_CTL_PIN, ADS1262_INIT_CONTROL_TYPE_HARD);
    ads1262_reset(ADS1262_CTL_PIN, ADS1262_RESET_RESET_TYPE_HARD, false);
    ads1262_reg_interface_t ads1262_reg_interface = ads1262_reg_new_interface();
    ads1262_reg_interface.status = ADS1262_REG_INTERFACE_STATUS_ENABLED;
    ads1262_reg_interface.crc = ADS1262_REG_INTERFACE_CRC_CRC;
    ads1262_reg_set_interface(&ads1262_reg_interface);
    ads1262_reg_mode_0_t ads1262_reg_mode_0 = ads1262_reg_new_mode_0();
    ads1262_reg_mode_0.run_mode = ADS1262_REG_MODE_0_RUN_MODE_CONTINUOUS;
    ads1262_reg_set_mode_0(&ads1262_reg_mode_0);
    ads1262_reg_mode_2_t ads1262_reg_mode_2 = ads1262_reg_new_mode_2();
    ads1262_reg_mode_2.dr = ADS1262_REG_MODE_2_DR_1200;
    ads1262_reg_set_mode_2(&ads1262_reg_mode_2);

    ssd1306_init();
    ssd1306_enable();

    eeprom_init(EEPROM_WP_PIN);
}

void welcome_screen(void) {
    ssd1306_clear();
    ssd1306_display_bitmap(0, 0, 128, 8, ANYSHAKE_LOGO_BITMAP, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 0, "Trimbit Init...", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    mcu_utils_led_blink(MCU_STATE_PIN, 5, false);
    mcu_utils_delay_ms(1000, false);
}

void handle_check_calib_data(void) {
    uint8_t calib_factors_status = 0;
    eeprom_read((uint8_t*)&calib_factors_status, EEPROM_OFFSET_CALIB_STATUS, sizeof(calib_factors_status));
    if (calib_factors_status == 1) {
        ssd1306_display_string(0, 0, "> Calib Data Exists", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
        ssd1306_display_string(0, 1, "> Continue in 10 sec", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
        mcu_utils_delay_ms(5000, false);
    }
}

void handle_short_inputs(void) {
    ssd1306_clear();
    ssd1306_display_string(0, 0, "Step 1/6:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "Short channel inputs,", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 5, "Set BOOT1 to right", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    while (!mcu_utils_gpio_read(MCU_BOOT1_PIN)) {
        ;
    }

    mcu_utils_led_blink(MCU_STATE_PIN, 5, false);
    mcu_utils_delay_ms(500, false);
}

void handle_set_dip_switch(void) {
    ssd1306_clear();
    ssd1306_display_string(0, 0, "Step 2/6:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "Dip switch OPTIONS", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 5, "Set all bits to 1", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    while (!digitalRead(OPTIONS_B1_PIN.pin) || !digitalRead(OPTIONS_B2_PIN.pin) || !digitalRead(OPTIONS_B3_PIN.pin)) {
        ;
    }

    mcu_utils_led_blink(MCU_STATE_PIN, 5, false);
    mcu_utils_delay_ms(500, false);
}

void handle_calibrate_channel_1(uint8_t calib_data[3]) {
    ssd1306_clear();
    ssd1306_display_string(0, 0, "Step 3/6:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "DO NOT TOUCH DEVICE", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 5, "Calibrating EHZ...", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    ads1262_reg_inpmux_t ads1262_reg_inpmux = ads1262_reg_new_inpmux();
    ads1262_reg_inpmux.mux_p = ADS1262_REG_INPMUX_AIN0;
    ads1262_reg_inpmux.mux_n = ADS1262_REG_INPMUX_AIN1;
    ads1262_reg_ofcal_t ads1262_reg_ofcal = ads1262_reg_new_ofcal();
    get_ofcal_calib_data(ADS1262_CTL_PIN, &ads1262_reg_inpmux, &ads1262_reg_ofcal);

    ssd1306_clear();
    ssd1306_display_string(0, 0, "INFO:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "Complete in 10 sec!", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 5, "Set OPTIONS B1 to 0", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    while (mcu_utils_gpio_read(OPTIONS_B1_PIN)) {
        ;
    }

    calib_data[0] = ads1262_reg_ofcal.ofcal_0;
    calib_data[1] = ads1262_reg_ofcal.ofcal_1;
    calib_data[2] = ads1262_reg_ofcal.ofcal_2;

    mcu_utils_led_blink(MCU_STATE_PIN, 5, false);
    mcu_utils_delay_ms(500, false);
}

void handle_calibrate_channel_2(uint8_t calib_data[3]) {
    ssd1306_clear();
    ssd1306_display_string(0, 0, "Step 4/6:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "DO NOT TOUCH DEVICE", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 5, "Calibrating EHE...", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    ads1262_reg_inpmux_t ads1262_reg_inpmux = ads1262_reg_new_inpmux();
    ads1262_reg_inpmux.mux_p = ADS1262_REG_INPMUX_AIN2;
    ads1262_reg_inpmux.mux_n = ADS1262_REG_INPMUX_AIN3;
    ads1262_reg_ofcal_t ads1262_reg_ofcal = ads1262_reg_new_ofcal();
    get_ofcal_calib_data(ADS1262_CTL_PIN, &ads1262_reg_inpmux, &ads1262_reg_ofcal);

    ssd1306_clear();
    ssd1306_display_string(0, 0, "INFO:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "Complete in 10 sec!", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 5, "Set OPTIONS B2 to 0", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    while (mcu_utils_gpio_read(OPTIONS_B2_PIN)) {
        ;
    }

    calib_data[0] = ads1262_reg_ofcal.ofcal_0;
    calib_data[1] = ads1262_reg_ofcal.ofcal_1;
    calib_data[2] = ads1262_reg_ofcal.ofcal_2;

    mcu_utils_led_blink(MCU_STATE_PIN, 5, false);
    mcu_utils_delay_ms(500, false);
}

void handle_calibrate_channel_3(uint8_t calib_data[3]) {
    ssd1306_clear();
    ssd1306_display_string(0, 0, "Step 5/6:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "DO NOT TOUCH DEVICE", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 5, "Calibrating EHN...", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    ads1262_reg_inpmux_t ads1262_reg_inpmux = ads1262_reg_new_inpmux();
    ads1262_reg_inpmux.mux_p = ADS1262_REG_INPMUX_AIN4;
    ads1262_reg_inpmux.mux_n = ADS1262_REG_INPMUX_AIN5;
    ads1262_reg_ofcal_t ads1262_reg_ofcal = ads1262_reg_new_ofcal();
    get_ofcal_calib_data(ADS1262_CTL_PIN, &ads1262_reg_inpmux, &ads1262_reg_ofcal);

    ssd1306_clear();
    ssd1306_display_string(0, 0, "INFO:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "Complete in 10 sec!", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 5, "Set OPTIONS B3 to 0", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    while (mcu_utils_gpio_read(OPTIONS_B3_PIN)) {
        ;
    }

    calib_data[0] = ads1262_reg_ofcal.ofcal_0;
    calib_data[1] = ads1262_reg_ofcal.ofcal_1;
    calib_data[2] = ads1262_reg_ofcal.ofcal_2;

    mcu_utils_led_blink(MCU_STATE_PIN, 5, false);
    mcu_utils_delay_ms(500, false);
}

void handle_apply_calibration_data(adc_calibration_data_t* calibration_data) {
    ssd1306_clear();
    ssd1306_display_string(0, 0, "Step 6/6:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "Set BOOT1 to left", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 5, "To apply new data", SSD1306_FONT_TYPE_ASCII_8X6, SSD1306_FONT_DISPLAY_COLOR_WHITE);

    while (mcu_utils_gpio_read(MCU_BOOT1_PIN)) {
        ;
    }

    eeprom_write(EEPROM_WP_PIN, calibration_data->channel_1, EEPROM_OFFSET_OFCAL_CHANNEL_1, sizeof(calibration_data->channel_1));
    mcu_utils_delay_ms(500, false);

    eeprom_write(EEPROM_WP_PIN, calibration_data->channel_2, EEPROM_OFFSET_OFCAL_CHANNEL_2, sizeof(calibration_data->channel_2));
    mcu_utils_delay_ms(500, false);

    eeprom_write(EEPROM_WP_PIN, calibration_data->channel_3, EEPROM_OFFSET_OFCAL_CHANNEL_3, sizeof(calibration_data->channel_3));
    mcu_utils_delay_ms(500, false);

    calibration_data->calib_status = 1;
    eeprom_write(EEPROM_WP_PIN, &calibration_data->calib_status, EEPROM_OFFSET_CALIB_STATUS, sizeof(calibration_data->calib_status));
    mcu_utils_delay_ms(500, false);

    mcu_utils_led_blink(MCU_STATE_PIN, 5, false);
    mcu_utils_delay_ms(500, false);

    ssd1306_clear();
    ssd1306_display_string(0, 0, "INFO:", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
    ssd1306_display_string(0, 4, "All Complete!", SSD1306_FONT_TYPE_ASCII_8X16, SSD1306_FONT_DISPLAY_COLOR_WHITE);
}

void setup(void) {
    peri_init();
    welcome_screen();

    handle_check_calib_data();
    handle_short_inputs();
    handle_set_dip_switch();

    adc_calibration_data_t calibration_data = {
        .calib_status = 0,
        .channel_1 = {0, 0, 0},
        .channel_2 = {0, 0, 0},
        .channel_3 = {0, 0, 0},
    };
    handle_calibrate_channel_1(calibration_data.channel_1);
    handle_calibrate_channel_2(calibration_data.channel_2);
    handle_calibrate_channel_3(calibration_data.channel_3);

    handle_apply_calibration_data(&calibration_data);
}

void loop(void) {
    mcu_utils_gpio_high(MCU_STATE_PIN);
    mcu_utils_delay_ms(1000, false);
    mcu_utils_gpio_low(MCU_STATE_PIN);
    mcu_utils_delay_ms(1000, false);

    uint32_t device_id = 0;
    eeprom_read((uint8_t*)&device_id, EEPROM_OFFSET_DEVICE_ID, sizeof(device_id));
    Serial.print("Device ID: ");
    Serial.println(device_id, HEX);

    uint8_t calib_data[3];
    eeprom_read((uint8_t*)&calib_data, EEPROM_OFFSET_OFCAL_CHANNEL_1, sizeof(calib_data));
    Serial.print("Calib Data Channel 1: ");
    Serial.print(calib_data[0], HEX);
    Serial.print(" ");
    Serial.print(calib_data[1], HEX);
    Serial.print(" ");
    Serial.println(calib_data[2], HEX);

    eeprom_read((uint8_t*)&calib_data, EEPROM_OFFSET_OFCAL_CHANNEL_2, sizeof(calib_data));
    Serial.print("Calib Data Channel 2: ");
    Serial.print(calib_data[0], HEX);
    Serial.print(" ");
    Serial.print(calib_data[1], HEX);
    Serial.print(" ");
    Serial.println(calib_data[2], HEX);

    eeprom_read((uint8_t*)&calib_data, EEPROM_OFFSET_OFCAL_CHANNEL_3, sizeof(calib_data));
    Serial.print("Calib Data Channel 3: ");
    Serial.print(calib_data[0], HEX);
    Serial.print(" ");
    Serial.print(calib_data[1], HEX);
    Serial.print(" ");
    Serial.println(calib_data[2], HEX);
}
