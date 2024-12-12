#ifndef _MEM_CONTROLLER_
#define _MEM_CONTROLLER_

#include "lib.h"

namespace mem_controller {
    Data* new_ptr();
    void delete_ptr(Data *p);
    void delete_ptr(uint8_t *p);
}

#endif