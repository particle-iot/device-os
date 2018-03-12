/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "ed25519-donna-portable-identify.h"

#define mul32x32_64(a,b) (((uint64_t)(a))*(b))

/* A uniform alignment framework for future use
#define DONNA_ALIGN_DECL(pow2, type, var) DONNA_ALIGN_DECL_ ##pow2 (type, var)

 * Then define a platform dependent implementation of DONNA_ALIGN_DECL_xxx
#if defined(COMPILER_MSVC)
    #define DONNA_ALIGN_DECL_xxx(type, var) type var __declspec(align(16))
#elif defined(__ICCARM__)
    #define DONNA_ALIGN_DECL_xxx(type, var) _Pragma ("data_alignment = xxx") \
    type var
#else
    #define DONNA_ALIGN_DECL_xxx(type, var) type __attribute__((aligned(x))) var
#endif
*/

/* platform */
#if defined(COMPILER_MSVC)
    #include <intrin.h>
    #if !defined(_DEBUG)
        #undef mul32x32_64
        #define mul32x32_64(a,b) __emulu(a,b)
    #endif
    #undef inline
    #define inline __forceinline
    #define DONNA_INLINE __forceinline
    #define DONNA_NOINLINE __declspec(noinline)
    #define ALIGN(x) __declspec(align(x))
    #define ROTL32(a,b) _rotl(a,b)
    #define ROTR32(a,b) _rotr(a,b)
#else
#if defined(__ICCARM__)
    /*
     * The currently support MCU such as Cortex M/R does not support vectorization.
     * Therefore, it is safe to use default natural alignment for IAR compiler. To enable
     * manual alignment support, please use above uniform alignment framework.
     */
    #define ALIGN(x)
    #define DONNA_INLINE _Pragma ("inline=forced")
    #define DONNA_NOINLINE _Pragma ("inline=never")
#else
#ifndef __NUTTX__
#include <sys/param.h>
#endif
    #define DONNA_INLINE inline __attribute__((always_inline))
    #define DONNA_NOINLINE __attribute__((noinline))
    #define ALIGN(x) __attribute__((aligned(x)))
#endif
    #define ROTL32(a,b) (((a) << (b)) | ((a) >> (32 - b)))
    #define ROTR32(a,b) (((a) >> (b)) | ((a) << (32 - b)))
#endif

