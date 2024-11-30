#ifndef _lib_
#define _lib_

#define DEBUG // debug時利用

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
{ // debug
    const int DEVINPUT = 17;
}

// 以下 include
#if !defined(DEBUG) || defined(PITOT)
#include "pitot.h"
#endif

#if !defined(DEBUG) || defined(SD_FAST)
#include "SD_fast.h"
#endif

#if !defined(DEBUG) || defined(SPIFLASH)
#include "flash.h"
#endif

#if !defined(DEBUG) || defined(CAN_MCP2562)
#include "CAN_MCP2562.h"
#endif

#endif