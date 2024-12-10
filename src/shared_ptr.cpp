#include "shared_ptr.h"
#include "debug.h"
#include "lib.h"
#include "task_queue.h"
#include <Arduino.h>
#include <vector>

// #define STRICT_MEM_MANAGER

#ifdef STRICT_MEM_MANAGER
class MemoryManager
{
public:
    // コンストラクタとデストラクタ
    MemoryManager() = default;
    ~MemoryManager()
    {
        // メモリが残っている場合、警告を表示することもできます。
        for (auto ptr : pointers)
        {
            pr_debug("Warning: Memory at %p was not properly freed.", ptr);
        }
    }

    // new_ptr メソッド - メモリを確保して返す
    Data *new_ptr()
    {
        Data *ptr = new Data;
        pointers.push_back(ptr);   // メモリポインタを管理リストに追加
        deleteCounts.push_back(0); // 新しく割り当てたメモリ用にカウントを初期化
        return ptr;
    }

    void delete_ptr(Data *p)
    {
        void *p = p;
        delete_ptr(p);
    }

    void delete_ptr(uint8_t *p)
    {
        void *p = p;
        delete_ptr(p);
    }

    // delete_ptr メソッド - メモリを解放する
    void delete_ptr(void *p)
    {
        for (size_t i = 0; i < pointers.size(); ++i)
        {
            if (p == pointers[i])
            {
                deleteCounts[i]++; // 該当ポインタのdelete回数をカウント

                // delete_ptrが3回呼ばれたらメモリ解放
                if (deleteCounts[i] == 3)
                {
                    for (int i = 0; i < numof_maxData; i++)
                    {
                        if (i != 0)
                            pr_debug("Warning: Some pointers may not have been erased.");
                        delete pointers[i]; // メモリ解放
                        pointers.erase(pointers.begin() + i);
                        deleteCounts.erase(deleteCounts.begin() + i);
                    }
                }
                return;
            }
        }
        pr_debug("Pointer not found: %d", p);
    }

private:
    std::vector<Data *> pointers;  // 確保したメモリのポインタを格納
    std::vector<int> deleteCounts; // 各ポインタに対するdeleteの呼び出し回数を管理
};

#else

// counter_maxの数を求める
#ifdef SPIFLASH
const int counter_flash = 1;
#else
const int counter_flash = 0;
#endif

#ifdef CAN_MCP2562
const int counter_can = 1;
#else
const int counter_can = 0;
#endif

const int counter_max = 1 + counter_flash + counter_can;

MemoryManager::MemoryManager()
{
    deleteCounter = 0;
    oldest_ptr = new shared_ptr;
    newest_ptr = oldest_ptr;
}

Data* MemoryManager::new_ptr()
{
    Data *ptr = new Data;
    newest_ptr->ptr = ptr;
    shared_ptr *new_ptr = new shared_ptr;
    newest_ptr->next = new_ptr;
    newest_ptr = new_ptr;
    return ptr;
}

void MemoryManager::delete_ptr()
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