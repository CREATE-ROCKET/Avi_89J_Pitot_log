#include <iostream>
#include <cstdint>

struct Data
{
    int64_t time; // 時刻
    float pa;     // 圧力
    float temp;   // 温度
};

// SPIFlashが読み書きするデータの単位
constexpr int numof_writeData = 256;
// distribute_dataから一度に送信するデータの個数
// SPIFlashが8bit* 256の配列単位で読み書きすることから算出する
constexpr int numof_maxData = numof_writeData / (sizeof(Data) / sizeof(uint8_t));

// 必要なバッファサイズを計算する
constexpr int bufferSize = 35 + 1;
constexpr int AllbufferSize = numof_maxData * bufferSize + 1; // 865

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
            bufferSize,
            "%14lld, %8g, %8g\n",
            pitotData[i].time,
            pitotData[i].pa,
            pitotData[i].temp);

        // バッファオーバーフローを防止
        if (written < 0)
        {
            delete[] buffer; // メモリを解放して失敗を返す
            std::cout << "failed" << std::endl;
            return nullptr;
        }

        if (offset + written >= AllbufferSize)
        {
            delete[] buffer;
            std::cout << "failed 1" << std::endl;
            return nullptr;
        }

        offset += written;
    }
    buffer[offset] = '\0';

    return buffer;
}

int main()
{
    std::cout << "Hello" << std::endl;
    Data *test_data = new Data[numof_maxData];
    for (int i = 0; i < numof_maxData; i++)
    {
        test_data[i].time = 10000000000;
        test_data[i].pa = 12.099567;
        test_data[i].temp = 23.082558;
    }
    char *data = DataToChar(test_data);
    std::cout << data << std::endl;
}
