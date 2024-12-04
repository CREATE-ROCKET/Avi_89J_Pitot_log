// 89ピトー管基板用のコードです。
//
// ピトー管用I2C通信
//    SDA:    IO21
//    SCL:    IO22
//
// SPI flash
//    CS:     IO5
//    CLK:    IO18
//    MOSI:   IO23
//    MISO:   IO19
//
// SD_MMC
//    CLK:    IO14
//    DAT0:   IO2
//    DAT1:   IO4
//    DAT2:   IO12
//    DAT3:   IO13
//    CMD:    IO15
//
// CAN
//    TX:     IO25
//    RX:     IO26
//    SELECT: IO33
//
// LED
//    IO27
//    IO32
//
// DEBUG_INPUT
//    IO17    task1,


#include <Arduino.h>
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include "commonTask.h"

#if !defined(DEBUG) || defined(PITOT)
#include "interfaces/pitot.h"
#endif

#if !defined(DEBUG) || defined(SD_FAST)
#include "interfaces/SD_fast.h"
#endif

#if !defined(DEBUG) || defined(SPIFLASH)
#include "interfaces/flash.h"
#endif

#if !defined(DEBUG) || defined(CAN_MCP2562)
#include "interfaces/CAN_MCP2562.h"
#endif

// task handle
TaskHandle_t getPitotDataTaskHandle = nullptr; // pitotからデータを取得し、それをsendDataToEveryICへ送る 優先度 -1
TaskHandle_t sendDataToEveryICTaskHandle = nullptr; // getPitotDataから得たデータを各端末に送る
TaskHandle_t writeDataToFlashTaskHandle = nullptr; // SPI_Flashにデーターを書き込む
TaskHandle_t makeParityTaskHandle = nullptr; // Micro SD記録用のデータにパリティをつける
TaskHandle_t writeDataToSDTaskHandle = nullptr; // makeParityから受け取ったデータをMicroSDに書き込む
TaskHandle_t sendDataByCanTaskHandle = nullptr; // sendDataToEveryICから受け取ったデータを送る

// queue handle
QueueHandle_t PitotToDistributeQueue; // PitotからsendDataにデータを渡すQueue
QueueHandle_t DistributeToFlashQueue; // sendDataからFlashにデータを渡すQueue
QueueHandle_t DistributeToSDQueue;    // sendDataからSDにデータを渡すQueue

#ifdef DEBUG

void pr_feature_fg()
{
  Serial.println("feature flag");
  Serial.print("Pitot: \t");
#ifdef PITOT
  Serial.println("enabled");
#else
  Serial.println("disenabled");
#endif
  Serial.print("SD: \t");
#ifdef SD_FAST
  Serial.println("enabled");
#else
  Serial.println("disenabled");
#endif
  Serial.print("SPIFlash: \t");
#ifdef SPIFLASH
  Serial.println("enabled");
#else
  Serial.println("disenabled");
#endif
  Serial.print("Can: \t");
#ifdef CAN_MCP2562
  Serial.println("enabled");
#else
  Serial.println("disenabled");
#endif
}

#endif

void setup()
{
  int result;
  int error_num = 0; // 2以上ならcriticalな問題が発生したと判断してreboot
#ifdef DEBUG
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10);
  }
  Serial.println("Debug mode is on.");
  pr_feature_fg();
#endif
  pinMode(led::LED1, OUTPUT);
  pinMode(led::LED2, OUTPUT);
  digitalWrite(led::LED1, LOW);
  digitalWrite(led::LED2, HIGH);

#if !defined(DEBUG) || defined(SD_FAST)
  result = sd_mmc::init();
  if (result) {
    pr_debug("SD_init failed: %d",result);
    ++error_num;
  }
#endif
#if !defined(DEBUG) || defined(SPIFLASH)
  result = flash::init();
  if (result) {
    pr_debug("flash_init failed: %d",result);
    ++error_num;
  }
#endif
#if !defined(DEBUG) || defined(PITOT)
  result = pitot::init();
  if (result) {
    pr_debug("pitot_init failed: %d",result);
    error_num += 2;
  }
#endif
#if !defined(DEBUG) || defined(CAN_MCP2562)
  result = can::init();
  if (result) {
    pr_debug("can_init failed: %d",result);
  }
#endif

  if (error_num >= 2) {
    pr_debug("fatal error occurred!! rebooting ....");
    ESP.restart();
  }

  // TODO: 以前から記録されているflashのデータをmicroSDに書き込む

#if !defined(DEBUG) || defined(PITOT)
  PitotToDistributeQueue = xQueueCreate(2, sizeof(Data));
  xTaskCreateUniversal(pitot::getPitotDataTask, "getPitotDataTask", 2048, NULL, 8, &getPitotDataTaskHandle, PRO_CPU_NUM);
#else
  // TODO: 模擬データ作成タスク
#endif

  DistributeToFlashQueue = xQueueCreate(2, sizeof(u_int8_t[numof_writeData]));
  DistributeToSDQueue = xQueueCreate(2, sizeof(Data[numof_maxData]));
  xTaskCreateUniversal(cmn_task::distribute_data, "distributeData", 2048, NULL, 7, &sendDataToEveryICTaskHandle, APP_CPU_NUM);
  vTaskSuspend(sendDataToEveryICTaskHandle);

#if !defined(DEBUG) || defined(SD_FAST)
  xTaskCreateUniversal(sd_mmc::writeDataToSD, "writeDataToSD", 4096, NULL, 6, &writeDataToSDTaskHandle, APP_CPU_NUM);
  vTaskSuspend(writeDataToSDTaskHandle);
#endif

#if !defined(DEBUG) || defined(SPIFLASH)
  xTaskCreateUniversal(flash::writeDataToFlash, "writeDataToFlash", 4096, NULL, 6, &writeDataToFlashTaskHandle, PRO_CPU_NUM);
  vTaskSuspend(writeDataToFlashTaskHandle);
#endif

  // TODO: CANも実装する

  digitalWrite(led::LED1, HIGH);
  digitalWrite(led::LED2, LOW);
}

void loop() {
  vTaskDelay(10000); // cpu を使いすぎないように
}