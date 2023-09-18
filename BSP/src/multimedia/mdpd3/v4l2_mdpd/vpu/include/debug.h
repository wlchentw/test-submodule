#pragma once

#include <assert.h>
#include <compiler.h>
#include <platform/debug.h>
#include <stdio.h>
#ifdef __ANDROID__
#include <cutils/log.h>
#endif

#define DEBUG 0

#if defined(DEBUG)
#define DEBUGLEVEL DEBUG
#else
#define DEBUGLEVEL 0
#endif

/* debug levels */
#define CRITICAL 0
#define ALWAYS 0
#define INFO 1
#define SPEW 2

#if __ANDROID__
#define dprintf(level, x...) do { if ((level) <= DEBUGLEVEL) { ALOGE(x); } } while (0)
#else
#define dprintf(level, x...) do { if ((level) <= DEBUGLEVEL) { fprintf(stderr, x); } } while (0)
#endif
