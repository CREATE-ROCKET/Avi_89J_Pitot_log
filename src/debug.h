#ifndef _debug_
#define _debug_
#include "lib.h"

#ifdef DEBUG
#define pr_debug(fmt, ...) Serial.printf((fmt), ##__VA_ARGS__); Serial.println();
#else
#define pr_debug(x)
#endif

/*
template <class... Args>
void pr_debug(Args... words)
{
#ifdef DEBUG
  Serial.printf(words...);
  Serial.println();
#endif
}
*/
#endif