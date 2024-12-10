#ifndef _sd_fast_
#define _sd_fast_

namespace sd_mmc
{
    // setup
    int init();
    int makeNewFile();
    // makeParity
    void makeParity(void *pvParameter);
    // write
    void writeDataToSD(void *pvParameter);
}

#endif