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

// task handle
TaskHandle_t getPitotDataTaskHandle = nullptr; // pitotからデータを取得し、それをsendDataToEveryICへ送る 優先度 -1
TaskHandle_t sendDataToEveryICTaskHandle = nullptr; // getPitotDataから得たデータを各端末に送る
TaskHandle_t writeDataToFlashTaskHandle = nullptr; // SPI_Flashにデーターを書き込む
TaskHandle_t makeParityTaskHandle = nullptr; // Micro SD記録用のデータにパリティをつける
TaskHandle_t writeDataToSDTaskHandle = nullptr; // makeParityから受け取ったデータをMicroSDに書き込む
TaskHandle_t sendDataByCanTaskHandle = nullptr; // sendDataToEveryICから受け取ったデータを送る

// queue handle
QueueHandle_t PitotToDistributeQueue;

void setup()
{
  int result;

#ifdef DEBUG
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10);
  }
  Serial.println("Debug mode is on.");
#endif

#if !defined(DEBUG) || defined(SD_FAST)
  result = sd_mmc::init();
  if (result) {
    pr_debug("SD_init failed: %d",result);
  }
#endif
#if !defined(DEBUG) || defined(SPIFLASH)
  result = flash::init();
  if (result) {
    pr_debug("flash_init failed: %d",result);
  }
#endif
#if !defined(DEBUG) || defined(PITOT)
  result = pitot::init();
  if (result) {
    pr_debug("pitot_init failed: %d",result);
  }
#endif
#if !defined(DEBUG) || defined(CAN_MCP2562)
  result = can::init();
  if (result) {
    pr_debug("can_init failed: %d",result);
  }
#endif

#if !defined(DEBUG) || defined(PITOT)
  PitotToDistributeQueue = xQueueCreate(20, sizeof(float[2]));
  xTaskCreateUniversal(pitot::getPitotDataTask, "getPitotDataTask", 2048, NULL, 5, &getPitotDataTaskHandle, 0);
#endif
}

void loop() {
  vTaskDelay(10000); // cpu を使いすぎないように
}