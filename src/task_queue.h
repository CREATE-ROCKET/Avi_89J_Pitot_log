#ifndef _task_and_queue_
#define _task_and_queue_

#include <Arduino.h>

// task handle
extern TaskHandle_t getPitotDataTaskHandle; // pitotからデータを取得し、それをsendDataToEveryICへ送る 優先度 -1
extern TaskHandle_t sendDataToEveryICTaskHandle; // getPitotDataから得たデータを各端末に送る
extern TaskHandle_t writeDataToFlashTaskHandle; // SPI_Flashにデーターを書き込む
extern TaskHandle_t makeParityTaskHandle; // sendDataToEveryICから受け取ったデータを加工してwriteDataToSDに送る
extern TaskHandle_t writeDataToSDTaskHandle; // makeParityから受け取ったデータやログをMicroSDに書き込む
extern TaskHandle_t sendDataByCanTaskHandle; // sendDataToEveryICから受け取ったデータを送る

// queue handle
extern QueueHandle_t PitotToDistributeQueue; // PitotからsendDataにデータを渡すQueue
extern QueueHandle_t DistributeToFlashQueue; // sendDataからFlashにデータを渡すQueue
extern QueueHandle_t DistributeToParityQueue; // sendDataからParityにデータを渡すQueue
extern QueueHandle_t ParityToSDQueue;    // ParityからSDにデータを渡すQueue

#endif