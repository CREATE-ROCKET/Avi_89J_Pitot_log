#include "pitot.h"
#include "lib.h"
#include "debug.h"
#include <ms4525do.h>
#include "task_queue.h"
#include "memory_controller.h"

#if !defined(DEBUG) || defined(PITOT)

bfs::Ms4525do pres;

namespace pitot
{
    int init()
    {
        if (!Wire.begin())
        {
            return 1; // Wire.begin() failed
        }
        if (!Wire.setClock(400000))
        {
            return 2; // Wire.setClock() failed
        }

        // set pitot config
        pres.Config(&Wire, 0x28, 1.0f, -1.0f);

        if (!pres.Begin())
        {
            return 3; // pres.begin() failed
        }

        return 0; // success
    }

    void IRAM_ATTR getPitotData(void *pvParameter)
    {
        portTickType xLastWakeTime = xTaskGetTickCount();
        while (true)
        {
            if (pres.Read())
            {
                Data *pitotData = mem_controller::new_ptr();
                pitotData->pa = pres.pres_pa();
                pitotData->temp = pres.die_temp_c();
                if (xQueueSend(PitotToDistributeQueue, &pitotData, 0) != pdTRUE){
                    error_log("%s(%d) failed to send queue date:%lu Queue:%d",__FILE__, __LINE__, millis(), uxQueueMessagesWaiting(PitotToDistributeQueue));
                    delete pitotData;
                }
                vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS); // 10msごとに呼び出すことにする
            }
            else
            {
                vTaskDelay(1);
            }
        }
    }
}

#endif