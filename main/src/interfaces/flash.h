#ifndef _spi_flash_
#define _spi_flash_

namespace flash
{
    int init();
    void writeDataToFlash(void *pvParameter);
    void eraseFlash();
}

#endif