#include "SD_fast.h"
#include <FS.h>
#include <SD_MMC.h>
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include "memory_controller.h"

#if !defined(DEBUG) || defined(SD_FAST)

String dataFile;
String logFile;
TaskHandle_t SDcloseTaskHandle;
File dataFileHandle;
File logFileHandle;

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

  int appendFile(fs::FS &fs, String path, const char *message)
  {
    pr_debug("Appending to file: %s", path.c_str());
    int64_t sd_write_time = esp_timer_get_time();
    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
      pr_debug("Failed to open file for appending");
      return 1; // failed to open file
    }
    if (file.print(message))
    {
      pr_debug("data append failed");
      return 2; // failed to append file
    }
    file.close();
    pr_debug("write time: %ld ms", esp_timer_get_time() - sd_write_time);
    return 0;
  }

  int init()
  {
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

    File root = SD_MMC.open("/");
    bool is_found_number = false; // number.txtがあるかどうか
    bool is_found_data = false;   // dataフォルダがあるかどうか
    bool is_found_log = false;    // logフォルダがあるかどうか
    bool isDir;
    for (;;)
    {
      String filename = root.getNextFileName(&isDir);
      if (filename = "") // 最後までいった or ファイルが1つもない
        break;
      if (isDir)
      { // ディレクトリなら
        if (filename == "data")
        {
          is_found_data = true;
        }
        if (filename == "log")
        {
          is_found_log = true;
        }
      }
      else if (filename == "number.txt")
      { // ファイルなら
        is_found_number == true;
      }
    }

    if (!is_found_number)
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
      if (!(file1.print(number + 1)))
      {
        pr_debug("Failed to write file!!!");
        return 7;
      }
      file1.close();
    }
    if (!is_found_data)
    {
      pr_debug("Can't find data dir. creating...");
      if (!SD_MMC.mkdir("/data"))
      {
        pr_debug("Failed to create data dir");
        return 8;
      }
    }
    if (!is_found_log)
    {
      pr_debug("Can't find log dir. creating...");
      if (!SD_MMC.mkdir("/log"))
      {
        pr_debug("Failed to create log dir");
        return 9;
      }
    }

    // dataN.csvとlogN.csvを作成する
    dataFile = "/data/data";
    logFile = "/log/log";
    String number_path = String(number) + ".csv";
    dataFile += number_path;
    logFile += number_path;
    pr_debug("dataFile: %s, logFile: %s", dataFile.c_str(), logFile.c_str());
    dataFileHandle = SD_MMC.open(dataFile, FILE_WRITE);
    if (!dataFileHandle)
    {
      pr_debug("failed to open file");
      return 10;
    }
    if (dataFileHandle.print("time, pascal, temperature\n"))
    {
      pr_debug("failed to write file");
      return 11;
    }
    logFileHandle = SD_MMC.open(logFile, FILE_WRITE);
    dataFileHandle.close();
    logFileHandle.close();
    return 0;
  }

  void IRAM_ATTR SDcloseTask(void *pvParameter)
  {
    vTaskSuspend(writeDataToSDTaskHandle);
    vTaskDelete(writeDataToSDTaskHandle);
    //    SD_MMC.close();
    vTaskDelete(NULL);
  }

  void IRAM_ATTR onButton()
  {
    xTaskCreateUniversal(sd_mmc::SDcloseTask, "SDcloseTask", 2048, NULL, 9, &SDcloseTaskHandle, APP_CPU_NUM);
  }
}
#endif

namespace sd_mmc
{
  // Data型の配列をchar型に変換する
  // newしているのでdelete[]等必要
  char *DataToChar(Data pitotData[numof_maxData])
  {
    // バッファを動的に確保
    char *buffer = new char[AllbufferSize];
    if (buffer == nullptr)
    {
      return nullptr; // メモリ確保失敗時はnullptrを返す
    }

    size_t offset = 0;

    for (int i = 0; i < numof_maxData; ++i)
    {
      // snprintfを使って1行をフォーマット
      int written = snprintf(
          buffer + offset,
          bufferSize,
          "%u, %g, %g\n",
          pitotData[i].time,
          pitotData[i].pa,
          pitotData[i].temp);

      // バッファオーバーフローを防止
      if (written < 0 || offset + written >= AllbufferSize)
      {
        delete[] buffer; // メモリを解放して失敗を返す
        return nullptr;
      }

      offset += written;
    }
    return buffer;
  }

  IRAM_ATTR void makeParity(void *pvParameter)
  {
    while (true)
    {
      Data *pitotData = nullptr;
      // Queueにデータがくるまで待つ
      if (xQueueReceive(DistributeToParityQueue, &pitotData, portMAX_DELAY) == pdTRUE)
      {
        char *data = DataToChar(pitotData);
        // pr_debug("%s(%d) :%s", __FILE__, __LINE__, data);
        if (!data)
        {
          pr_debug("nullptr found");
          continue;
        }
        // mem_controller::delete_ptr(pitotData);
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
        char *data = data_wrapper->data;
        if (!(data_wrapper->is_log))
        {
#if !defined(DEBUG) || defined(SD_FAST)
          int result = appendFile(SD_MMC, dataFile, data);
          if (result)
          {
            pr_debug("failed to write SD: %d", result);
          }
#endif
          pr_debug("%s", data);
        }
        else
        {
#if !defined(DEBUG) || defined(SD_FAST)
          int result = appendFile(SD_MMC, logFile, data);
          if (result)
          {
            pr_debug("failed to write SD: %d", result);
          }
#endif
          pr_debug("log: %s", data);
        }
        delete[] data;
      }
      else
        pr_debug("failed to receive Queue");

      delete[] data_wrapper;
    }
  }
}