/**
 * \file padlock.h
 *
 *  Based on XySSL: Copyright (C) 2006-2008  Christophe Devine
 *
 *  Copyright (C) 2009  Paul Bakker <polarssl_maintainer at polarssl dot org>
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of PolarSSL or XySSL nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TROPICSSL_PADLOCK_H
#define TROPICSSL_PADLOCK_H

#include "tropicssl/aes.h"

#if (defined(__GNUC__) && defined(__i386__))

#ifndef TROPICSSL_HAVE_X86
#define TROPICSSL_HAVE_X86
#endif

#define PADLOCK_RNG 0x000C
#define PADLOCK_ACE 0x00C0
#define PADLOCK_PHE 0x0C00
#define PADLOCK_PMM 0x3000

#define PADLOCK_ALIGN16(x) (unsigned long *) (16 + ((long) x & ~15))

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * \brief          PadLock detection routine
	 *
	 * \return         1 if CPU has support for the feature, 0 otherwise
	 */
	int padlock_supports(int feature);

	/**
	 * \brief          PadLock AES-ECB block en(de)cryption
	 *
	 * \param ctx      AES context
	 * \param mode     AES_ENCRYPT or AES_DECRYPT
	 * \param input    16-byte input block
	 * \param output   16-byte output block
	 *
	 * \return         0 if success, 1 if operation failed
	 */
	int padlock_xcryptecb(aes_context * ctx,
			      int mode,
			      const unsigned char input[16],
			      unsigned char output[16]);

	/**
	 * \brief          PadLock AES-CBC buffer en(de)cryption
	 *
	 * \param ctx      AES context
	 * \param mode     AES_ENCRYPT or AES_DECRYPT
	 * \param length   length of the input data
	 * \param iv       initialization vector (updated after use)
	 * \param input    buffer holding the input data
	 * \param output   buffer holding the output data
	 *
	 * \return         0 if success, 1 if operation failed
	 */
	int padlock_xcryptcbc(aes_context * ctx,
			      int mode,
			      int length,
			      unsigned char iv[16],
			      const unsigned char *input,
			      unsigned char *output);

#ifdef __cplusplus
}
#endif
#endif				/* HAVE_X86  */
#endif				/* padlock.h */
