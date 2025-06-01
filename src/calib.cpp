#include "calib.h"

void get_ofcal_calib_data(ads1262_ctl_pin_t ctl_pin, ads1262_reg_inpmux_t* inpmux, ads1262_reg_ofcal_t* ofcal_data) {
    ads1262_reg_ofcal_t ofcal = ads1262_reg_new_ofcal();
    ofcal.ofcal_0 = 0;
    ofcal.ofcal_1 = 0;
    ofcal.ofcal_2 = 0;
    ads1262_reg_set_ofcal(&ofcal);
    ads1262_reg_set_inpmux(inpmux);

    ads1262_cmd_start(ctl_pin);

    ads1262_cmd_rdata_t rdata;
    float data[100];
    for (uint8_t i = 0; i < sizeof(data) / sizeof(float); i++) {
        ads1262_wait(ctl_pin);
        ads1262_cmd_rdata(ctl_pin, &rdata, ADS1262_INIT_CONTROL_TYPE_HARD);
        data[i] = rdata.data;
    }

    ads1262_cmd_stop(ctl_pin);

    float sum = 0.0f;
    for (uint8_t i = 0; i < sizeof(data) / sizeof(float); i++) {
        sum += data[i];
    }
    float avg = sum / (sizeof(data) / sizeof(float));
    int32_t avg_int = (int32_t)(avg);
    uint32_t avg_high24 = ((uint32_t)avg_int >> 8) & 0xFFFFFF;

    ofcal_data->ofcal_0 = avg_high24 & 0xFF;
    ofcal_data->ofcal_1 = (avg_high24 >> 8) & 0xFF;
    ofcal_data->ofcal_2 = (avg_high24 >> 16) & 0xFF;
}
