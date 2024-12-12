#include "shared_ptr.h"
#include "lib.h"
#include "debug.h"
#include "task_queue.h"
#include <Arduino.h>

// counter_maxの数を求める
#if !defined(DEBUG) || defined(SPIFLASH)
const int counter_flash = 1;
#else
const int counter_flash = 0;
#endif

#if !defined(DEBUG) || defined(CAN_MCP2562)
const int counter_can = 1;
#else
const int counter_can = 0;
#endif

const int counter_max = 1 + counter_flash + counter_can;

#ifdef STRICT_MEM_MANAGER
#include <vector>

// コンストラクタとデストラクタ
MemoryManager::MemoryManager() = default;
MemoryManager::~MemoryManager()
{
    for (auto ptr : pointers)
    {
        pr_debug("Warning: Memory at %p was not properly freed.", ptr);
    }
}

// new_ptr メソッド - メモリを確保して返す
Data *MemoryManager::new_ptr()
{
    Data *ptr = new Data[numof_maxData];
    pointers.push_back(ptr);   // メモリポインタを管理リストに追加
    deleteCounts.push_back(0); // 新しく割り当てたメモリ用にカウントを初期化
    return ptr;
}

// delete_ptr メソッド - メモリを解放する
void MemoryManager::delete_ptr(Data *p)
{
    for (size_t i = 0; i < pointers.size(); ++i)
    {
        if (p == pointers[i])
        {
            deleteCounts[i]++; // 該当ポインタのdelete回数をカウント

            // delete_ptrが3回呼ばれたらメモリ解放
            if (deleteCounts[i] == counter_max)
            {
                if (i != 0)
                {
                    error_log("Warning: Some pointers may not have been erased.");
                }
                //pr_debug("free pointer!");
                delete p; // メモリ解放
                pointers.erase(pointers.begin() + i);
                deleteCounts.erase(deleteCounts.begin() + i);
            }
            return;
        }
    }
    pr_debug("Pointer not found: %d", p);
}

#else

MemoryManager::MemoryManager()
{
    deleteCounter = 0;
    oldest_ptr = new shared_ptr;
    newest_ptr = oldest_ptr;
}

Data *MemoryManager::new_ptr()
{
    Data *ptr = new Data;
    newest_ptr->ptr = ptr;
    shared_ptr *new_ptr = new shared_ptr;
    newest_ptr->next = new_ptr;
    newest_ptr = new_ptr;
    return ptr;
}

void MemoryManager::delete_ptr(void *p)
{
    ++deleteCounter;
    if (deleteCounter >= counter_max)
    {
        deleteCounter = 0;
        for (int i = 0; i < numof_maxData; i++)
        {
            Data *ptr = oldest_ptr->ptr;
            if (ptr == nullptr)
            {
                error_log("shared_ptr.ptr is nullptr");
                break;
            }
            shared_ptr *old_ptr = oldest_ptr->next;
            delete ptr;
            if (old_ptr == nullptr)
            {
                error_log("shared_ptr.next is nullptr");
                break;
            }
            delete oldest_ptr;
            oldest_ptr = old_ptr;
        }
        return;
    }
}

#endif