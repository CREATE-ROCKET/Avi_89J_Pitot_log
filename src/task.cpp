#include "task.h"
#include <Arduino.h>
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include <Ticker.h>
#include "memory_controller.h"

// Flashで書き込むために Data型をunit8_t[8]に変換する
union PitotDataUnion
{
    Data pitotData[numof_maxData];
    uint8_t Uint8Data[numof_maxData * 8];
};

Ticker blinker1;
Ticker blinker2;

namespace cmn_task
{
    int counter = 0;

    PitotDataUnion pitotData[numof_maxData];
    uint8_t *DataForFlash = pitotData->Uint8Data;
    // メモリの利用が地獄になってる
    // getPitotDataでnewされたデータを受け取って、
    // makeParity、 writeDataToFlash、 SendDataByCan に送信し、
    // すべてで利用されたあとdeleteする
    void IRAM_ATTR distribute_data(void *pvParameter)
    {
        while (true)
        {
            // Queueにデータがくるまで待つ
            if (xQueueReceive(PitotToDistributeQueue, &pitotData->pitotData[counter], portMAX_DELAY) == pdTRUE)
            {
                ++counter;
                if (counter >= numof_maxData) // 一度に送信するタスク
                {
                    counter = 0;
#ifdef SD_FAST
                    xQueueSend(DistributeToParityQueue, &pitotData, 0);
#endif
#ifdef SPIFLASH
                    xQueueSend(DistributeToFlashQueue, DataForFlash, 0);
#endif
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
            Data *pitotData = mem_controller::new_ptr();
            pitotData->pa = 12.099567;
            pitotData->temp = 23.082558; // test用の値 実際に取れた値の一つ
            if (xQueueSend(PitotToDistributeQueue, &pitotData, 0) != pdTRUE)
            {
                error_log("%s(%d) failed to send queue date:%lu Queue:%d", __FILE__, __LINE__, millis(), uxQueueMessagesWaiting(PitotToDistributeQueue));
                delete pitotData;
            }
            vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS);
        }
    }
#endif

    void blink1()
    {
        digitalWrite(led::LED1, !digitalRead(led::LED1));
    }

    void blink2()
    {
        digitalWrite(led::LED2, !digitalRead(led::LED2));
    }

    // LEDを点滅させる
    // ledには1or2を指定可能で、blinkIntervalはms指定で0だと連続点灯となる。
    void blinkLED_start(int led, int blinkInterval)
    {
        if (led == 1)
        {
            if (!blinkInterval)
            {
                blinkLED_stop(led);
                digitalWrite(led::LED1, HIGH);
                return;
            }
            blinker1.attach_ms(blinkInterval, blink1);
        }
        else if (led == 2)
        {
            if (!blinkInterval)
            {
                blinkLED_stop(led);
                digitalWrite(led::LED2, HIGH);
                return;
            }
            blinker2.attach_ms(blinkInterval, blink2);
        }
        else
        {
            pr_debug("Wrong led name specified!!!");
        }
    }

    // LEDの点滅を停止させる
    void blinkLED_stop(int led)
    {
        if (led == 1)
        {
            blinker1.detach();
            digitalWrite(led::LED1, LOW);
        }
        else if (led == 2)
        {
            blinker2.detach();
            digitalWrite(led::LED2, LOW);
        }
        else
        {
            pr_debug("Wrong led name specified!!!")
        }
    }
}