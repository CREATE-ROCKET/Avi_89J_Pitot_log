#ifndef _debug_
#define _debug_
#include "lib.h"

#ifdef DEBUG
#define pr_debug(fmt, ...) \
  Serial.printf(fmt "\n", ##__VA_ARGS__);
#else
#define pr_debug(fmt, ...)
#endif

// なるべく<Arduino.h>をincludeしたくないので、、、
// do while(0) で展開箇所依存を減らす
#define error_log(format, ...)                                                \
  do                                                                          \
  {                                                                           \
    char *buffer = new char[bufferSize];                                      \
    if (buffer)                                                               \
    {                                                                         \
      int written = snprintf(buffer, bufferSize, format "\n", ##__VA_ARGS__); \
      if (written < 0 || written >= bufferSize)                               \
      {                                                                       \
        pr_debug("buffer overflow!!!");                                       \
        delete[] buffer;                                                      \
        pr_debug("failed to make error log");                                 \
        pr_debug(format, ##__VA_ARGS__);                                      \
        break;                                                                \
      }                                                                       \
      if (xQueueSend(ParityToSDQueue, buffer, 0) != pdPASS)                   \
      {                                                                       \
        delete[] buffer;                                                      \
        pr_debug("failed to make error log");                                 \
        pr_debug(format, ##__VA_ARGS__);                                      \
        break;                                                                \
      }                                                                       \
      delete[] buffer;                                                        \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      pr_debug("Failed to allocate buffer");                                  \
    }                                                                         \
  } while (0)

/*
#include "task_queue.h"
#include <Arduino.h>
template <class... Args>
void error_log(const char* format, Args... args)
{
  char *buffer = new char[bufferSize];
  int written = snprintf(buffer, bufferSize, format, args...);
  if (written < 0 || written > bufferSize)
  {
    pr_debug("buffer overflow!!!");
    delete[] buffer;
    pr_debug("failed to make error log");
    pr_debug(format, args...);
  }
  if (xQueueSend(ParityToSDQueue, buffer, 0) != pdPASS)
  {
    delete[] buffer;
    pr_debug("failed to make error log");
    pr_debug(format, args...);
  }
}*/
#endif