#include <cstdint>
#include <cstring>
#include <iostream>

union Data
{
    uint32_t array32[32]; // 32要素の配列
    uint8_t array8[128];  // 128要素のuint8_t配列
};

int main()
{
    Data data;

    // 32要素のuint32_t配列に値を設定
    for (uint32_t i = 0; i < 32; ++i)
    {
        data.array32[i] = i;
    }

    // 同じメモリをuint8_t配列としてアクセス
    for (uint32_t i = 0; i < 128; ++i)
    {
        std::cout << static_cast<int>(data.array8[i]) << " ";
    }

    for (int i = 0; i < 32; ++i)
    {
        std::cout << data.array32[i] << std::endl;
    }

    return 0;
}
