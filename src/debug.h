#ifndef _debug_
#define _debug_
#include "lib.h"

// pr_debug("%s(%d)", __FILE__, __LINE__);

#ifdef DEBUG
#define pr_debug(fmt, ...)           \
  Serial.printf(fmt, ##__VA_ARGS__); \
  Serial.println();
#else
#define pr_debug(fmt, ...)
#endif

//*
// なるべく<Arduino.h>をincludeしたくないので、、、
// do while(0) で展開箇所依存を減らす
#define error_log(format, ...)                                            \
  do                                                                      \
  {                                                                       \
    char *buffer = new char[AllbufferSize];                               \
    int written = snprintf(buffer, AllbufferSize, format, ##__VA_ARGS__); \
    if (written < 0 || written >= AllbufferSize)                          \
    {                                                                     \
      delete[] buffer;                                                    \
      char *buffer = new char[written + 1];                               \
      int re_written = snprintf(buffer, written, format, ##__VA_ARGS__);  \
      if (written < 0 || re_written >= written)                           \
      {                                                                   \
        pr_debug("failed to make error log");                             \
        pr_debug(format, ##__VA_ARGS__);                                  \
        break;                                                            \
        ;                                                                 \
      }                                                                   \
    }                                                                     \
    SD_Data *data_wrapper = new SD_Data;                                  \
    data_wrapper->is_log = true;                                          \
    data_wrapper->data = buffer;                                          \
    if (xQueueSend(ParityToSDQueue, &data_wrapper, 0) != pdPASS)          \
    {                                                                     \
      delete[] buffer;                                                    \
      pr_debug("failed to send error log");                               \
      pr_debug(format, ##__VA_ARGS__);                                    \
      break;                                                              \
    }                                                                     \
  } while (0)
;

/*/
#include "task_queue.h"
#include <Arduino.h>
template <class... Args>
void error_log(const char *format, Args... args)
{
  do
  {
    char *buffer = new char[bufferSize];
    int written = snprintf(buffer, bufferSize, format, args...);
    if (written < 0 || written >= bufferSize)
    {
      delete[] buffer;
      buffer = new char[written + 1];
      int written = snprintf(buffer, written, format, args...);
      if (written < 0 || written >= bufferSize)
      {
        pr_debug("Failed to make error log");
        pr_debug(format, args...);
        break;
        ;
      }
    }
    SD_Data *data_wrapper = new SD_Data;
    data_wrapper->is_log = false;
    data_wrapper->data = buffer;
    // pr_debug("%s", buffer);
    if (xQueueSend(ParityToSDQueue, &data_wrapper, 0) != pdPASS)
    {
      delete[] buffer;
      pr_debug("failed to make error log");
      pr_debug(format, args...);
      break;
    }
  } while (0);
} //*/
#endif