#ifndef _my_task_
#define _my_task_

#include "lib.h"

namespace task
{
    void distribute_data(void *pvParameter);
#ifdef DEBUG
    void createData(void *pvParameter);
#endif
}

#endif