#include "flash.h"
#include "lib.h"
#include "debug.h"
#include "S25FL127S 1.0.0/src/SPIflash.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include "task_queue.h"
#include "common_task.h"
#include "CAN_MCP2562.h"

#if !defined(DEBUG) || defined(SPIFLASH)

#define SPIFREQ 5000000

SPICREATE::SPICreate SPIC1;
Flash flash1;

constexpr int data_size = numof_maxData * (sizeof(Data) / sizeof(uint8_t));
int position = 0; // SPIflashの書き込み位置
constexpr int FLASH_BLOCK_SIZE = 0x100;
constexpr uint32_t SPI_FLASH_MAX_ADDRESS = 0x2000000;
constexpr char num_path[] = "/number.txt";

String SPIFFSpath;

// Flashから読み取った値をData型に変換する
union PitotDataUnion
{
    Data pitotData[numof_maxData];
    uint8_t Uint8Data[numof_maxData * (sizeof(Data) / sizeof(uint8_t))];
};

namespace flash
{
    // 基本的にSDと同様
    int makeNewFile()
    {
        int number = 0;
        File file = SPIFFS.open(num_path);
        int result = file.read();
        if (result != -1)
        { // ファイルが存在する場合読み取る
            String number_txt = "";
            while (file.available())
            {
                number_txt.concat((char)file.read());
            }
            if (!number_txt.length())
            {
                pr_debug("number.txt maybe broken: %s", number_txt.c_str());
                return 1;
            }
            number = number_txt.toInt();
            if (!number)
            {
                pr_debug("number.txt maybe contain nonInt: %s", number_txt.c_str());
                return 2;
            }
        }
        else // /spiffs/number.txt ファイルが存在しない場合
            pr_debug("Cannot find number.txt assume the SPIFFS is init. creating...");
        file.close();

        File file1 = SPIFFS.open(num_path, FILE_WRITE);
        if (!file1)
        {
            pr_debug("Failed to open file!!!");
            return 3;
        }
        if (!file1.print(number + 1))
        {
            pr_debug("Failed to write file!!!");
            return 4;
        }
        file1.close();
        SPIFFSpath = "/" + String(number) + ".csv";
        pr_debug("SPIFFS path: %s", SPIFFSpath.c_str());
        File tmp = SPIFFS.open(SPIFFSpath, FILE_WRITE);
        if (!tmp)
        {
            pr_debug("failed to open file");
            return 5;
        }
        if (!tmp.print("time, pascal, temperature\n"))
        {
            pr_debug("failed to write file");
            return 6;
        }
        tmp.close();
        return 0;
    }

    int SPIFFS_init()
    {
        // 初期化されているため、ファイル名は0から始まる
        File numberHandle = SPIFFS.open(num_path, "w");
        if (!numberHandle)
        {
            pr_debug("%s cannot open", num_path);
            return 1;
        }
        if (!numberHandle.print("1"))
        {
            pr_debug("failed to write file");
            return 2;
        }
        numberHandle.close();
        SPIFFSpath = "/0.csv";
        pr_debug("SPIFFS path: %s", SPIFFSpath.c_str());
        File tmp = SPIFFS.open(SPIFFSpath, FILE_WRITE);
        if (!tmp)
        {
            pr_debug("failed to open file");
            return 5;
        }
        if (!tmp.print("time, pascal, temperature\n"))
        {
            pr_debug("failed to write file");
            return 6;
        }
        tmp.close();
        return 0;
    }

    PitotDataUnion pitotData;
    uint8_t tmp[256];
    int get_flash_old_data()
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
                    SD_Data *data_wrapper = new SD_Data;
                    data_wrapper->type = data_type::data_type_spi_flash;
                    data_wrapper->data = data;
                    if (xQueueSend(ParityToSDQueue, &data_wrapper, portMAX_DELAY) != pdTRUE)
                    {
                        delete[] data;
                        delete data_wrapper;
                        return 2;
                    }
                    position += FLASH_BLOCK_SIZE;
                    break;
                }
            }
        } while (is_remaining);
        return 0;
    }

    int get_SPIFFS_old_data()
    {
        File root = SPIFFS.open("/");
        if (!root)
        {
            pr_debug("failed to open root dir");
            return 1;
        }
        pr_debug("listing SPIFFS dir...");
        File file = root.openNextFile();
        while (file)
        {
            if (file.isDirectory())
            {
                pr_debug("no directory expected. but found");
                break;
            }
            if (!file.available())
            {
                pr_debug("not available file found");
                continue;
            }
            const char *filename = file.name();
            if (strcmp(filename, "number.txt") || strcmp(filename, num_path))
                continue;
            char *end;
            int filecount = strtol(filename, &end, 10);
            if (*end != '.')
            {
                pr_debug("failed to convert filename");
                return 2;
            }
            size_t filesize = file.size();
            char *buffer;
            if (psramFound())
            {
                buffer = (char *)heap_caps_malloc(filesize + 1, MALLOC_CAP_SPIRAM);
            }
            else
            {
                pr_debug("no PSRAM found");
                buffer = (char *)malloc(filesize + 1);
            }
            if (!buffer)
            {
                pr_debug("failed to malloc");
                return 3;
            }

            file.read((uint8_t *)buffer, filesize);
            buffer[filesize] = '\0';
            SD_Data *data_wrapper = new SD_Data;
            data_wrapper->data = buffer;
            data_wrapper->type = data_type::data_type_SPIFFS + filecount;
            if (xQueueSend(ParityToSDQueue, &data_wrapper, portMAX_DELAY) != pdTRUE)
            {
                pr_debug("failed to send");
                return 4;
            }
            file = file.openNextFile();
        }
        return 0;
    }

    int appendfile(char *message)
    {
        pr_debug("Appending to file: %s", SPIFFSpath.c_str());
        File file = SPIFFS.open(SPIFFSpath, FILE_APPEND);
        if (!file)
        {
            pr_debug("Failed to open file for appending");
            return 1;
        }
        if (!file.print(message))
        {
            pr_debug("data append failed");
            return 2;
        }
        file.close();
        return 0;
    }

    int init()
    {
        if (!SPIC1.begin(SPI2_HOST, CLK, MISO, MOSI))
        {
            pr_debug("Failed to init spi bus");
            return 1;
        }
        flash1.begin(&SPIC1, CS, SPIFREQ);
        if (!SPIFFS.begin(true))
        {
            pr_debug("Failed to init SPIFFS");
            return 2;
        }
#ifdef DEBUG
        size_t total, used;
        total = SPIFFS.totalBytes();
        used = SPIFFS.usedBytes();
        pr_debug("SPIFFS\n\ttotal size:%u\nused size:\t%u", total, used);
#endif
        int makefileresult = makeNewFile();
        if (makefileresult)
        {
            pr_debug("failed to make new file in SPIFFS: %d", makefileresult);
        }
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
                char *tmp = cmn_task::DataToChar((Data *)pitotData);
                if (tmp)
                {
                    appendfile(tmp);
                    delete[] tmp;
                }
                delete[] pitotData;
                position += FLASH_BLOCK_SIZE;
            }
        }
    }

    void writeFlashDataToSD(void *pvParameter)
    {
        int result = get_flash_old_data();
        if (result)
            can::canSend('0' + result);
        result = get_SPIFFS_old_data();
        if (result)
            can::canSend('0' + result);
        vTaskDelete(NULL);
    }
}

#endif

namespace flash
{
    void eraseFlash()
    {
#if !defined(DEBUG) || defined(SPIFLASH)
        esp_task_wdt_delete(xTaskGetCurrentTaskHandle());
        flash1.erase();
        position = 0;
        can::canSend('1');
        if (!SPIFFS.format())
        {
            can::canSend('F');
            error_log("failed to format SPIFFS");
        }
        esp_task_wdt_add(xTaskGetCurrentTaskHandle());
        can::canSend('2');
        int hoge = SPIFFS_init();
        if (hoge)
        {
            can::canSend('F');
            error_log("failed to make new file: %d", hoge);
        }
#endif
    }
}