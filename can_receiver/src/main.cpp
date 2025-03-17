#include <CANCREATE.h>
#include <Arduino.h>

#define CAN_RX 17 // CAN ICのTXに接続しているピン
#define CAN_TX 18 // CAN ICのRXに接続しているピン

CAN_CREATE CAN(true); // 旧ライブラリ互換かどうか決める trueで新ライブラリ用になる

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(1000);

  Serial.println("CAN Sender");
  // start the CAN bus at 100 kbps
  if (CAN.begin(100E3, CAN_RX, CAN_TX, 10))
  {
    Serial.println("Starting CAN failed!");
    while (1)
      ;
  }
}

void loop()
{
  if (CAN.available())
  {
    char Data[9]; // 最大8文字+改行文字が送信される
    if (CAN.readLine(Data))
    {
      Serial.println("failed to get CAN data");
    }
    else
      Serial.printf("Can received!!!: %s\r\n", Data);
  }
  if (Serial.available())
  {
    char data = Serial.peek(); // どの文字が入力されたかを確認(取り出さない)
    if (data == '\n' || data == '\r')
    {
      Serial.read(); // 改行文字だったら取り出して捨てる
      return;
    }
    char cmd[9] = {};
    for (int i = 0; i < 8; i++)
    {
      while (!Serial.available())
        ;
      char read = Serial.read();
      if (read == '\n' || read == '\r')
        break; // 改行文字だったら終了
      cmd[i] = read;
      Serial.print(read);
    }
    Serial.println();
    if (CAN.sendLine(cmd))
    {
      Serial.println("failed to send CAN data");
    }
  }
}