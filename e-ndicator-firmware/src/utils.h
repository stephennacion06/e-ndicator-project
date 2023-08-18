#ifndef UTILS_H
#define UTILS_H
#include <Arduino.h>

#define DEBUG_ENABLED 1

#if 1  == DEBUG_ENABLED
#define DEBUG_PRINT(...)     Serial.print(__VA_ARGS__)
#define DEBUG_PRINT_LN(...)  Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINT_LN(...)
#endif

#endif // UTILS_H