#include <iostream>

constexpr int numof_maxData = 32;

constexpr int bufferSize = numof_maxData * (10 + 1 + 10 + 1) + 1;

#define pr_debug(fmt, ...) \
    printf(fmt "\n", ##__VA_ARGS__);

#define error_log(format, ...)                                                      \
    do                                                                              \
    {                                                                               \
        char *buffer = new char[bufferSize];                                        \
        if (buffer)                                                                 \
        {                                                                           \
            int written = snprintf(buffer, bufferSize, format "\n", ##__VA_ARGS__); \
            if (written < 0 || written >= bufferSize)                               \
            {                                                                       \
                pr_debug("buffer overflow!!!");                                     \
                delete[] buffer;                                                    \
                pr_debug("failed to make error log");                               \
                pr_debug(format, ##__VA_ARGS__);                                    \
                break;                                                              \
            }                                                                       \
            std::cout << buffer << std::endl;                                       \
            /*                                                                      \
            if (xQueueSend(ParityToSDQueue, buffer, 0) != pdPASS)                   \
            {                                                                       \
                delete[] buffer;                                                    \
                pr_debug("failed to make error log");                               \
                pr_debug(format, ##__VA_ARGS__);                                    \
                break;                                                              \
            }*/                                                                     \
            delete[] buffer;                                                        \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            pr_debug("Failed to allocate buffer");                                  \
        }                                                                           \
    } while (0)

int main()
{
    error_log("%s(%d) failed to send PitotToDistributeQueue", __FILE__, __LINE__);
}