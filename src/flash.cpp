#include <lib.h>
#include "debug.h"
#include "S25FL127S 1.0.0/src/SPIflash.h"

#if !defined(DEBUG) || defined(SPIFLASH)

#define SPIFREQ 5000000

SPICREATE::SPICreate SPIC1;
Flash flash1;

namespace flash
{
    // setup内で実行 エラー処理なし
    int init()
    {
        digitalWrite(CS, HIGH);
        SPIC1.begin(CS, CLK, MISO, MOSI);
        flash1.begin(&SPIC1, CS, SPIFREQ);
#ifdef DEBUG
        flash1.erase();
#endif
        return 0;
    }
}

#endif