/* uint128_t */
#if defined(CPU_64BITS) && !defined(ED25519_FORCE_32BIT)
    #if defined(COMPILER_CLANG) && (COMPILER_CLANG >= 30100)
        #define HAVE_NATIVE_UINT128
        typedef unsigned __int128 uint128_t;
    #elif defined(COMPILER_MSVC)
        #define HAVE_UINT128
        typedef struct uint128_t {
            uint64_t lo, hi;
        } uint128_t;
        #define mul64x64_128(out,a,b) out.lo = _umul128(a,b,&out.hi);
        #define shr128_pair(out,hi,lo,shift) out = __shiftright128(lo, hi, shift);
        #define shl128_pair(out,hi,lo,shift) out = __shiftleft128(lo, hi, shift);
        #define shr128(out,in,shift) shr128_pair(out, in.hi, in.lo, shift)
        #define shl128(out,in,shift) shl128_pair(out, in.hi, in.lo, shift)
        #define add128(a,b) { uint64_t p = a.lo; a.lo += b.lo; a.hi += b.hi + (a.lo < p); }
        #define add128_64(a,b) { uint64_t p = a.lo; a.lo += b; a.hi += (a.lo < p); }
        #define lo128(a) (a.lo)
        #define hi128(a) (a.hi)
    #elif defined(COMPILER_GCC) && !defined(HAVE_NATIVE_UINT128)
        #if defined(__SIZEOF_INT128__)
            #define HAVE_NATIVE_UINT128
            typedef unsigned __int128 uint128_t;
        #elif (COMPILER_GCC >= 40400)
            #define HAVE_NATIVE_UINT128
            typedef unsigned uint128_t __attribute__((mode(TI)));
        #elif defined(CPU_X86_64)
            #define HAVE_UINT128
            typedef struct uint128_t {
                uint64_t lo, hi;
            } uint128_t;
            #define mul64x64_128(out,a,b) __asm__ ("mulq %3" : "=a" (out.lo), "=d" (out.hi) : "a" (a), "rm" (b));
            #define shr128_pair(out,hi,lo,shift) __asm__ ("shrdq %2,%1,%0" : "+r" (lo) : "r" (hi), "J" (shift)); out = lo;
            #define shl128_pair(out,hi,lo,shift) __asm__ ("shldq %2,%1,%0" : "+r" (hi) : "r" (lo), "J" (shift)); out = hi;
            #define shr128(out,in,shift) shr128_pair(out,in.hi, in.lo, shift)
            #define shl128(out,in,shift) shl128_pair(out,in.hi, in.lo, shift)
            #define add128(a,b) __asm__ ("addq %4,%2; adcq %5,%3" : "=r" (a.hi), "=r" (a.lo) : "1" (a.lo), "0" (a.hi), "rm" (b.lo), "rm" (b.hi) : "cc");
            #define add128_64(a,b) __asm__ ("addq %4,%2; adcq $0,%3" : "=r" (a.hi), "=r" (a.lo) : "1" (a.lo), "0" (a.hi), "rm" (b) : "cc");
            #define lo128(a) (a.lo)
            #define hi128(a) (a.hi)
        #endif
    #endif

    #if defined(HAVE_NATIVE_UINT128)
        #define HAVE_UINT128
        #define mul64x64_128(out,a,b) out = (uint128_t)a * b;
        #define shr128_pair(out,hi,lo,shift) out = (uint64_t)((((uint128_t)hi << 64) | lo) >> (shift));
        #define shl128_pair(out,hi,lo,shift) out = (uint64_t)(((((uint128_t)hi << 64) | lo) << (shift)) >> 64);
        #define shr128(out,in,shift) out = (uint64_t)(in >> (shift));
        #define shl128(out,in,shift) out = (uint64_t)((in << shift) >> 64);
        #define add128(a,b) a += b;
        #define add128_64(a,b) a += (uint64_t)b;
        #define lo128(a) ((uint64_t)a)
        #define hi128(a) ((uint64_t)(a >> 64))
    #endif

    #if !defined(HAVE_UINT128)
        #error Need a uint128_t implementation!
    #endif
#endif

/* endian */
#if !defined(ED25519_OPENSSLRNG)
static inline void U32TO8_LE(unsigned char *p, const uint32_t v) {
    p[0] = (unsigned char)(v      );
    p[1] = (unsigned char)(v >>  8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}
#endif

#if !defined(HAVE_UINT128)
static inline uint32_t U8TO32_LE(const unsigned char *p) {
    return
    (((uint32_t)(p[0])      ) |
     ((uint32_t)(p[1]) <<  8) |
     ((uint32_t)(p[2]) << 16) |
     ((uint32_t)(p[3]) << 24));
}
#else
static inline uint64_t U8TO64_LE(const unsigned char *p) {
    return
    (((uint64_t)(p[0])      ) |
     ((uint64_t)(p[1]) <<  8) |
     ((uint64_t)(p[2]) << 16) |
     ((uint64_t)(p[3]) << 24) |
     ((uint64_t)(p[4]) << 32) |
     ((uint64_t)(p[5]) << 40) |
     ((uint64_t)(p[6]) << 48) |
     ((uint64_t)(p[7]) << 56));
}

static inline void U64TO8_LE(unsigned char *p, const uint64_t v) {
    p[0] = (unsigned char)(v      );
    p[1] = (unsigned char)(v >>  8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
    p[4] = (unsigned char)(v >> 32);
    p[5] = (unsigned char)(v >> 40);
    p[6] = (unsigned char)(v >> 48);
    p[7] = (unsigned char)(v >> 56);
}
#endif

#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
} /*extern "C" */
#endif
