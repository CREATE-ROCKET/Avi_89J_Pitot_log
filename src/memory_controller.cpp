#include "memory_controller.h"
#include "shared_ptr.h"
#include "lib.h"

MemoryManager mem_manager;

namespace mem_controller
{
    Data *new_ptr()
    {
        Data *tmp_data = mem_manager.new_ptr();
        return tmp_data;
    }
    void delete_ptr()
    {
        mem_manager.delete_ptr();
    }
} // namespace mem_controller
