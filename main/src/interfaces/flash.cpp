#include "flash.h"
#include <lib.h>
#include "debug.h"
#include "S25FL127S 1.0.0/src/SPIflash.h"
#include <Arduino.h>
#include <esp_spiffs.h>
#include <esp_task_wdt.h>
#include <cstdio>
#include "task_queue.h"
#include "common_task.h"
#include "CAN_MCP2562.h"

#if !defined(DEBUG) || defined(SPIFLASH)

#define SPIFREQ 5000000

SPICREATE::SPICreate SPIC1;
Flash flash1;

constexpr int data_size = numof_maxData * (sizeof(Data) / sizeof(uint8_t));
constexpr int FLASH_BLOCK_SIZE = 0x100;
constexpr uint32_t SPI_FLASH_MAX_ADDRESS = 0x2000000;
constexpr char *num_path = "/spiffs/number.txt";

String SPIFFSpath;

// Flashから読み取った値をData型に変換する
union PitotDataUnion
{
    Data pitotData[numof_maxData];
    uint8_t Uint8Data[numof_maxData * (sizeof(Data) / sizeof(uint8_t))];
};

namespace flash
{
    int makeNewFile()
    {
        using namespace std;
        int number = 0;
        FILE *numberHandle = fopen(num_path, "r");

        if (numberHandle)
        {
            // /spiffs/number.txt ファイルが存在する場合読み取る
            char fileContent[10];
            if (!fgets(fileContent, sizeof(fileContent), numberHandle))
            {
                pr_debug("failed to read file");
                return 2;
            }
            errno = 0;
            char *tmp;
            number = static_cast<int>(strtol(fileContent, &tmp, 10));
            if (errno == ERANGE || number < 0 || number == INT_MAX)
            {
                pr_debug("failed to convert to int");
                return 3;
            }
            if (tmp == fileContent || *tmp != '\n' && *tmp != '\0')
                pr_debug("\e[31mデータが破損している可能性があります\e[37m");
        }
        else
            // ファイルがないはず
            pr_debug("Cannot find number.txt assume the SPIFFS is init. creating...");
        numberHandle = freopen(num_path, "w", numberHandle);
        if (!numberHandle)
        {
            pr_debug("failed to open file for writing");
            return 1;
        }
        fprintf(numberHandle, "%d", number + 1);
        fclose(numberHandle);
        SPIFFSpath = "/spiffs/data" + String(number) + ".csv";
        pr_debug("SPIFFS path: %s", SPIFFSpath.c_str());
        return 0;
    }

    PitotDataUnion pitotData;
    uint8_t tmp[256];
    int get_old_data()
    {
        pr_debug("getting old flash data...");
        int position = 0;
        bool is_remaining;
        do
        {
            is_remaining = false;
            flash1.read(position, tmp);
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
                    position += FLASH_BLOCK_SIZE;
                    break;
                }
            }
        } while (is_remaining);
        return 0;
    }

    int init()
    {
        SPIC1.begin(SPI2_HOST, CLK, MISO, MOSI);
        flash1.begin(&SPIC1, CS, SPIFREQ);
        esp_vfs_spiffs_conf_t configSPIFFS = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = false,
        };
        esp_err_t result = esp_vfs_spiffs_register(&configSPIFFS);
        if (result != ESP_OK)
        {
            pr_debug("failed to init SPIFFS: %s", esp_err_to_name(result));
            return 1;
        }
        result = esp_spiffs_check(configSPIFFS.base_path);
        if (result != ESP_OK)
        {
            pr_debug("failed to check SPIFFS: %s", esp_err_to_name(result));
            return 2;
        }
#ifdef DEBUG
        size_t total, used;
        result = esp_spiffs_info(configSPIFFS.base_path, &total, &used);
        if (result != ESP_OK)
        {
            pr_debug("SPIFFS\n\ttotal size:%u\nused size:\t%u", total, used);
            if (total < used)
            {
                pr_debug("used size is bigger than the total size is crazy");
                return 1;
            }
        }
        else
            pr_debug("failed to read spiffs info: %s", esp_err_to_name(result));
#endif
        int result = makeNewFile();
        if (result)
        {
            pr_debug("failed to make new file in SPIFFS: %d", result);
        }
        return 0;
    }

    void IRAM_ATTR writeDataToFlash(void *pvParameter)
    {
        int position = 0;
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
                FILE *tmp = fopen(SPIFFSpath.c_str(), "ab");
                fwrite(pitotData, 1, 256, tmp);
                fclose(tmp);
                delete[] pitotData;
                position += FLASH_BLOCK_SIZE;
            }
        }
    }
}

#endif

namespace flash
{
    void eraseFlash()
    {
#if !defined(DEBUG) || defined(SPIFLASH)
        flash1.erase();

        TaskHandle_t tmp = xTaskGetIdleTaskHandle();
        if (!tmp || esp_task_wdt_delete(tmp) != ESP_OK)
            pr_debug("failed to disable watch dog timer");
        esp_err_t result = esp_spiffs_format("/spiffs");
        if (!tmp || esp_task_wdt_add(tmp) != ESP_OK)
            pr_debug("failed to enable watchdog timer");
        if (result)
        {
            can::canSend('F');
            error_log("failed to format spiffs: %s", esp_err_to_name(result));
        }
#endif
    }
}