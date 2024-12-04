#include "pitot.h"
#include "lib.h"
#include "debug.h"
#include <ms4525do.h>
#include <task_queue.h>

#if !defined(DEBUG) || defined(PITOT)

bfs::Ms4525do pres;

Data pitotData; //volatileにしなくて大丈夫かは要検証

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

        if (!pres.Begin()) {
            return 3; // pres.begin() failed
        }

        return 0; // success
    }

    void IRAM_ATTR getPitotDataTask(void *pvParameter) {
        while(true) {
            if (pres.Read()) {
                pitotData.pa = pres.pres_pa();
                pitotData.temp = pres.die_temp_c();
                xQueueSend(PitotToDistributeQueue, &pitotData, 0);
                vTaskResume(sendDataToEveryICTaskHandle);
            }
            vTaskDelay(10);
        }
    }
}

#endif