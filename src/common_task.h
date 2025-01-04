#ifndef _common_task_
#define _common_task_

#include "lib.h"

namespace cmn_task
{
    void blinkLED_start(int led, int blinkInterval);
    void blinkLED_stop(int led);
    char* DataToChar(Data pitotData[numof_maxData]);
}
#endif