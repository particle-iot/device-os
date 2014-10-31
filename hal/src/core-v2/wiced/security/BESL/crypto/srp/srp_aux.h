/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
/*
 * Copyright (c) 1997-2007  The Stanford SRP Authentication Project
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL STANFORD BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
 * THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Redistributions in source or binary form must retain an intact copy
 * of this copyright notice.
 */

#ifndef SRP_AUX_H
#define SRP_AUX_H

#include "cstr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BigInteger abstraction API */

#ifndef MATH_PRIV
typedef struct
{
        void* pad1;
        void* pad2;
        void* pad3;
}BigInteger;
#endif
/*
 * Some functions return a BigIntegerResult.
 * Use BigIntegerOK to test for success.
 */
#define BIG_INTEGER_SUCCESS 0
#define BIG_INTEGER_ERROR -1
#define BigIntegerOK(v) ((v) == BIG_INTEGER_SUCCESS)
typedef int BigIntegerResult;

void             BigIntegerFromInt   ( BigInteger* a, unsigned int n );
int              BigIntegerBitLen    ( BigInteger* b );
int              BigIntegerCmp       ( BigInteger* c1, BigInteger* c2 );
BigIntegerResult BigIntegerFromBytes ( BigInteger* a, const unsigned char* bytes, int length );
int              BigIntegerToBytes   ( BigInteger* src, unsigned char* dest, int destlen );
int              BigIntegerByteLen   ( BigInteger* b );
BigIntegerResult BigIntegerToCstr    ( BigInteger* x, cstr * out );
int              BigIntegerCmpInt    ( BigInteger* c1, unsigned int c2 );
BigIntegerResult BigIntegerAdd       ( BigInteger* result, BigInteger* a1, BigInteger* a2 );
BigIntegerResult BigIntegerAddInt    ( BigInteger* result, BigInteger* a1, unsigned int a2 );
BigIntegerResult BigIntegerSub       ( BigInteger* result, BigInteger* s1, BigInteger* s2 );
BigIntegerResult BigIntegerSubInt    ( BigInteger* result, BigInteger* s1, unsigned int s2 );
BigIntegerResult BigIntegerMul       ( BigInteger* result, BigInteger* m1, BigInteger* m2 );
BigIntegerResult BigIntegerDivInt    ( BigInteger* result, BigInteger* d, unsigned int m );
BigIntegerResult BigIntegerMod       ( BigInteger* result, BigInteger* d, BigInteger* m );
unsigned int     BigIntegerModInt    ( BigInteger* d, unsigned int m );
BigIntegerResult BigIntegerModMul    ( BigInteger* r, BigInteger* m1, BigInteger* m2, BigInteger* modulus );
BigIntegerResult BigIntegerModExp    ( BigInteger* r, BigInteger* b, BigInteger* e, BigInteger* m );
BigIntegerResult BigIntegerToCstrEx  ( BigInteger* x, cstr * out, int len );
BigIntegerResult BigIntegerInitialize( BigInteger* a );
BigIntegerResult BigIntegerFree      ( BigInteger* b );

/* Miscellaneous functions - formerly in t_pwd.h */

/*
 * "t_random" is a cryptographic random number generator, which is seeded
 *   from various high-entropy sources and uses a one-way hash function
 *   in a feedback configuration.
 * "t_sessionkey" is the interleaved hash used to generate session keys
 *   from a large integer.
 * "t_mgf1" is an implementation of MGF1 using SHA1 to generate session
 *   keys from large integers, and is preferred over the older
 *   interleaved hash, and is used with SRP6.
 * "t_getpass" reads a password from the terminal without echoing.
 */
 void t_random P((void*, unsigned));
 unsigned char * t_sessionkey P((unsigned char *, unsigned char *, unsigned));
 void t_mgf1 P((unsigned char *, unsigned, const unsigned char *, unsigned));
 int t_getpass P((char *, unsigned, const char *));

#ifdef __cplusplus
}
#endif

#endif /* SRP_AUX_H */
