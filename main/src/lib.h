#ifndef _lib_
#define _lib_

#include <sdkconfig.h> // どのboardを使ってるか
#include <stdint.h>
#include <task_queue.h>

#define DEBUG // debug時利用

#ifdef DEBUG // debug時に機能のON/OFFを切り替え

#define CAN_MCP2562
#define PITOT
#define SD_FAST
#define SPIFLASH // SPIFFSも

#endif

#ifndef CREATE_SPECIFY_ESP32
#define CREATE_SPECIFY_ESP32

#ifdef CONFIG_IDF_TARGET_ESP32S3
#define IS_S3 1
#elif CONFIG_IDF_TARGET_ESP32
#define IS_S3 0
#else
#error "No supported board specified!!!"
#endif

#endif

#if IS_S3

namespace pitot
{ // ピトー管
    const int SDA = 5;
    const int SCL = 4;
}

namespace flash
{ // SPI flash
    const int CS = 18;
    const int CLK = 3;
    const int MOSI = 9;
    const int MISO = 8;
}

namespace sd_mmc
{ // SD_MMC
    const int CLK = 13;
    const int DAT0 = 11;
    const int DAT1 = 10;
    const int DAT2 = 48;
    const int DAT3 = 47;
    const int CMD = 21;
}

namespace can
{ // CAN
    const int TX = 41;
    const int RX = 42;
    const int SELECT = 38;
}

namespace led
{ // LED
    const int LED = 6;
    const int LED_PITOT = 7;
    const int LED_SD = 15;
    const int LED_FLASH = 16;
    const int LED_CAN = 17;
}

namespace debug
{                               // ピンヘッダ 両方ともPULLDOWNされてる
    const int DEBUG_INPUT1 = 1; // SDのclose用に利用する
    const int DEBUG_INPUT2 = 2;
}

#else
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
#endif

struct Data
{
    int64_t time; // ESPタイマ初期化時からの経過時間 ms
    float pa;     // 圧力
    float temp;   // 温度
};

enum data_type : uint8_t
{
    data_type_data,      // SDに書き込むデータ
    data_type_log,       // SDに書き込むログ
    data_type_spi_flash, // spi_flashに書き込まれているデータ
    data_type_SPIFFS,    // 内蔵flashに書き込まれているデータ
    // これ以上の数はSPIFFSのファイル名として扱われているため注意!!!
};

struct SD_Data
{
    uint8_t type;
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
constexpr int bufferSize = 40;
constexpr int AllbufferSize = numof_maxData * bufferSize + 1;

extern uint8_t IsInitSuccess; // 初期化が成功したかどうかを表す

#endif