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
#include <esp_task_wdt.h>
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include "task.h"
#include "panic_wrapper.h"
#include "common_task.h"
#include "interfaces/pitot.h"
#include "interfaces/SD_fast.h"
#include "interfaces/flash.h"
#include "interfaces/CAN_MCP2562.h"
#include "freertos/task.h"

// task handle
TaskHandle_t getPitotDataTaskHandle;      // pitotからデータを取得し、それをsendDataToEveryICへ送る 優先度 -1
TaskHandle_t sendDataToEveryICTaskHandle; // getPitotDataから得たデータを各端末に送る
TaskHandle_t writeDataToFlashTaskHandle;  // SPI_Flashにデーターを書き込む
TaskHandle_t makeParityTaskHandle;        // sendDataToEveryICから受け取ったデータを加工してwriteDataToSDに送る
TaskHandle_t writeDataToSDTaskHandle;     // makeParityから受け取ったデータやログをMicroSDに書き込む
TaskHandle_t sendDataByCanTaskHandle;     // sendDataToEveryICから受け取ったデータを送る

// queue handle
QueueHandle_t PitotToDistributeQueue;  // PitotからsendDataにデータを渡すQueue
QueueHandle_t DistributeToFlashQueue;  // sendDataからFlashにデータを渡すQueue
QueueHandle_t DistributeToParityQueue; // sendDataからParityにデータを渡すQueue
QueueHandle_t ParityToSDQueue;         // ParityからSDにデータを渡すQueue
QueueHandle_t DistributeToCanQueue;    // sendDataからCanにデータを渡すQueue

// 初期化が成功したかどうかを表す
// pitotが0
// SDが1
// flashが2
// CANが3
uint8_t IsInitSuccess = 0;

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

void pr_reset_reason()
{
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason)
  {
  case ESP_RST_UNKNOWN:
    Serial.println("Reset reason can not be determined");
    break;
  case ESP_RST_POWERON:
    Serial.println("Reset due to power-on event");
    break;
  case ESP_RST_EXT:
    Serial.println("Reset by external pin (not applicable for ESP32)");
    break;
  case ESP_RST_SW:
    Serial.println("Software reset via esp_restart");
    break;
  case ESP_RST_PANIC:
    Serial.println("Software reset due to exception/panic");
    break;
  case ESP_RST_INT_WDT:
    Serial.println("Reset (software or hardware) due to interrupt watchdog");
    break;
  case ESP_RST_TASK_WDT:
    Serial.println("Reset due to task watchdog");
    break;
  case ESP_RST_WDT:
    Serial.println("Reset due to other watchdogs");
    break;
  case ESP_RST_DEEPSLEEP:
    Serial.println("Reset after exiting deep sleep mode");
    break;
  case ESP_RST_BROWNOUT:
    Serial.println("Brownout reset (software or hardware)");
    break;
  case ESP_RST_SDIO:
    Serial.println("Reset over SDIO");
    break;
  default:
    break;
  }
}

void vApplicationStackOverflowHook(TaskHandle_t *xTask, portCHAR *taskname)
{
  error_log("Stack Over Flow Detected!!!: %s", taskname);
}

#endif

