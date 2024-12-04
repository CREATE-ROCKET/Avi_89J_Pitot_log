#include "commonTask.h"
#include <Arduino.h>
#include "lib.h"
#include "debug.h"
#include "task_queue.h"

// Flashで書き込むために Data型をunit8_t[8]に変換する
union PitotDataUnion
{
    Data pitotData[numof_maxData];
    uint8_t Uint8Data[numof_maxData * 8];
};

namespace cmn_task
{
    int counter = 0;
    int retry_counter = 0;

    PitotDataUnion pitotData[numof_maxData];
    uint8_t *DataForFlash = pitotData->Uint8Data;
    void IRAM_ATTR distribute_data(void *pvParameter)
    {
        while (true)
        {
            if (xQueueReceive(PitotToDistributeQueue, &pitotData->pitotData[counter], 0) == pdTRUE)
            {
                ++counter;
                if (counter >= numof_maxData) // 一度に送信するタスク
                {
                    counter = 0;
#ifdef SD_FAST
                    xQueueSend(DistributeToSDQueue, &pitotData, 0);
                    vTaskResume(writeDataToSDTaskHandle);
#endif
#ifdef SPIFLASH
                    xQueueSend(DistributeToFlashQueue, DataForFlash, 0);
                    vTaskResume(writeDataToFlashTaskHandle);
#endif
                    vTaskSuspend(NULL);
                }
            }
            else
            {
                if (retry_counter < 50)
                {
                    ++retry_counter;
                    vTaskDelay(1);
                }
                else
                {
                    retry_counter = 0;
                    pr_debug("failed to receive PitotToDistributeQueue");
                    vTaskSuspend(NULL);
                }
            }
        }
    }
}