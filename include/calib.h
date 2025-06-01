#ifndef __CALIB_H
#define __CALIB_H

#include <stdint.h>

#include "ads1262/cmds/rdata.h"
#include "ads1262/cmds/start.h"
#include "ads1262/cmds/stop.h"
#include "ads1262/regs/fscal.h"
#include "ads1262/regs/inpmux.h"
#include "ads1262/regs/ofcal.h"

void get_ofcal_calib_data(ads1262_ctl_pin_t ctl_pin, ads1262_reg_inpmux_t* inpmux, ads1262_reg_ofcal_t* ofcal_data);

#endif
