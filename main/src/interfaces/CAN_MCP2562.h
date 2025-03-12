#ifndef _can_MCP2562_
#define _can_MCP2562_

namespace can
{
    int init();
    void sendDataByCAN(void *pvParameter);
    void canReceive(void *pvParameter);
}

#endif