#include "flash.h"
#include <lib.h>
#include "debug.h"
#include "S25FL127S 1.0.0/src/SPIflash.h"
#include <Arduino.h>
#include "task_queue.h"

#if !defined(DEBUG) || defined(SPIFLASH)

#define SPIFREQ 5000000

SPICREATE::SPICreate SPIC1;
Flash flash1;

namespace flash
{
    // setup内で実行 エラー処理なし
    int init()
    {
        digitalWrite(CS, HIGH);
        SPIC1.begin(CS, CLK, MISO, MOSI);
        flash1.begin(&SPIC1, CS, SPIFREQ);
#ifdef DEBUG
        flash1.erase();
#endif
        return 0;
    }

    void IRAM_ATTR writeDataToFlash(void *pvParameter)
    {
        int counter = 0;
        while (true)
        {
            uint8_t pitotData[256];
            if (xQueueReceive(DistributeToFlashQueue, &pitotData, 0) == pdTRUE)
            {
                flash1.write(counter, pitotData);
                counter += 0x100;
                vTaskSuspend(NULL);
            }
            else
            {
                if (counter < 50)
                {
                    ++counter;
                    vTaskDelay(1);
                }
                else
                {
                    counter = 0;
                    pr_debug("failed to receive DistributeToFlashQueue");
                    vTaskSuspend(NULL);
                }
            }
        }
    }
}

#endif