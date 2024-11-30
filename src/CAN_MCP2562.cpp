#include "CAN_MCP2562.h"
#include "lib.h"
#include "debug.h"
#include <CAN.h>

#if !defined(DEBUG) || defined(CAN_MCP2562)

namespace can {
    int init() {
        // CAN.setPins(rx, tx);
        CAN.setPins(can::RX, can::TX);
        // start the CAN bus at 500kbps
        if(!CAN.begin(500E3)) {
            return 1; // CAN.begin() failed
        }
        return 0;
    }
}

#endif