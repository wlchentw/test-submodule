/*
 * LZ4 - Fast LZ compression algorithm
 * Copyright (C) 2011-2012, Yann Collet.
 * BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You can contact the author at :
 * - LZ4 homepage : http://fastcompression.blogspot.com/p/lz4.html
 * - LZ4 source repository : http://code.google.com/p/lz4/
 *
 *  Changed for kernel use by:
 *  Chanho Min <chanho.min@lge.com>
 */

#include <debug.h>
#include <string.h>
#include <sys/types.h>
#include <lib/decompress.h>

static inline __u16 __le16_to_cpup(const __le16 *p)
{
    return (__force __u16)*p;
}

static inline u16 get_unaligned_le16(const void *p)
{
    return le16_to_cpup((__le16 *)p);
}

static inline __u32 __le32_to_cpup(const __le32 *p)
{
    return (__force __u32)*p;
}

static inline u32 get_unaligned_le32(const void *p)
{
    return le32_to_cpup((__le32 *)p);
}

static int lz4_uncompress(const char *source, char *dest, int osize)
{
    const BYTE *ip = (const BYTE *) source;
    const BYTE *ref;
    BYTE *op = (BYTE *) dest;
    BYTE *const oend = op + osize;
    BYTE *cpy;
    unsigned int token;
    size_t length;

    while (1) {

        /* get runlength */
        token = *ip++;
        length = (token >> ML_BITS);
        if (length == RUN_MASK) {
            size_t len;

            len = *ip++;
            for (; len == 255; length += 255)
                len = *ip++;
            if (unlikely(length > (size_t)(length + len))) {
                goto _output_error;
            }
            length += len;
        }

        /* copy literals */
        cpy = op + length;
        if (unlikely(cpy > oend - COPYLENGTH)) {
            /*
             * Error: not enough place for another match
             * (min 4) + 5 literals
             */
            if (cpy != oend) {
                goto _output_error;
            }

            memcpy(op, ip, length);
            ip += length;
            break; /* EOF */
        }
        LZ4_WILDCOPY(ip, op, cpy);
        ip -= (op - cpy);
        op = cpy;

        /* get offset */
        LZ4_READ_LITTLEENDIAN_16(ref, cpy, ip);
        ip += 2;

        /* Error: offset create reference outside destination buffer */
        if (unlikely(ref < (BYTE *const) dest)) {
            goto _output_error;
        }

        /* get matchlength */
        length = token & ML_MASK;
        if (length == ML_MASK) {
            for (; *ip == 255; length += 255)
                ip++;
            if (unlikely(length > (size_t)(length + *ip))) {
                goto _output_error;
            }
            length += *ip++;
        }

        /* copy repeated sequence */
        if (unlikely((op - ref) < STEPSIZE)) {
#if LZ4_ARCH64
            int dec64 = dec64table[op - ref];
#else
            const int dec64 = 0;
#endif
            op[0] = ref[0];
            op[1] = ref[1];
            op[2] = ref[2];
            op[3] = ref[3];
            op += 4;
            ref += 4;
            ref -= dec32table[op-ref];
            PUT4(ref, op);
            op += STEPSIZE - 4;
            ref -= dec64;
        } else {
            LZ4_COPYSTEP(ref, op);
        }
        cpy = op + length - (STEPSIZE - 4);
        if (cpy > (oend - COPYLENGTH)) {

            /* Error: request to write beyond destination buffer */
            if (cpy > oend) {
                goto _output_error;
            }
#if LZ4_ARCH64
            if ((ref + COPYLENGTH) > oend) {
#else
            if ((ref + COPYLENGTH) > oend ||
                    (op + COPYLENGTH) > oend) {
#endif
                goto _output_error;
            }
            LZ4_SECURECOPY(ref, op, (oend - COPYLENGTH));
            while (op < cpy)
                *op++ = *ref++;
            op = cpy;
            /*
             * Check EOF (should never happen, since last 5 bytes
             * are supposed to be literals)
             */
            if (op == oend) {
                goto _output_error;
            }
            continue;
        }
        LZ4_SECURECOPY(ref, op, cpy);
        op = cpy; /* correction */
    }

    /* end of decoding */
    return (int) (((char *)ip) - source);

    /* write overflow error detected */
_output_error:
    return -ELZ4WO;
}

static int lz4_decompress(const unsigned char *src, size_t *src_len,
                          unsigned char *dest, size_t actual_dest_len)
{
    int input_len = 0;

    input_len = lz4_uncompress((const char *)src, (char *)dest, actual_dest_len);

    if (input_len < 0)
        return -ELZ4IR;

    /* decode successfully */
    *src_len = input_len;

    return LZ4_OK;
}

int unlz4(const void *input, long in_len, void *output)
{
    int ret = -1;
    u8 *inp, *outp;
    size_t chunksize = 0, dest_len;
    size_t uncomp_chunksize = LZ4_DEFAULT_UNCOMPRESSED_CHUNK_SIZE;
    size_t out_len = get_unaligned_le32(input + in_len);
    long size = in_len;

    if (output) {
        outp = output;
    } else {
        dprintf(ALWAYS, "unlz4 error: NULL output pointer\n");
        return -ELZ4NP;
    }

    if (input) {
        inp = (u8 *) input;
    } else {
        dprintf(ALWAYS, "unlz4 error: NULL input pointer\n");
        return -ELZ4NP;
    }

    chunksize = get_unaligned_le32(inp);
    if (chunksize == ARCHIVE_MAGICNUMBER) {
        inp += 4;
        size -= 4;
    } else {
        dprintf(ALWAYS, "unlz4 error: invalid header\n");
        return -ELZ4IH;
    }

    for (;;) {

        chunksize = get_unaligned_le32(inp);
        if (chunksize == ARCHIVE_MAGICNUMBER) {
            inp += 4;
            size -= 4;
            continue;
        }

        inp += 4;
        size -= 4;

        if (out_len >= uncomp_chunksize) {
            dest_len = uncomp_chunksize;
            out_len -= dest_len;
        } else {
            dest_len = out_len;
        }

        ret = lz4_decompress(inp, &chunksize, outp, dest_len);
        if (ret < 0) {
            dprintf(ALWAYS, "unlz4 error: decodes failure\n");
            return -ELZ4DF;
        }

        if (output)
            outp += dest_len;

        size -= chunksize;

        if (size == 0) // Note: here should append size info at the end
            break;
        else if (size < 0) {
            dprintf(ALWAYS, "unlz4 error: data corrupt\n");
            return -ELZ4DC;
        }

        inp += chunksize;
    }

    return LZ4_OK;
}
