/*
 * Copyright (c) 2020-2020 Xi Chen (xixi.chen@mediatek.com)
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
#include <debug.h>
#include <assert.h>
#include <limits.h>
#include <printf.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <platform/debug.h>

#define SSCANF_DEBUG

#define FLAG_CHAR      (0x1 << 1)
#define FLAG_SHORT      (0x1 << 2)

static int xchar_to_int(char c)
{
    if (isdigit(c))
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    else if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    else
        return 0;   // not xdigit: return 0!
}

static int get_int(const char **buf, int *p_err)
{
    const char *p = *buf;
    int i = 0;
    int err = -1;

    while(1) {
        char c = *p;

        if (isdigit(c)) {
            i = i * 10 + (c - '0');
            err = 0;        // find at least one digit

        #ifdef SSCANF_DEBUG
            printf("buf:%s, i:%d, c:%c\n", p, i, c);
        #endif
            ++p;
            continue;
        }
        else {
            if (err == -1) {
                *p_err = -1;
                return -1;
            }

            *buf = p;

            return i;
        }
    }

    return -1;
}

static int get_hex_int(const char **buf, int *p_err)
{
    const char *p = *buf;
    int i = 0;
    int err = -1;
    int found_hex_prefix = 0, ignore_hex_prefix = 0;
    int found_digit = 0;

    // ignore blank
    while (isspace(*p)) {
        ++p;
    }

    while(1) {
        char c = *p;

        if (isspace(c)) {
            *buf = p;
            break;
        }

        // hex prefix
        if (!ignore_hex_prefix && !found_hex_prefix && c == '0') {
            c = *++p;
            if (c == 'x' || c == 'X') {
                found_hex_prefix = 1;
                ignore_hex_prefix = 1;
                ++p;
                continue;
            } else if (isxdigit(c)) {
                i = i * 16 + (xchar_to_int(c));
                ignore_hex_prefix = 1;
                ++p;
            }
            else
                goto error;
        }

        if (isxdigit(c)) {
            i = i * 16 + xchar_to_int(c);
            ignore_hex_prefix = 1;
            err = 0;        // find at least one digit

        #ifdef SSCANF_DEBUG
            printf("buf:%s, i:0x%x, c:%c\n", p, i, c);
        #endif
            ++p;
            continue;
        }
        else
            goto error;
    }

    *buf = p;
    return i;

error:
    *p_err = -1;
    return -1;
}

int vsscanf(const char *buf, const char *fmt, va_list ap)
{
    int err = 0;
    char c;
    unsigned char uc;
    const char *s;
    size_t item;
    int n;
    void *ptr;
    int flags;
    unsigned int format_num;
    char signchar;
    size_t chars_written = 0;
    char num_buffer[32];
    int *p;
    char *pch;
    short *psh;
    int flag;

    s = fmt;
    item = 0;

    for (;;) {
        while ((c = *fmt++) != 0) {
            if (isspace(c))
                continue;

            if (c == '%')
                break;
            else {
                while (isspace(*buf)) {
                    ++buf;
                }
                if (c != *buf)
                    return 0;

                ++buf;
                //printf("buf change:|%s|, fmt:|%s|\n", buf, fmt);
            }
        }

        /* make sure we haven't just hit the end of the string */
        if (c == 0)
            break;

        c = *fmt++;
        if (c == 0)
            break;

format_continue:
        switch (c) {
            case 'a':   /* character interger */
                flag = FLAG_CHAR;
                c = *fmt++;
                goto format_continue;
            case 'h':
                flag = FLAG_SHORT;
                c = *fmt++;
                goto format_continue;
            case 'd':   /* interger */
                if (flag == FLAG_CHAR)
                    pch = va_arg(ap, char *);
                else if (flag == FLAG_SHORT)
                    psh = va_arg(ap, short *);
                else
                    p = va_arg(ap, int *);

                n = get_int(&buf, &err);

                if (err < 0)
                    goto exit;

                if (flag == FLAG_CHAR)
                    *pch = n;
                else if (flag == FLAG_SHORT)
                    *psh = n;
                else
                    *p = n;
                item++;
                break;

             case 'x':  /* hex interger */
             case 'X':
                p = va_arg(ap, int *);

                n = get_hex_int(&buf, &err);

                if (err < 0)
                    goto exit;

                *p = n;
                item++;
                break;

            default:
                err = -1;
                break;
        }

        continue;
    }

exit:
    return (err < 0) ? 0 : (int)item;
}


int sscanf(const char *buf, const char *fmt, ...)
{
    va_list ap;
    int err;

    va_start(ap, fmt);
    err = vsscanf(buf, fmt, ap);
    va_end(ap);

    return err;
}

