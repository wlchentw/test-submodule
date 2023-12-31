/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <endian.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/mempool.h>
#include <assert.h>

#include "avb_sysdeps.h"

int avb_memcmp(const void* src1, const void* src2, size_t n) {
  return memcmp(src1, src2, n);
}

void* avb_memcpy(void* dest, const void* src, size_t n) {
  return memcpy(dest, src, n);
}

void* avb_memset(void* dest, const int c, size_t n) {
  return memset(dest, c, n);
}

int avb_strcmp(const char* s1, const char* s2) {
  return strcmp(s1, s2);
}

size_t avb_strlen(const char* str) {
  return strlen(str);
}

void avb_abort(void) {
  dprintf(ALWAYS, "Fatal ERROR,loop while");
  while(1);
}

void avb_print(const char* message) {
  dprintf(ALWAYS, "%s", message);
}


void avb_printv(const char* message, ...) {
  va_list ap;
  const char* m;

  va_start(ap, message);
  for (m = message; m != NULL; m = va_arg(ap, const char*)) {
    dprintf(ALWAYS, "%s", m);
  }
  va_end(ap);
}

void* avb_malloc_(size_t size) {
  return mempool_alloc(size,MEMPOOL_ANY);
}

void avb_free(void* ptr) {
  mempool_free(ptr);
}
