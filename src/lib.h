#ifndef _lib_
#define _lib_

#include <stdint.h>

#define DEBUG // debug時利用

#ifdef DEBUG // debug時に機能のON/OFFを切り替え

// #define CAN_MCP2562
// #define PITOT
//#define SD_FAST
// #define SPIFLASH

#endif

// distribute_dataから一度に送信するデータの個数
// SPIFlashが8bit* 256の配列単位で読み書きするため、
// Pitot (float型(32bit) * 2) のデータを32個単位で読み書きすることにした。
constexpr int numof_maxData = 32;
// SPIFlashが読み書きするデータの単位
constexpr int numof_writeData = 256;

// 必要なバッファサイズを計算する
// 各行が最大で21文字 + 終端の '\0'
constexpr int bufferSize = numof_maxData * (10 + 1 + 10 + 1) * 2 + 1; // 673

namespace pitot
{ // ピトー管
    const int SDA = 21;
    const int SCL = 22;
}

namespace flash
{ // SPI_flash
    const int CS = 5;
    const int CLK = 18;
    const int MOSI = 23;
    const int MISO = 19;
}

namespace sd_mmc
{ // SD_MMC
    const int CLK = 14;
    const int DAT0 = 2;
    const int DAT1 = 4;
    const int DAT2 = 12;
    const int DAT3 = 13;
    const int CMD = 15;
};

namespace can
{ // CAN
    const int TX = 25;
    const int RX = 26;
    const int SELECT = 33;
}

namespace led
{ // LED
    const int LED1 = 27;
    const int LED2 = 32;
}

namespace debug
{ // debug
    const int DEVINPUT = 17;
}

struct Data
{
    uint32_t time; // ESPタイマ初期化時からの経過時間 ms
    float pa;      // 圧力
    float temp;    // 温度
};

#endif