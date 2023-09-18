/* SHA-256 and SHA-512 implementation based on code by Oliver Gay
 * <olivier.gay@a3.epfl.ch> under a BSD-style license. See below.
 */

/*
 * FIPS 180-2 SHA-224/256/384/512 implementation
 * Last update: 02/02/2007
 * Issue date:  04/30/2005
 *
 * Copyright (C) 2005, 2007 Olivier Gay <olivier.gay@a3.epfl.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "avb_sha.h"
#include "avb_util.h"
#include "sha256.h"
/* SHA-256 implementation */
void avb_sha256_init(AvbSHA256Ctx* ctx) {
    struct sha256_context* s_ctx = (struct sha256_context*)ctx;
    if(sizeof(struct sha256_context)>sizeof(AvbSHA256Ctx))
    {
        avb_print("ERROR:sizeof(struct sha256_context)>sizeof(AvbSHA256Ctx)\n");
        while(1);
    }
    avb_memset(s_ctx, 0, sizeof(struct sha256_context));
    sha256_start(s_ctx);
}

void avb_sha256_update(AvbSHA256Ctx* ctx, const uint8_t* data, uint32_t len) {
    sha256_process((struct sha256_context*)ctx,data,len);
}

uint8_t* avb_sha256_final(AvbSHA256Ctx* ctx) {
  uint8_t* output = avb_malloc(AVB_SHA256_DIGEST_SIZE);
  sha256_end((struct sha256_context*)ctx,output);
  return output;
}

