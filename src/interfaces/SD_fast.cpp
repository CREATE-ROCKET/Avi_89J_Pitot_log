#include "SD_fast.h"
#include <FS.h>
#include <SD_MMC.h>
#include "lib.h"
#include "debug.h"
#include "task_queue.h"

#if !defined(DEBUG) || defined(SD_FAST)

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

  // Data型の配列をchar型に変換する
  // newしているのでdelete[]等必要
  char *DataToChar(Data pitotData[256])
  {
    // 必要なバッファサイズを計算する
     // 各行が最大で21文字 + 終端の '\0'
    size_t bufferSize = numof_maxData * (10 + 1 + 10 + 1) + 1;

    // バッファを動的に確保
    char *buffer = new char[bufferSize];
    if (buffer == nullptr)
    {
      return nullptr; // メモリ確保失敗時はnullptrを返す
    }

    size_t offset = 0;

    for (size_t i = 0; i < numof_maxData; ++i)
    {
      // snprintfを使って1行をフォーマット
      int written = snprintf(
          buffer + offset,
          bufferSize - offset,
          "%.2f %.2f\n",
          pitotData[i].pa,
          pitotData[i].temp);

      // バッファオーバーフローを防止
      if (written < 0 || offset + written >= bufferSize)
      {
        delete[] buffer; // メモリを解放して失敗を返す
        return nullptr;
      }

      offset += written;
    }

    return buffer;
  }

  int appendFile(fs::FS &fs, const char *path, const char *message)
  {
    pr_debug("Appending to file: %s", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
      pr_debug("Failed to open file for appending");
      return 1; // failed to open file
    }
    if (file.print(message))
    {
      return 0; // success
    }
    else
    {
      pr_debug("data append failed");
      return 2; // failed to append file
    }
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

  // makeParityからchar型のデータを受け取り、それを書き込むタスク
  // appendFileでopenとcloseを逐一やるため遅い可能性が高い
  // TaskHandleはwriteDataToSDTaskHandle
  IRAM_ATTR void writeDataToSD(void *pvParameter)
  {
    while (true)
    {
      Data pitotData[numof_maxData];
      if (xQueueReceive(DistributeToSDQueue, &pitotData, 0) == pdTRUE)
      {
        char *data = DataToChar(pitotData);
        int result = appendFile(SD_MMC, "/test.txt", data);
        if (result)
        {
          pr_debug("failed to write SD: %d", result);
        }
        delete[] data;
        vTaskSuspend(NULL); // suspend ourselves
      }
      else
      {
        delay(1);
      }
    }
  }
}
#endif