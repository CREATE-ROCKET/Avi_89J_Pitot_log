#include "CAN_MCP2562.h"
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include <Arduino.h>
#include "../common_task.h"
#include "../task_queue.h"
#include "flash.h"

volatile SemaphoreHandle_t semaphore_flash;

#if !defined(DEBUG) || defined(CAN_MCP2562)

#include <CANCREATE.h>

// CANの通信コマンド一覧
// 受信:
//  - S: シーケンス開始
//  - Q: シーケンス停止
//  - K: microSD停止
//  - E: flash削除

CAN_CREATE CAN(true);

TaskHandle_t canReceiveTask;               // CANを受け取るタスク
TaskHandle_t writeFlashDataToSDTaskHandle; // flashのデータをSDに書き込むタスク
bool is_in_sequence;                       // シークエンス中かどうか
bool is_SD_on = true;                      // SDが動作しているかどうか

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
        xTaskCreateUniversal(can::canReceive, "canReceive", 4096, NULL, 6, &canReceiveTask, APP_CPU_NUM);
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
                for (int i = 0; i < sizeof(Data) / sizeof(uint8_t) / 8; i++)
                {
                    CAN.sendData(2, tmp + i * 8, 8);
                    delay(10);
                }
                CAN.sendChar(2, '>');
                delete[] tmp;
                delay(10);
            }
        }
    }

    IRAM_ATTR void canReceive(void *pvParameter)
    {
        for (;;)
        {
            can_return_t Data;
            if (!CAN.readWithDetail(&Data, portMAX_DELAY))
            {
                if (Data.size == 1)
                {
                    canSend(Data.data[0]); // エコーバック
                    switch (Data.data[0])
                    {
                    case 'S': // シーケンス開始
                        pr_debug("S");
                        if (is_in_sequence)
                            canSend('F');
                        else if (xSemaphoreGive(semaphore_flash) != pdTRUE)
                        {
                            error_log("failed to Start Sequence");
                        }
                        is_in_sequence = true;
                        break;
                    case 'Q': // シーケンス停止
                        pr_debug("Q");
                        if (!is_in_sequence)
                            canSend('F');
                        if (xSemaphoreTake(semaphore_flash, portMAX_DELAY) != pdTRUE)
                        {
                            error_log("failed to End Sequence");
                        }
                        is_in_sequence = false;
                        break;
                    case 'K': // microSD停止
                        pr_debug("K");
                        if (is_in_sequence)
                            canSend('F');
                        else if (xSemaphoreTake(semaphore_sd, portMAX_DELAY) != pdTRUE)
                        {
                            error_log("failed to End SD");
                        }
                        else
                        {
#ifdef IS_S3
                            cmn_task::blinkLED_start(3, 1000);
#else
                            cmn_task::blinkLED_start(1, 1000);
#endif
                            is_SD_on = false;
                        }
                        break;
                    case 'E': // フラッシュ削除
                        pr_debug("E");
                        if (is_in_sequence)
                            canSend('F');
                        flash::eraseFlash();
                        canSend('s');
                        break;
                    case 'W': // フラッシュのデータをmicroSDに書き込む
                        pr_debug("W");
                        if (is_in_sequence || !is_SD_on)
                            canSend('F');
#if !defined(DEBUG) || defined(SPIFLASH)
                        xTaskCreateUniversal(
                            flash::writeFlashDataToSD,
                            "writeFlashDataToSDTaskHandle",
                            4096, NULL, 6,
                            &writeFlashDataToSDTaskHandle, PRO_CPU_NUM);
#endif
                        canSend('W');
                        break;
                    default:
                        break;
                    }
                }
            }
            else
                error_log("failed to read CAN");

            delay(100);
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