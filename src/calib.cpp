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
    float avg_list[50] = {0};

    for (uint8_t round = 0; round < 50; round++) {
        float sum = 0.0f;

        for (uint8_t i = 0; i < 100; i++) {
            ads1262_wait(ctl_pin);
            ads1262_cmd_rdata(ctl_pin, &rdata, ADS1262_INIT_CONTROL_TYPE_HARD);
            sum += rdata.data;
        }

        avg_list[round] = sum / 100.0f;
    }

    ads1262_cmd_stop(ctl_pin);

    float grand_sum = 0.0f;
    for (uint8_t i = 0; i < 50; i++) {
        grand_sum += avg_list[i];
    }
    float final_avg = grand_sum / 50.0f;

    int32_t avg_int = (int32_t)((final_avg >= 0) ? (final_avg + 250.0f) : (final_avg - 250.0f));
    avg_int = (avg_int / 500) * 500;

    uint32_t avg_high24 = ((uint32_t)avg_int >> 8) & 0xFFFFFF;

    ofcal_data->ofcal_0 = avg_high24 & 0xFF;
    ofcal_data->ofcal_1 = (avg_high24 >> 8) & 0xFF;
    ofcal_data->ofcal_2 = (avg_high24 >> 16) & 0xFF;
}
