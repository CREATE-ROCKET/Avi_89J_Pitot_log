#ifndef SHARED_PTR___
#define SHARED_PTR___

#include "lib.h"

struct shared_ptr
{
    Data *ptr;
    shared_ptr *next;
};

class MemoryManager
{
private:
    int deleteCounter;
    shared_ptr *newest_ptr;
    shared_ptr *oldest_ptr;

public:
    MemoryManager();

    Data* new_ptr();
    void delete_ptr();
};

#endif