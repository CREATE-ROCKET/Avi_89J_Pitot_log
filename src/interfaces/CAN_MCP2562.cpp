#include "CAN_MCP2562.h"
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include "Arduino.h"

#if !defined(DEBUG) || defined(CAN_MCP2562)

#include <CAN.h>

CAN_CREATE CAN(true);

namespace can
{
    int init()
    {
        if (CAN.begin(1000E3, 17, 18, 10))
        {
            pr_debug("Start Can failed");
            return 1; // CAN.begin() failed
        }

        return 0;
    }

    IRAM_ATTR void sendDataByCAN(void *pvParameter)
    {
        for (;;)
        {
            uint8_t *tmp = nullptr;
            if (xQueueReceive(DistributeToCanQueue, &tmp, portMAX_DELAY) == pdTRUE)
            {
                for (int i = 0; i < sizeof(Data) / sizeof(char); i++)
                {
                    if (i < 4)
                        continue;;
                    CAN.sendData(tmp + i, 8);
                }
            }
        }
    }
}

#endif