/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/* Originally from https://github.com/antonyantony/openssh/blob/master/cipher-chachapoly.h */

/* $OpenBSD: cipher-chachapoly.h,v 1.1 2013/11/21 00:45:44 djm Exp $ */

/*
 * Copyright (c) Damien Miller 2013 <djm@mindrot.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef CHACHA_POLY_AEAD_H
#define CHACHA_POLY_AEAD_H

#include <stdint.h>
#include "chacha.h"
#include "poly1305.h"

#define CHACHA_KEYLEN   32 /* Only 256 bit keys used here */

struct chachapoly_ctx {
        chacha_context_t main_ctx, header_ctx;
};

#ifdef __cplusplus
extern "C"
{
#endif

void chachapoly_init(struct chachapoly_ctx *ctx, const uint8_t *key, uint32_t keylen);

int chachapoly_crypt(struct chachapoly_ctx *ctx, uint32_t seqnr, uint8_t *dest, const uint8_t *src, uint32_t len, uint32_t aadlen, uint32_t authlen, int do_encrypt);

int chachapoly_get_length(struct chachapoly_ctx *ctx, uint32_t *plenp, uint32_t seqnr, const uint8_t *cp, uint32_t len);

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* CHACHA_POLY_AEAD_H */

