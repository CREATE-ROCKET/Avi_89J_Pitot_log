#include "task.h"
#include <Arduino.h>
#include "lib.h"
#include "debug.h"
#include "task_queue.h"

namespace task
{
    // getPitotDataでnewされたデータを受け取って、
    // makeParity、 writeDataToFlash、 SendDataByCan に送信し、
    // すべてで利用されたあとdeleteする
    void IRAM_ATTR distribute_data(void *pvParameter)
    {
        Data *pitotData = new Data[numof_maxData];
        int counter = 0;
        while (true)
        {
            Data *tmp_data = nullptr;
            // Queueにデータがくるまで待つ
            if (xQueueReceive(PitotToDistributeQueue, &tmp_data, portMAX_DELAY) == pdTRUE)
            {
                pitotData[counter] = *tmp_data;
                ++counter;
                delete tmp_data;

                if (counter >= numof_maxData) // 一度に送信するタスク
                {
                    counter = 0;
                    xQueueSend(DistributeToParityQueue, &pitotData, 0);
#ifdef SPIFLASH
                    Data* pitotData_flash = new Data[numof_maxData];
                    memcpy(pitotData_flash, pitotData, sizeof(Data) * numof_maxData);
                    xQueueSend(DistributeToFlashQueue, &pitotData_flash, 0);
#endif
#ifdef CAN_MCP2562
                    Data* pitotData_can = new Data;
                    *pitotData_can = pitotData[0];
                    xQueueSend(DistributeToCanQueue, &pitotData_can, 0);
#endif
                    pitotData = new  Data[numof_maxData];
                }
            }
            else
            {
                pr_debug("failed to receive PitotToDistributeQueue");
            }
        }
    }

#if defined(DEBUG) && !defined(PITOT)
    void IRAM_ATTR createData(void *pvParameter)
    {
        portTickType xLastWakeTime = xTaskGetTickCount();
        for (;;)
        {
            Data *pitotData = new Data;
            // 2^32ms = 49日 uint32_tで足りると判断
            pitotData->time = static_cast<uint32_t>(esp_timer_get_time());
            pitotData->pa = 12.099567;
            pitotData->temp = 23.082558; // test用の値 実際に取れた値の一つ
            if (xQueueSend(PitotToDistributeQueue, &pitotData, 2) != pdTRUE)
            {
                error_log("%s(%d) failed to send queue date:%lu Queue:%d", __FILE__, __LINE__, millis(), uxQueueMessagesWaiting(PitotToDistributeQueue));
                delete pitotData;
            }
            vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS);
        }
    }
#endif
}