# 89Jのピトー管基板用です

## 現在の予定
基本的にFree RTOSでのタスク機能を利用して
 - ピトー管からデーターを得るタスク
 - それを他に送るタスク(CAN、 SPIFlash、 パリティ)
 - CANを送信するタスク
 - SPIFlashに書き込むタスク
 - パリティを追加するタスク
 - それをMicroSDに書き込むタスク
の5つをタスクとして動作させたい。

```mermaid
flowchart TB
  node_1("getPitotData")
  node_2("sendDataToEveryIC")
  node_3("writeDataToFlash")
  node_4("sendDataByCan")
  node_5("makeParity")
  node_6("writeDataToSD")
  node_2 --> node_4
  node_2 --> node_3
  node_1 --> node_2
  node_2 --> node_5
  node_5 --> node_6
```