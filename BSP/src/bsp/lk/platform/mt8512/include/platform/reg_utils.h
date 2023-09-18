/*
 * Copyright (c) 2019 MediaTek Inc.
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

#include <reg.h>

#define clrbits32(addr, clr_bits)   writel(readl(addr) & (~clr_bits), (addr))
#define setbits32(addr, set_bits)   writel(readl(addr) | (set_bits), (addr))
#define clrsetbits32(addr, clr_bits, set_bits) \
    writel((readl(addr) & (~clr_bits)) | (set_bits), (addr))

/* value first style for backward compatibility */
#define clrbits32_r(clr_bits, addr) clrbits32(addr, clr_bits)
#define setbits32_r(set_bits, addr) setbits32(addr, set_bits)

#define writel_r(a, v) writel(v, a)

#define writew(v, a)    (*REG16(a) = (v))
#define readw(a)        (*REG16(a))
