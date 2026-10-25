
#pragma once

#include <cstdio>

#ifdef DEBUG
#define DEBUG_PRINT(message, ...) printf(message "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif
