#pragma once

/**
  ******************************************************************************
  * @file    dynalib.h
  * @authors  Matthew McGowan
  * @brief   User Application File Header
  ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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

#define DYNALIB_TABLE_EXTERN(tablename) \
    extern const void* dynalib_##tablename[];

#define DYNALIB_TABLE_NAME(tablename) \
    dynalib_##tablename

#ifdef DYNALIB_EXPORT


    /**
     * Begin the jump table definition
     */
    #define DYNALIB_BEGIN(tablename) \
        const void* dynalib_##tablename[] = {

    #define DYNALIB_FN(tablename,name,index) \
        &name,

    #define DYNALIB_END(name)   \
        0 };


#elif defined(DYNALIB_IMPORT)

    #ifdef __arm__ 

        #define DYNALIB_BEGIN(tablename)    \
            extern const void* dynalib_location_##tablename;

        #define DYNALIB_FN(tablename, name, index) \
            const char check_index_##tablename_##index[0]={}; /* this will fail if index is already defined */ \
            const char check_name_##tablename_##name[0]={}; /* this will fail if the name is already defined */ \
            void name() __attribute__((naked)); \
            void name() { \
                asm volatile ( \
                    ".equ offset, (" #index " * 4)\n" \
                    ".extern dynalib_location_" #tablename "\n" \
                    "push {r3, lr}\n"           /* save register we will change plus storage for sp value */ \
                                                /* pushes highest register first, so lowest register is at lowest memory address */ \
                                                /* SP points to the last pushed item, which is r3. sp+4 is then the pushed lr value */ \
                    "ldr r3, =dynalib_location_" #tablename "\n" \
                    "ldr r3, [r3]\n"                    /* the address of the jump table */ \
                    "ldr r3, [r3, #offset ]\n"    /* the address at index __COUNTER__ */ \
                    "str r3, [sp, #4]\n"                /* patch the link address on the stack */ \
                    "pop {r3, pc}\n"                    /* restore register and jump to function */ \
                ); \
            };

        #define DYNALIB_END(name)
    #else
        #error Unknown architecture
    #endif // __arm__        
#endif