// loop関数のスタックサイズを決める関数
size_t getArduinoLoopTaskStackSize(void)
{
  return 8192; // weak attribute で定義されているため再定義している
}

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
  Serial.println("plz press some key");
  while (true)
  {
    if (Serial.available())
      break;
    delay(10);
  }

  Serial.println("Debug mode is on.");
  pr_feature_fg();
  pr_reset_reason();

  Serial.printf("Chip model: %s, %u MHZ, %hu cores\n", ESP.getChipModel(), ESP.getCpuFreqMHz(), ESP.getChipCores());
  Serial.printf("Free heap: %u / %u %u max block\n", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
  Serial.printf("Free PSRAM: %u / %u %u max block\n", ESP.getFreePsram(), ESP.getPsramSize(), ESP.getMaxAllocPsram());
#endif
#if IS_S3
  pinMode(led::LED, OUTPUT);
  pinMode(led::LED_PITOT, OUTPUT);
  pinMode(led::LED_FLASH, OUTPUT);
  pinMode(led::LED_SD, OUTPUT);
  pinMode(led::LED_CAN, OUTPUT);
  pinMode(debug::DEBUG_INPUT1, INPUT);
  pinMode(debug::DEBUG_INPUT2, INPUT);
  digitalWrite(led::LED, HIGH);
  digitalWrite(led::LED_PITOT, LOW);
  digitalWrite(led::LED_SD, LOW);
  digitalWrite(led::LED_FLASH, LOW);
  digitalWrite(led::LED_CAN, LOW);
#else
  pinMode(led::LED1, OUTPUT);
  pinMode(led::LED2, OUTPUT);
  pinMode(debug::DEBUG_INPUT, INPUT);
  digitalWrite(led::LED1, LOW);
  digitalWrite(led::LED2, HIGH);
#endif

  // Queue作成 error_logを利用したいので上に置いてる
  PitotToDistributeQueue = xQueueCreate(20, sizeof(Data *));
  ParityToSDQueue = xQueueCreate(30, sizeof(char *));
  DistributeToFlashQueue = xQueueCreate(5, sizeof(u_int8_t *));
  DistributeToParityQueue = xQueueCreate(5, sizeof(Data *));
  DistributeToCanQueue = xQueueCreate(5, sizeof(uint8_t *));
  error_log("init: %lld", esp_timer_get_time());
  error_t why_reset = esp_reset_reason();
  if (ESP_RST_PANIC == why_reset)
  {
    cmn_task::blinkLED_start(1, 1000);
    char backtrace_str[1024];
    uint16_t offset = 0;
    offset += sprintf(backtrace_str + offset, "Backtrace: ");
    for (uint8_t i = 0; i < STACK_DEPTH; i++)
    {
      if (s_exception_info.pc[i] != 0 && s_exception_info.sp[i] != 0)
      {
        offset += sprintf(backtrace_str + offset, "0x%08x:0x%08x ", (unsigned int)s_exception_info.pc[i], (unsigned int)s_exception_info.sp[i]);
      }
    }
    error_log("esp was reset due to panic\nBacktrace: %s\n", backtrace_str);
  }
  else if (ESP_RST_SW == why_reset)
  {
    cmn_task::blinkLED_start(1, 1000);
    error_log("restart due to esp_restart at %lldms", esp_timer_get_time());
  }

#if !defined(DEBUG) || defined(CAN_MCP2562)
  result = can::init();
  if (result)
  {
    error_log("can_init failed: %d", result);
  }
  else
  {
#if IS_S3
    digitalWrite(led::LED_CAN, HIGH);
#endif
    xTaskCreateUniversal(can::sendDataByCAN, "sendDataByCAN", 8192, NULL, 6, &sendDataByCanTaskHandle, APP_CPU_NUM);
    IsInitSuccess += 1 << 3;
  }
  can::canSend('i');
#endif
#if !defined(DEBUG) || defined(SD_FAST)
  result = sd_mmc::init();
  if (result)
  {
    pr_debug("SD_init failed: %d", result);
    ++error_num;
  }
  else
  {
#if IS_S3
    digitalWrite(led::LED_SD, HIGH);
#endif
    result = sd_mmc::makeNewFile();
    if (result)
    {
      pr_debug("Can't make new file: %d", result);
    }
    else
    {
#if IS_S3
      attachInterrupt(digitalPinToInterrupt(debug::DEBUG_INPUT1), sd_mmc::onButton, RISING);
#else
      attachInterrupt(digitalPinToInterrupt(debug::DEBUG_INPUT), sd_mmc::onButton, RISING);
#endif
      xTaskCreateUniversal(sd_mmc::writeDataToSD, "writeDataToSD", 8192, NULL, 6, &writeDataToSDTaskHandle, APP_CPU_NUM);
      IsInitSuccess += 1 << 1;
    }
  }
#endif

#if !defined(DEBUG) || defined(SPIFLASH)
  result = flash::init();
  if (result)
  {
    error_log("flash_init failed: %d", result);
    ++error_num;
  }
  else
  {
#if IS_S3
    digitalWrite(led::LED_FLASH, HIGH);
#endif
    xTaskCreateUniversal(flash::writeDataToFlash, "writeDataToFlash", 8192, NULL, 6, &writeDataToFlashTaskHandle, PRO_CPU_NUM);
    IsInitSuccess += 1 << 2;
  }
#endif

#if !defined(DEBUG) || defined(PITOT)
#if IS_S3
  digitalWrite(led::LED_PITOT, HIGH);
#endif
  xTaskCreateUniversal(pitot::getPitotData, "getPitotDataTask", 4096, NULL, 8, &getPitotDataTaskHandle, PRO_CPU_NUM);
  IsInitSuccess += 1;
#else
  xTaskCreateUniversal(task::createData, "createDataForTest", 2048, NULL, 8, &getPitotDataTaskHandle, PRO_CPU_NUM);
#endif

  if (error_num >= 2)
  {
    error_log("fatal error occurred!! rebooting ....");
    esp_restart(); // 再起動する
  }
  pr_debug("done all init");

  xTaskCreateUniversal(sd_mmc::makeParity, "makeParity", 8192, NULL, 6, &makeParityTaskHandle, APP_CPU_NUM);

  xTaskCreateUniversal(task::distribute_data, "distributeData", 8192, NULL, 7, &sendDataToEveryICTaskHandle, APP_CPU_NUM);

  pr_debug("all done!!!");

#if IS_S3
  digitalWrite(led::LED, LOW);
#else
  digitalWrite(led::LED1, HIGH);
  digitalWrite(led::LED2, LOW);
#endif
}

void loop()
{
#ifndef DEBUG
  vTaskDelay(10000); // cpu を使いすぎないように
#else
  vTaskDelay(1000);
  unsigned int stackHighWaterMark;

  stackHighWaterMark = uxTaskGetStackHighWaterMark(getPitotDataTaskHandle);
  error_log("[LOG] remain of getPitotData stack is: %u\n", stackHighWaterMark);

  stackHighWaterMark = uxTaskGetStackHighWaterMark(sendDataToEveryICTaskHandle);
  error_log("[LOG] remain of sendDataToEveryIC stack is: %u\n", stackHighWaterMark);

  stackHighWaterMark = uxTaskGetStackHighWaterMark(writeDataToFlashTaskHandle);
  error_log("[LOG] remain of writeDataToFlash stack is: %u\n", stackHighWaterMark);

  stackHighWaterMark = uxTaskGetStackHighWaterMark(makeParityTaskHandle);
  error_log("[LOG] remain of makeParity stack is: %u\n", stackHighWaterMark);

  stackHighWaterMark = uxTaskGetStackHighWaterMark(writeDataToSDTaskHandle);
  error_log("[LOG] remain of writeDataToSD stack is: %u\n", stackHighWaterMark);

  stackHighWaterMark = uxTaskGetStackHighWaterMark(sendDataByCanTaskHandle);
  error_log("[LOG] remain of sendDataByCan stack is: %u\n", stackHighWaterMark);

#endif
}