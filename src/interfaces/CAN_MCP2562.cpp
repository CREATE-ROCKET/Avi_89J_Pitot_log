#include "CAN_MCP2562.h"
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include "Arduino.h"

#if !defined(DEBUG) || defined(CAN_MCP2562)

#include <CANCREATE.h>

CAN_CREATE CAN(true);

namespace can
{
    int init()
    {
        can_setting_t settings;
        settings.baudRate = 100E3;
        settings.multiData_send = true;
        settings.filter_config = CAN_FILTER_DEFAULT;
        if (CAN.begin(settings, RX, TX, 2, SELECT))
        {
            pr_debug("Start Can failed");
            return 1; // CAN.begin() failed
        }

        return 0;
    }

    // pitotのデータがnumof_maxData * 10μsの間隔で送られてくるため、それを送信する
    IRAM_ATTR void sendDataByCAN(void *pvParameter)
    {
        for (;;)
        {
            uint8_t *tmp = nullptr;
            if (xQueueReceive(DistributeToCanQueue, &tmp, portMAX_DELAY) == pdTRUE)
            {
                CAN.sendChar(2, '<');
                for (int i = 0; i < sizeof(Data) / sizeof(uint8_t); i++)
                    CAN.sendData(2, tmp + i * 8, 8);
                CAN.sendChar(2, '>');
                delay(10);
            }
        }
    }
}

#endif