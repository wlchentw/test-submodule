/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _RSA_H
#define _RSA_H

#define RSA_LEN 256
#define SHA256_LEN 32

unsigned char *rsa_encryptdecrypt(const unsigned char *sig, const unsigned char *e, const unsigned char *n);

#endif
