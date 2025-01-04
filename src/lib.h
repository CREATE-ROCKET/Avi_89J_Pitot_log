#ifndef _lib_
#define _lib_

#include <stdint.h>

//#define DEBUG // debug時利用

#ifdef DEBUG // debug時に機能のON/OFFを切り替え

#define CAN_MCP2562
#define PITOT
#define SD_FAST
#define SPIFLASH

#endif

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
{ // debug用 SD_MMCのcloseに使うことにした PULLDOWN
    const int DEBUG_INPUT = 17;
}

struct Data
{
    uint32_t time; // ESPタイマ初期化時からの経過時間 ms
    float pa;      // 圧力
    float temp;    // 温度
};

struct SD_Data
{
    bool is_log;
    char *data;
};

#define STACK_DEPTH 32
typedef struct
{
    int number;
    uint32_t pc[STACK_DEPTH];
    uint32_t sp[STACK_DEPTH];
} ExceptionInfo_t;

// SPIFlashが読み書きするデータの単位
constexpr int numof_writeData = 256;
// distribute_dataから一度に送信するデータの個数
// SPIFlashが8bit* 256の配列単位で読み書きすることから算出する
constexpr int numof_maxData = numof_writeData / (sizeof(Data) / sizeof(uint8_t));

// 必要なバッファサイズを計算する
// 各行が最大で21文字 + 終端の '\0'
constexpr int bufferSize = 12 + 2 + 6 + 2 + 6 + 1;
constexpr int AllbufferSize = numof_maxData * (bufferSize - 1) + 1; // 865

#endif