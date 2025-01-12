#include "CAN_MCP2562.h"
#include "lib.h"
#include "debug.h"
#include <CAN.h>

#if !defined(DEBUG) || defined(CAN_MCP2562)


namespace can {
    int init() {
        pinMode(can::SELECT, OUTPUT);
        CAN.setPins(can::RX, can::TX);
        // start the CAN bus at 100kbps
        if(!CAN.begin(100E3)) {
            pr_debug("Start Can failed");
            return 1; // CAN.begin() failed
        }
        return 0;
    }

    //IRAM_ATTR void 
}

#endif