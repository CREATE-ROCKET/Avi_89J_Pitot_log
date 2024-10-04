#define DEBUG // debug時利用

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <SD.h>
#include <FS.h>
#include <S25FLx.h>
#include <ms4525do.h>

template <class... Args>
void pr_debug(Args... words)
{
#ifdef DEBUG
  Serial.printf(words...);
  Serial.println();
#endif
}

SPIClass vspi(VSPI); // microSD SPI flash 用
flash flash1; // SPI flash 用
bfs::Ms4525do

// VSPI 記録用
#define CS_1_1 15 // MicroSD
#define CS_1_2 16 // SPIflash
#define CLK_1 14
#define MOSI_1 13
#define MISO_1 12

// HSPI データ取得用


uint8_t tx[256] = {};
uint8_t rx[256] = {};
uint8_t ux[256] = {};
bool flash_equal_all = true;
bool sd_equal_all = true;

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10);
  }
  Serial.println("Debug mode is on.");
#endif
  pinMode(CS_1_1, OUTPUT);
  pinMode(CS_1_2, OUTPUT);
  digitalWrite(CS_1_1, HIGH);
  digitalWrite(CS_1_2, HIGH);

  vspi.begin(CLK_1, MISO_1, MOSI_1, CS_1_1);
  flash1.waitforit(); // use between each communication to make sure S25FLxx is ready to go.
  flash1.read_info(); //will return an error if the chip isn't wired up correctly.

  WiFi.disconnect(true); // wifiをoffにして少電力を狙ってみる
  if (!SD.begin(CS_1_1, vspi))
  {
    pr_debug("Card Mount Failed");
    return;
    //goto exit_failure;
  }

  if (SD.cardType() == CARD_NONE)
  {
    pr_debug("No SD card attached");
    return;
    //goto exit_failure;
  }
  pr_debug("SD Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));

  flash1.erase_all();
  File file1 = SD.open("/data", FILE_WRITE);
  if (!file1) {
    pr_debug("ERROR: Can't open SD file");
    return;
  }
  // 書き込むデータを作る
  for (int i = 0; i < 256; i++)
  {
    tx[i] = i;
    file1.printf("%d ",i);
  }
  file1.close();
  // 書き込み
  flash1.write(0, tx, 256); // flash1.write(0, tx); ???
  delay(1);
  // 読み込み
  flash1.read(0, rx, 256); // flash1.read(0, rx); ???
  pr_debug("from spiFlash");
  // 読み込んだデータをシリアルで表示
  for (int i = 0; i < 256; i++)
  {
    Serial.print(rx[i]);
    Serial.print(" ");
    if (tx[i] != rx[i]) {
      flash_equal_all = false;
    }
  }

  pr_debug("\nfrom SD");
  File file2 = SD.open("/data", FILE_READ);
  if (!file2) {
    pr_debug("ERROR: Can't open SD file 2");
    return;
  }
  while(file2.available()) {
    Serial.write(file2.read());
  }
  file2.close();

  if (flash_equal_all == true) {
    pr_debug("spiFlash: equal_all");
  }
  pr_debug("fin");

exit_failure:
  //*
  return;
  /*/
  pr_debug("restarting...");
  delay(10);
  ESP.restart();
  return;
  //*/
}

void loop() {}