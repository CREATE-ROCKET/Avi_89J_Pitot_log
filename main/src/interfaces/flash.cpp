#include <S25FL127S 1.0.0/src/SPIflash.h>
#include "flash.h"
#include "lib.h"
#include "debug.h"
#include <Arduino.h>
#include "task_queue.h"

#if !defined(DEBUG) || defined(SPIFLASH)

#define SPIFREQ 5000000

SPICREATE::SPICreate SPIC1;
Flash flash1;

constexpr int data_size = numof_maxData * (sizeof(Data) / sizeof(uint8_t));
int position = 0; // SPIflashの書き込み位置
constexpr uint32_t FLASH_BLOCK_SIZE = 0x100;
constexpr uint32_t SPI_FLASH_MAX_ADDRESS = 0x2000000;

namespace flash
{
    int init()
    {
        if (!SPIC1.begin(SPI2_HOST, CLK, MISO, MOSI))
        {
            pr_debug("Failed to init spi bus");
            return 1;
        }
        flash1.begin(&SPIC1, CS, SPIFREQ);
        return 0;
    }

    void IRAM_ATTR writeDataToFlash(void *pvParameter)
    {
        pr_debug("flash write data size: %d", data_size);
#ifdef DEBUG
        configASSERT(256 >= data_size);
#endif
        while (true)
        {
            // numof_maxData個だけDataが送られてくるので、あまりを0埋めして保存
            uint8_t *pitotData = nullptr;
            // Queueにデータがくるまで待つ
            if (xQueueReceive(DistributeToFlashQueue, &pitotData, portMAX_DELAY) == pdTRUE)
            {
                if (position >= SPI_FLASH_MAX_ADDRESS)
                {
                    error_log("all flash used!!!");
                    continue;
                }
                flash1.write(position, pitotData);
                delete[] pitotData;
                position += FLASH_BLOCK_SIZE;
            }
        }
    }

    void writeFlashDataToSD(void *pvParameter)
    {
        // int result = get_flash_old_data();
        // if (result)
        //     can::canSend('0' + result);
        // // result = get_SPIFFS_old_data();
        // if (result)
        //     can::canSend('0' + result);
        vTaskDelete(NULL);
    }
}

#endif

namespace flash
{
    void eraseFlash()
    {
#if !defined(DEBUG) || defined(SPIFLASH)
        flash1.erase();
        position = 0;
#endif
    }
}