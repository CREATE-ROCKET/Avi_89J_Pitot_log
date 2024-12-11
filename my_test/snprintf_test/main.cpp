#include <iostream>

struct Data
{
    float pa;   // 圧力
    float temp; // 温度
};

constexpr int numof_maxData = 32;

constexpr int bufferSize = numof_maxData * (10 + 1 + 10 + 1) + 1; // 673

// Data型の配列をchar型に変換する
// newしているのでdelete[]等必要
char *DataToChar(Data pitotData[numof_maxData])
{
    // バッファを動的に確保
    char *buffer = new char[bufferSize];
    if (buffer == nullptr)
    {
        return nullptr; // メモリ確保失敗時はnullptrを返す
    }

    size_t offset = 0;

    for (size_t i = 0; i < numof_maxData; ++i)
    {
        // snprintfを使って1行をフォーマット
        int written = snprintf(
            buffer + offset,
            bufferSize - offset,
            "%g %g\n",
            pitotData[i].pa,
            pitotData[i].temp);

        // バッファオーバーフローを防止
        if (written < 0 || offset + written >= bufferSize)
        {
            delete[] buffer; // メモリを解放して失敗を返す
            return nullptr;
        }

        offset += written;
    }
    buffer[offset] = '\0';

    return buffer;
}

int main()
{
    Data *test_data = new Data[numof_maxData];
    for (int i = 0; i < numof_maxData; i++)
    {
        test_data[i].pa = 12.099567;
        test_data[i].temp = 23.082558;
    }
    char *data = DataToChar(test_data);
    std::cout << data << std::endl;
}
