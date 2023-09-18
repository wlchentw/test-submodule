/*
 * Copyright (c) 2016 MediaTek Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

struct fastboot_cmd {
    struct fastboot_cmd *next;
    const char *prefix;
    unsigned prefix_len;
    void (*handle)(const char *arg, void *data, unsigned sz);
};

struct fastboot_var {
    struct fastboot_var *next;
    const char *name;
    const char *value;
};

void fastboot_okay(const char *info);
void fastboot_fail(const char *reason);
void fastboot_register(const char *prefix, void (*handle)(const char *arg, void *data, unsigned sz));
void fastboot_publish(const char *name, const char *value);
void fastboot_info(const char *reason);
int fastboot_init(void *base, unsigned size, int mode);

#define STATE_OFFLINE   0
#define STATE_COMMAND   1
#define STATE_COMPLETE  2
#define STATE_ERROR     3
#define STATE_RETURN    4
