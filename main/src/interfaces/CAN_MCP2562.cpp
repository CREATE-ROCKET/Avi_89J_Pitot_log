#include "CAN_MCP2562.h"
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include <Arduino.h>

volatile SemaphoreHandle_t semaphore_flash;

#if !defined(DEBUG) || defined(CAN_MCP2562)

#include <CANCREATE.h>

CAN_CREATE CAN(true);

TaskHandle_t canReceiveTask; // CANを受け取るタスク

namespace can
{
    int init()
    {
        semaphore_flash = xSemaphoreCreateBinary();
        can_setting_t settings;
        settings.baudRate = 100E3;
        settings.multiData_send = true;
        settings.filter_config = CAN_FILTER_DEFAULT;
        if (CAN.begin(settings, RX, TX, 2, SELECT))
        {
            pr_debug("Start Can failed");
            return 1; // CAN.begin() failed
        }
        xTaskCreateUniversal(can::canReceive, "canReceive", 2048, NULL, 6, &canReceiveTask, APP_CPU_NUM);
        if (!semaphore_flash)
        {
            pr_debug("failed to Start Can");
            return 2;
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
                {
                    CAN.sendData(2, tmp + i * 8, 8);
                    delay(10);
                }
                CAN.sendChar(2, '>');
                delay(10);
            }
        }
    }

    IRAM_ATTR void canReceive(void *pvParameter)
    {
        for (;;)
        {
            can_return_t Data;
            if (CAN.readWithDetail(&Data, portMAX_DELAY))
            {
                if (Data.size == 1)
                {
                    switch (Data.data[0])
                    {
                    case 'S': // シーケンス開始
                        if (xSemaphoreGive(semaphore_flash) != pdTRUE)
                        {
                            error_log("failed to Start Sequence");
                        }
                        break;
                    case 'Q': // シーケンス停止
                        if (xSemaphoreTake(semaphore_flash, portMAX_DELAY) != pdTRUE)
                        {
                            error_log("failed to End Sequence");
                        }
                        break;
                    case 'K': // microSD停止
                        if (xSemaphoreTake(semaphore_sd, portMAX_DELAY) != pdTRUE)
                        {
                            error_log("failed to End SD");
                        }
                        break;
                    case 'E': // フラッシュ削除
                        // if (xTaskNotifyGive(writeDataToFlashTaskHandle))
                        {
                            error_log("failed to Erase flash");
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
            else
                error_log("failed to read CAN");
        }
    }
}
#endif

namespace can
{
    void canSend(char data)
    {
#if !defined(DEBUG) || defined(CAN_MCP2562)
        CAN.sendChar(1, data);
#endif
    }
}