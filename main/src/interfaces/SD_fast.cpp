#include "SD_fast.h"
#include <FS.h>
#include <SD_MMC.h>
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include "common_task.h"

File dataFileHandle;
File logFileHandle;
#if !defined(DEBUG) || defined(SD_FAST)

String dataFile;
String logFile;
volatile bool run_close_task = false;

//  DEBUGINPUTがRiseされたとき、別のタスクに取られてwriteDataToSDTaskがこれ以上動作しないようにする
volatile SemaphoreHandle_t semaphore_sd;
TaskHandle_t GetSDSemaphoreHandle;

namespace sd_mmc
{
#ifdef DEBUG
  void _listDir(fs::FS &fs, const char *dirname, uint8_t levels)
  {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
      Serial.println("Failed to open directory");
      return;
    }
    if (!root.isDirectory())
    {
      Serial.println("Not a directory");
      return;
    }

    File file = root.openNextFile();
    while (file)
    {
      if (file.isDirectory())
      {
        Serial.print("  DIR : ");
        Serial.println(file.name());
        if (levels)
        {
          _listDir(fs, file.path(), levels - 1);
        }
      }
      else
      {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("  SIZE: ");
        Serial.println(file.size());
      }
      file = root.openNextFile();
    }
  }
#endif

  int appendFile(String path, const char *message)
  {
    pr_debug("Appending to file: %s", path.c_str());
    int64_t sd_write_time = esp_timer_get_time();
    File file = SD_MMC.open(path, FILE_APPEND);
    if (!file)
    {
      pr_debug("Failed to open file for appending");
      return 1; // failed to open file
    }
    if (!file.print(message))
    {
      pr_debug("data append failed");
      return 2; // failed to append file
    }
    file.close();
    pr_debug("write time: %lld ms", esp_timer_get_time() - sd_write_time);
    return 0;
  }

  int init()
  {
    semaphore_sd = xSemaphoreCreateBinary();
    xSemaphoreGive(semaphore_sd);
#ifdef IS_S3
    if (!SD_MMC.setPins(sd_mmc::CLK, sd_mmc::CMD, sd_mmc::DAT0, sd_mmc::DAT1, sd_mmc::DAT2, sd_mmc::DAT3))
    {
      pr_debug("failed to set pins");
      return 3; // failed to set pin
    }
#endif
    if (!SD_MMC.begin())
    {
      pr_debug("SD_init failed");
      return 1; // init failed
    }
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE)
    {
      pr_debug("No SD_MMC card attached");
      return 2; // sd none
    }
