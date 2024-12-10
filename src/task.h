#ifndef _common_task_
#define _common_task_

#include "lib.h"

namespace cmn_task
{
    void distribute_data(void *pvParameter);
#ifdef DEBUG
    void createData(void *pvParameter);
#endif
    void blinkLED_start(int led, int blinkInterval);
    void blinkLED_stop(int led);
}

#endif