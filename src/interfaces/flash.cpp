#include "flash.h"
#include <lib.h>
#include "debug.h"
#include "S25FL127S 1.0.0/src/SPIflash.h"
#include <Arduino.h>
#include "task_queue.h"
#include "memory_controller.h"

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
            // Queueにデータがくるまで待つ
            if (xQueueReceive(DistributeToFlashQueue, &pitotData, portMAX_DELAY) == pdTRUE)
            {
                flash1.write(counter, pitotData);
                mem_controller::delete_ptr();
                counter += 0x100;
            }
            else
            {
                pr_debug("failed to receive DistributeToFlashQueue");
            }
        }
    }
}

#endif