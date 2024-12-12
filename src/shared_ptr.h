#ifndef SHARED_PTR___
#define SHARED_PTR___

#include "lib.h"

#define STRICT_MEM_MANAGER

#ifdef STRICT_MEM_MANAGER
#include<vector>

class MemoryManager{
    private:

    std::vector<void *> pointers;  // 確保したメモリのポインタを格納
    std::vector<int> deleteCounts; // 各ポインタに対するdeleteの呼び出し回数を管理

    public:
    MemoryManager();
    ~MemoryManager();
    Data* new_ptr();
    void delete_ptr(Data *p);
};

#else
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
    void delete_ptr(void *p);
};
#endif

#endif