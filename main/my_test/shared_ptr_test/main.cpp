#include <iostream>

constexpr int numof_maxData = 32;

#define error_log(x) std::cout << x << std::endl;


struct Data
{
    float pa;   // 圧力
    float temp; // 温度
};

struct shared_ptr
{
    Data *ptr;
    shared_ptr *next;
};

// counter_maxの数を求める
const int counter_max = 3;

class MemoryManager
{
public:
    MemoryManager()
    {
        deleteCounter = 0;
        oldest_ptr = new shared_ptr;
        newest_ptr = oldest_ptr;
    }

    Data *new_ptr()
    {
        Data *ptr = new Data;
        newest_ptr->ptr = ptr;
        shared_ptr *new_ptr = new shared_ptr;
        newest_ptr->next = new_ptr;
        newest_ptr = new_ptr;
        return ptr;
    }

    void delete_ptr()
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

private:
    int deleteCounter;
    shared_ptr *newest_ptr;
    shared_ptr *oldest_ptr;
};

int main()
{
    MemoryManager data_mem_manager;
    Data* pitot_data[numof_maxData];
    for (int i = 0; i <= numof_maxData; i++)
    {
        pitot_data[i] = data_mem_manager.new_ptr();
    }

    data_mem_manager.delete_ptr();
    data_mem_manager.delete_ptr();
    data_mem_manager.delete_ptr();
    std::cout << "done" << std::endl;
}