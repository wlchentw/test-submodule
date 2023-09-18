#pragma once

#include <stdbool.h>
#include <sys/time.h>
#include <stdint.h>

#define INFINITE_TIME (0xffffffff)

#ifndef __ANDROID__
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
#else
#define NO_ERROR 0
#endif

typedef int status_t;