#ifdef DEBUG
    Serial.print("SD_MMC Card Type: ");
    if (cardType == CARD_MMC)
    {
      pr_debug("MMC");
    }
    else if (cardType == CARD_SD)
    {
      pr_debug("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
      pr_debug("SDHC");
    }
    else
    {
      pr_debug("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    pr_debug("SD_MMC Card Size: %lluMB\n", cardSize);
    _listDir(SD_MMC, "/", 0);
#endif
    return 0; // sd init success
  }

  // SDのファイルを作成する
  // - data
  //  - data1.csv
  //  - data2.csv
  //  - ...
  // - log
  //  - log1.csv
  //  - log2.csv
  //  - ...
  // - number.txt
  // dataフォルダ直下にピトー管のデータを保存し、
  // dataN.csv(Nは1,2,3...)の形式
  // logフォルダ直下にerrorログやmemory残量等のデータを保存する。
  // logN.csv(Nは1,2,3...)の形式
  // Nは再起動ごとに1から順番に増えていき、number.txtに入っている数字で最大数を管理する。
  int makeNewFile()
  {
    // まずnumber.txtがあるかどうかを判別し、あったらnumber.txtに入っている数字で
    // dataN.csv、 logN.csvを作成、なかったらnumber.txtを作成して0を代入。
    // number.txtは1足す
    pr_debug("making new file....");
    int number; // dataN.csv、 logN.csvのNの値

    File file = SD_MMC.open("/number.txt");
    int result = file.read();
    file.close();
    if (result == -1)
    {
      pr_debug("Cannot find number.txt assume the SD is init. creating...");
      number = 0;
      File file = SD_MMC.open("/number.txt", FILE_WRITE);
      if (!file)
      {
        pr_debug("Failed to open file!!!");
        return 1; // open error
      }
      if (!(file.print(1)))
      {
        pr_debug("Failed to write file!!!");
        return 2;
      }
      file.close();

      pr_debug("Can't find data dir. creating...");
      if (!SD_MMC.mkdir("/data"))
      {
        pr_debug("Failed to create data dir");
        return 8;
      }
      pr_debug("Can't find log dir. creating...");
      if (!SD_MMC.mkdir("/log"))
      {
        pr_debug("Failed to create log dir");
        return 9;
      }
    }
    else
    {
      pr_debug("number.txt found. reading...");
      File file = SD_MMC.open("/number.txt");
      if (!file)
      {
        pr_debug("Failed to open file!!!");
        return 3;
      }
      String number_txt = "";
      while (file.available())
      {
        number_txt.concat((char)file.read()); // += (char)file.read();
      }
      if (!number_txt.length())
      {
        pr_debug("number.txt maybe broken");
        return 4;
      }
      number = number_txt.toInt(); // 1 or more
      if (!number)
      {
        pr_debug("number.txt maybe contain nonInt: %s", number_txt.c_str());
        return 5;
      }
      file.close();

      File file1 = SD_MMC.open("/number.txt", FILE_WRITE);
      if (!file1)
      {
        pr_debug("Failed to open file!!!");
        return 6; // open error
      }
      if (!file1.print(number + 1))
      {
        pr_debug("Failed to write file!!!");
        return 7;
      }
      file1.close();
    }

    // dataN.csvとlogN.csvを作成する
    dataFile = "/data/data";
    logFile = "/log/log";
    String number_path_csv = String(number) + ".csv";
    String number_path_txt = String(number) + ".txt";
    dataFile += number_path_csv;
    logFile += number_path_txt;
    pr_debug("dataFile: %s, logFile: %s", dataFile.c_str(), logFile.c_str());
    File dataFileHandle = SD_MMC.open(dataFile, FILE_WRITE);
    if (!dataFileHandle)
    {
      pr_debug("failed to open file");
      return 10;
    }
    if (!dataFileHandle.print("time, pascal, temperature\n"))
    {
      pr_debug("failed to write file");
      return 11;
    }
    File logFileHandle = SD_MMC.open(logFile, FILE_WRITE);
    dataFileHandle.close();
    logFileHandle.close();
    return 0;
  }

  void IRAM_ATTR GetSDSemaphore(void *pvParameter)
  {
    xSemaphoreTake(semaphore_sd, portMAX_DELAY);
    pr_debug("close SD");
    SD_MMC.end();
    cmn_task::blinkLED_start(1, 1000);
    vTaskDelete(NULL);
  }

  void IRAM_ATTR onButton()
  {
    if (!run_close_task)
    {
      pr_debug("Debug pin rising");
      xTaskCreateUniversal(sd_mmc::GetSDSemaphore, "microSD", 2048, NULL, 6, &GetSDSemaphoreHandle, PRO_CPU_NUM);
      run_close_task = true;
    }
  }
}
#endif

namespace sd_mmc
{
  IRAM_ATTR void makeParity(void *pvParameter)
  {
    while (true)
    {
      Data *pitotData = nullptr;
      constexpr float rho = 2.8966 * 101.33 / 8.314 / 10;
      // Queueにデータがくるまで待つ
      if (xQueueReceive(DistributeToParityQueue, &pitotData, portMAX_DELAY) == pdTRUE)
      {
        char *data = cmn_task::DataToChar(pitotData);
        // pr_debug("%s(%d) :%s", __FILE__, __LINE__, data);
        if (!data)
        {
          pr_debug("nullptr found");
          continue;
        }
        pr_debug("%s, speed: %g", data, std::sqrt(pitotData->pa * 2 / rho));
        delete[] pitotData;
        SD_Data *data_wrapper = new SD_Data;
        data_wrapper->is_log = false;
        data_wrapper->data = data;
        if (xQueueSend(ParityToSDQueue, &data_wrapper, 0) != pdTRUE)
        {
          pr_debug("failed to send parity to sd queue");
          delete data_wrapper;
        }
      }
      else
      {
        delay(1);
      }
    }
  }

  // makeParityからchar型のデータを受け取り、それを書き込むタスク
  // appendFileでopenとcloseを逐一やるため遅い可能性が高い
  // TaskHandleはwriteDataToSDTaskHandle
  IRAM_ATTR void writeDataToSD(void *pvParameter)
  {
    for (;;)
    {
      SD_Data *data_wrapper;
      if (xQueueReceive(ParityToSDQueue, &data_wrapper, portMAX_DELAY) == pdTRUE)
      {
#if !defined(DEBUG) || defined(SD_FAST)
        if (xSemaphoreTake(semaphore_sd, 0) == pdTRUE)
        {
#endif
          if (!(data_wrapper->is_log))
          {
#if !defined(DEBUG) || defined(SD_FAST)
            int result = appendFile(dataFile, data_wrapper->data);
            if (result)
            {
              pr_debug("failed to write SD: %d", result);
            }
#endif
            // pr_debug("%s", data_wrapper->data);
          }
          else
          {
#if !defined(DEBUG) || defined(SD_FAST)
            int result = appendFile(logFile, data_wrapper->data);
            if (result)
            {
              pr_debug("failed to write SD: %d", result);
            }
#endif
            pr_debug("log: %s", data_wrapper->data);
          }

          delete[] data_wrapper->data;
          delete data_wrapper;
#if !defined(DEBUG) || defined(SD_FAST)
        }
        xSemaphoreGive(semaphore_sd);
#endif
      }
      else
      {
        pr_debug("failed to receive Queue");
      }
    }
  }
}