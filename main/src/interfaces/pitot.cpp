#include "pitot.h"
#include "lib.h"
#include "debug.h"
#include <ms4525do.h>
#include "task_queue.h"
#include "common_task.h"
#include "interfaces/CAN_MCP2562.h"

#if !defined(DEBUG) || defined(PITOT)

bfs::Ms4525do pres;

namespace pitot
{
    void IRAM_ATTR getPitotData(void *pvParameter)
    {
#if IS_S3
        // ピトー管初期化遅延用
        if (digitalRead(debug::DEBUG_INPUT2))
        {
            // HIGHのとき
            cmn_task::blinkLED_start(2, 1000); // ピトー管を点滅
            can::canSend('w');
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            cmn_task::blinkLED_stop(2);
            digitalWrite(led::LED_PITOT, HIGH);
        }

        if (Wire.setPins(pitot::SDA, pitot::SCL))
        {
#endif
            if (Wire.begin())
            {
                if (Wire.setClock(0))
                {

                    // set pitot config
                    pres.Config(&Wire, 0x28, 1.0f, -1.0f);

                    if (pres.Begin())
                    {

                        switch (pres.status())
                        {
                        case bfs::Ms4525do::Status::STATUS_GOOD:
                            break;
                        case bfs::Ms4525do::Status::STATUS_STALE_DATA:
                            error_log("pitot error: STATUS_STALE_DATA");
                            break;
                        case bfs::Ms4525do::Status::STATUS_FAULT:
                            error_log("pitot error: STATUS_FAULT");
                            break;
                        default:
                            pr_debug("code error");
                            break;
                        }

                        portTickType xLastWakeTime = xTaskGetTickCount();
                        while (true)
                        {
                            if (pres.Read())
                            {
                                Data *pitotData = new Data;
                                pitotData->time = esp_timer_get_time();
                                pitotData->pa = pres.pres_pa();
                                pitotData->temp = pres.die_temp_c();
                                if (xQueueSend(PitotToDistributeQueue, &pitotData, 2) != pdTRUE)
                                {
                                    error_log("%s(%d) failed to send queue date:%lu Queue:%d", __FILE__, __LINE__, millis(), uxQueueMessagesWaiting(PitotToDistributeQueue));
                                    delete pitotData;
                                }
                                vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS); // 10msごとに呼び出すことにする
                            }
                            else
                            {
                                vTaskDelay(1 / portTICK_PERIOD_MS);
                            }
                        }
                    }
                }
            }
#if IS_S3
        }
#endif
        error_log("Pitot init failed. restarting...");
        ESP.restart();
        vTaskDelete(NULL);
    }
}

#endif