#include "task.h"
#include <Arduino.h>
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include <Ticker.h>
#include "memory_controller.h"

Ticker blinker1;
Ticker blinker2;

namespace cmn_task
{
    // getPitotDataでnewされたデータを受け取って、
    // makeParity、 writeDataToFlash、 SendDataByCan に送信し、
    // すべてで利用されたあとdeleteする
    void IRAM_ATTR distribute_data(void *pvParameter)
    {
        Data *pitotData = mem_controller::new_ptr();
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
                    xQueueSend(DistributeToFlashQueue, &pitotData, 0);
#endif
                    pitotData = mem_controller::new_ptr();
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
        float counter = 0;
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