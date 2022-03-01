#pragma once

/**
  ******************************************************************************
  * @file    dynalib.h
  * @authors Matthew McGowan
  * @brief   Import and Export of dynamically linked functions
  ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */

//
// The dynalib macros produce either a a dynamic lib export table (jump table)
// or a set of import stubs.
// DYNALIB_EXPORT is defined to produce a jump table.
// DYNALIB_IMPORT is defined to produce a set of function stubs
//

// DYNALIB_EXTERN_C to mark symbols with C linkage
#ifdef __cplusplus
#define DYNALIB_EXTERN_C extern "C"
#define EXTERN_C DYNALIB_EXTERN_C
#else
#define DYNALIB_EXTERN_C
#define EXTERN_C extern
#endif


/*
 * Create an export table that delegates to the export table in another module.
 */



#define DYNALIB_TABLE_EXTERN(tablename) \
    EXTERN_C const void* const dynalib_##tablename[];

#define DYNALIB_TABLE_NAME(tablename) \
    dynalib_##tablename


#if defined(__cplusplus) && __cplusplus >= 201103 // C++11

#include <type_traits>

// XXX: casting is not allowed in constant expressions (at least since C++14).
// We were previously casting to const void* in dynalib_checked_cast(), which resulted in
// dynalib tables being placed into .bss section with certain GCC versions.
// Miraculously, everything works correctly if the cast is outside of the constexpr function.
// TODO: consider adding std::is_constant_evaluated() check in the future (available in C++20)

template<typename T1, typename T2>
constexpr T2* dynalib_checked_cast(T2 *p) {
    static_assert(std::is_same<T1, T2>::value, "Signature of the dynamically exported function has changed");
    return p;
}

#define DYNALIB_FN_EXPORT(index, tablename, name, type) \
    reinterpret_cast<const void*>(dynalib_checked_cast<type>(name)),

#define DYNALIB_STATIC_ASSERT(cond, msg) \
    static_assert(cond, msg)

#elif (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901) && !(defined(__STRICT_ANSI__) && __STRICT_ANSI__) // C99 with GNU extensions

#define DYNALIB_FN_EXPORT(index, tablename, name, type) \
    __builtin_choose_expr(__builtin_types_compatible_p(type, __typeof__(name)), (const void*)&name, sizeof(struct { \
        _Static_assert(__builtin_types_compatible_p(type, __typeof__(name)), \
            "Signature of the dynamically exported function has changed");})),

#define DYNALIB_STATIC_ASSERT(cond, msg) \
    _Static_assert(cond, msg)

#else

#warning "Compile-time check of the dynamically exported functions is not supported under current compiler settings"

#define DYNALIB_FN_EXPORT(index, tablename, name, type) \
    (const void*)&name,

#define DYNALIB_STATIC_ASSERT(cond, msg)

#endif


#ifdef DYNALIB_EXPORT

    /**
     * Begin the jump table definition
     */
    #define DYNALIB_BEGIN(tablename) \
        DYNALIB_EXTERN_C const void* const dynalib_##tablename[] = {

    #define DYNALIB_FN(index, tablename, name, type) \
        DYNALIB_FN_EXPORT(index, tablename, name, type)

    #define DYNALIB_FN_PLACEHOLDER(index, tablename) \
        0,

    #define DYNALIB_END(name)   \
        0 };


#elif defined(DYNALIB_IMPORT)

    #ifdef __arm__

        #define DYNALIB_BEGIN(tablename)    \
            EXTERN_C const void* dynalib_location_##tablename;

        #define __S(x) #x
        #define __SX(x) __S(x)

        #define DYNALIB_FN_IMPORT(index, tablename, name, counter) \
            DYNALIB_STATIC_ASSERT(index == counter, "Index of the dynamically exported function has changed"); \
            const char check_name_##tablename_##name[0]={}; /* this will fail if the name is already defined */ \
            void name() __attribute__((naked)); \
            void name() { \
                asm volatile ( \
                    ".equ offset, ( " __SX(counter) " * 4)\n" \
                    ".extern dynalib_location_" #tablename "\n" \
                    "push {r3, lr}\n"           /* save register we will change plus storage for sp value */ \
                                                /* pushes highest register first, so lowest register is at lowest memory address */ \
                                                /* SP points to the last pushed item, which is r3. sp+4 is then the pushed lr value */ \
                    "ldr r3, =dynalib_location_" #tablename "\n" \
                    "ldr r3, [r3]\n"                    /* the address of the jump table */ \
                    "ldr r3, [r3, #offset]\n"    /* the address at index __COUNTER__ */ \
                    "str r3, [sp, #4]\n"                /* patch the link address on the stack */ \
                    "pop {r3, pc}\n"                    /* restore register and jump to function */ \
                ); \
            };

        #define DYNALIB_FN(index, tablename, name, type) \
            DYNALIB_FN_IMPORT(index, tablename, name, __COUNTER__)

        #define DYNALIB_FN_PLACEHOLDER(index, tablename) \
            DYNALIB_STATIC_ASSERT(index == __COUNTER__, "Index of the dynamically exported function has changed"); // Ensures that __COUNTER__ is incremented

        #define DYNALIB_END(name)
    #else
        #error Unknown architecture
    #endif // __arm__
#endif


