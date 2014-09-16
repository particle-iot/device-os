/*
 *  VIA PadLock support functions
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
/*
 *  This implementation is based on the VIA PadLock Programming Guide:
 *
 *  http://www.via.com.tw/en/downloads/whitepapers/initiatives/padlock/
 *  programming_guide.pdf
 */

#include "tropicssl/config.h"

#if defined(TROPICSSL_PADLOCK_C)

#include "tropicssl/aes.h"
#include "tropicssl/padlock.h"

#if defined(TROPICSSL_HAVE_X86)

#include <string.h>

/*
 * PadLock detection routine
 */
int padlock_supports(int feature)
{
	static int flags = -1;
	int ebx, edx;

	if (flags == -1) {
asm("movl  %%ebx, %0           \n" "movl  $0xC0000000, %%eax  \n" "cpuid                     \n" "cmpl  $0xC0000001, %%eax  \n" "movl  $0, %%edx           \n" "jb    unsupported         \n" "movl  $0xC0000001, %%eax  \n" "cpuid                     \n" "unsupported:              \n" "movl  %%edx, %1           \n" "movl  %2, %%ebx           \n":"=m"(ebx),
		    "=m"
		    (edx)
:		    "m"(ebx)
:		    "eax", "ecx", "edx");

		flags = edx;
	}

	return (flags & feature);
}

/*
 * PadLock AES-ECB block en(de)cryption
 */
int padlock_xcryptecb(aes_context * ctx,
		      int mode,
		      const unsigned char input[16],
		      unsigned char output[16])
{
	int ebx;
	unsigned long *rk;
	unsigned long *blk;
	unsigned long *ctrl;
	unsigned char buf[256];

	rk = ctx->rk;
	blk = PADLOCK_ALIGN16(buf);
	memcpy(blk, input, 16);

	ctrl = blk + 4;
	*ctrl = 0x80 | ctx->nr | ((ctx->nr + (mode ^ 1) - 10) << 9);

asm("pushfl; popfl         \n" "movl    %%ebx, %0     \n" "movl    $1, %%ecx     \n" "movl    %2, %%edx     \n" "movl    %3, %%ebx     \n" "movl    %4, %%esi     \n" "movl    %4, %%edi     \n" ".byte  0xf3,0x0f,0xa7,0xc8\n" "movl    %1, %%ebx     \n":"=m"(ebx)
:	    "m"(ebx), "m"(ctrl), "m"(rk), "m"(blk)
:	    "ecx", "edx", "esi", "edi");

	memcpy(output, blk, 16);

	return (0);
}

/*
 * PadLock AES-CBC buffer en(de)cryption
 */
int padlock_xcryptcbc(aes_context * ctx,
		      int mode,
		      int length,
		      unsigned char iv[16],
		      const unsigned char *input,
		      unsigned char *output)
{
	int ebx, count;
	unsigned long *rk;
	unsigned long *iw;
	unsigned long *ctrl;
	unsigned char buf[256];

	if (((long)input & 15) != 0 || ((long)output & 15) != 0)
		return (1);

	rk = ctx->rk;
	iw = PADLOCK_ALIGN16(buf);
	memcpy(iw, iv, 16);

	ctrl = iw + 4;
	*ctrl = 0x80 | ctx->nr | ((ctx->nr + (mode ^ 1) - 10) << 9);

	count = (length + 15) >> 4;

asm("pushfl; popfl         \n" "movl    %%ebx, %0     \n" "movl    %2, %%ecx     \n" "movl    %3, %%edx     \n" "movl    %4, %%ebx     \n" "movl    %5, %%esi     \n" "movl    %6, %%edi     \n" "movl    %7, %%eax     \n" ".byte  0xf3,0x0f,0xa7,0xd0\n" "movl    %1, %%ebx     \n":"=m"(ebx)
:	    "m"(ebx), "m"(count), "m"(ctrl),
	    "m"(rk), "m"(input), "m"(output), "m"(iw)
:	    "eax", "ecx", "edx", "esi", "edi");

	memcpy(iv, iw, 16);

	return (0);
}

#endif

#endif
