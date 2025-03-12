#include "flash.h"
#include <lib.h>
#include "debug.h"
#include "S25FL127S 1.0.0/src/SPIflash.h"
#include <Arduino.h>
#include "task_queue.h"
#include "common_task.h"

#if !defined(DEBUG) || defined(SPIFLASH)

#define SPIFREQ 5000000

SPICREATE::SPICreate SPIC1;
Flash flash1;

constexpr int data_size = numof_maxData * (sizeof(Data) / sizeof(uint8_t));
constexpr int FLASH_BLOCK_SIZE = 0x100;

// Flashから読み取った値をData型に変換する
union PitotDataUnion
{
    Data pitotData[numof_maxData];
    uint8_t Uint8Data[numof_maxData * (sizeof(Data) / sizeof(uint8_t))];
};

namespace flash
{
    PitotDataUnion pitotData;
    uint8_t tmp[256];
    int get_old_data()
    {
        pr_debug("getting old flash data...");
        int counter = 0;
        bool is_remaining;
        do
        {
            is_remaining = false;
            flash1.read(counter, tmp);
            for (int i = 0; i < 256; i++)
            {
                if (tmp[i] != 255)
                {
                    is_remaining = true;
                    for (int j = 0; j < data_size; j++)
                    {
                        pitotData.Uint8Data[j] = tmp[j];
                        pr_debug("%u, ", tmp[i]);
                    }
                    char *data = cmn_task::DataToChar(pitotData.pitotData);
                    if (!data)
                    {
                        pr_debug("nullptr found");
                        return 1; // cannot do new
                    }
                    pr_debug("%s", data);
                    delete[] data;
                    counter += FLASH_BLOCK_SIZE;
                    break;
                }
            }
        } while (is_remaining);
        return 0;
    }

    // setup内で実行 エラー処理なし
    int init()
    {
        SPIC1.begin(SPI2_HOST, CLK, MISO, MOSI);
        flash1.begin(&SPIC1, CS, SPIFREQ);
        int result = get_old_data();
        if (result)
        {
            pr_debug("Can't get old flash data");
            return 1;
        }
#ifdef DEBUG
        pr_debug("In DEBUG mode flash will erased");
        // flash1.erase();
#endif
        return 0;
    }

    void IRAM_ATTR writeDataToFlash(void *pvParameter)
    {
        int counter = 0;
        pr_debug("flash write data size: %d", data_size);
#ifdef DEBUG
        configASSERT(256 >= data_size);
#endif
        while (true)
        {
            // numof_maxData個だけDataが送られてくるので、あまりを0埋めして保存
            uint8_t *tmp = nullptr;
            // Queueにデータがくるまで待つ
            if (xQueueReceive(DistributeToFlashQueue, &tmp, portMAX_DELAY) == pdTRUE)
            {
                uint8_t pitotData[256];
                for (int i = 0; i < 256; i++)
                {
                    if (i < data_size)
                    {
                        pitotData[i] = tmp[i];
                    }
                    else
                    {
                        pitotData[i] = 255;
                    }
                }
                flash1.write(counter, pitotData);
                /*
                PitotDataUnion change;
                for (int i = 0; i < 256; i++)
                {
                    change.Uint8Data[i] = pitotData[i];
                }
                char *data = cmn_task::DataToChar(change.pitotData);
                pr_debug("flash data: %s", data);
                */
                delete[] tmp;
                counter += FLASH_BLOCK_SIZE;
            }
            else
            {
                pr_debug("failed to receive DistributeToFlashQueue");
            }
        }
    }
}

#endif