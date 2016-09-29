/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PREPROCESSOR_H
#define _PREPROCESSOR_H

/*
    PP_STR(x)

    Expands and stringifies argument.

    #define A a
    PP_STR(A) // Expands to "a"
*/
#define PP_STR(x) \
        _PP_STR(x)

#define _PP_STR(x) #x

/*
    PP_CAT(a, b)

    Expands and concatenates arguments.

    #define A a
    #define B b
    PP_CAT(A, B) // Expands to ab
*/
#define PP_CAT(a, b) \
        _PP_CAT(a, b)

#define _PP_CAT(a, b) a##b

/*
    PP_TUPLE(...)

    Encloses arguments in parentheses.

    PP_TUPLE(a, b) // Expands to (a, b)
*/
#define PP_TUPLE(...) (__VA_ARGS__)

/*
    PP_ARGS(x)

    Removes enclosing parentheses.

    PP_ARGS((a, b)) // Expands to a, b
*/
#define PP_ARGS(x) \
        _PP_ARGS x

#define _PP_ARGS(...) __VA_ARGS__

/*
    PP_COUNT(...)

    Expands to a number of arguments. This macro supports up to 30 arguments.

    PP_COUNT(a, b) // Expands to 2
*/
#define PP_COUNT(...) \
        _PP_COUNT(_, ##__VA_ARGS__, _PP_COUNT_N)

#define _PP_COUNT(...) \
        _PP_COUNT_(__VA_ARGS__)

#define _PP_COUNT_(_, \
        _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
        _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
        n, ...) n

#define _PP_COUNT_N \
        30, 29, 28, 27, 26, 25, 24, 23, 22, 21, \
        20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
        10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

/*
    PP_FOR_EACH(macro, data, ...)

    Expands macro for each argument. This macro supports up to 30 arguments.

    #define CALL(func, arg) func(arg);
    PP_FOR_EACH(CALL, foo, 1, 2, 3) // Expands to foo(1); foo(2); foo(3);
*/
#define PP_FOR_EACH(macro, data, ...) \
        _PP_FOR_EACH(PP_CAT(_PP_FOR_EACH_, PP_COUNT(__VA_ARGS__)), macro, data, ##__VA_ARGS__)

#define _PP_FOR_EACH(...) \
        _PP_FOR_EACH_(__VA_ARGS__)

#define _PP_FOR_EACH_(for_each, macro, data, ...) \
        for_each(macro, data, ##__VA_ARGS__)

#define _PP_FOR_EACH_0(m, d, ...)
#define _PP_FOR_EACH_1(m, d, _1) \
        _PP_FOR_EACH_0(m, d) m(d, _1)
#define _PP_FOR_EACH_2(m, d, _1, _2) \
        _PP_FOR_EACH_1(m, d, _1) m(d, _2)
#define _PP_FOR_EACH_3(m, d, _1, _2, _3) \
        _PP_FOR_EACH_2(m, d, _1, _2) m(d, _3)
#define _PP_FOR_EACH_4(m, d, _1, _2, _3, _4) \
        _PP_FOR_EACH_3(m, d, _1, _2, _3) m(d, _4)
#define _PP_FOR_EACH_5(m, d, _1, _2, _3, _4, _5) \
        _PP_FOR_EACH_4(m, d, _1, _2, _3, _4) m(d, _5)
#define _PP_FOR_EACH_6(m, d, _1, _2, _3, _4, _5, _6) \
        _PP_FOR_EACH_5(m, d, _1, _2, _3, _4, _5) m(d, _6)
#define _PP_FOR_EACH_7(m, d, _1, _2, _3, _4, _5, _6, _7) \
        _PP_FOR_EACH_6(m, d, _1, _2, _3, _4, _5, _6) m(d, _7)
#define _PP_FOR_EACH_8(m, d, _1, _2, _3, _4, _5, _6, _7, _8) \
        _PP_FOR_EACH_7(m, d, _1, _2, _3, _4, _5, _6, _7) m(d, _8)
#define _PP_FOR_EACH_9(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9) \
        _PP_FOR_EACH_8(m, d, _1, _2, _3, _4, _5, _6, _7, _8) m(d, _9)
#define _PP_FOR_EACH_10(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) \
        _PP_FOR_EACH_9(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9) m(d, _10)
#define _PP_FOR_EACH_11(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) \
        _PP_FOR_EACH_10(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) m(d, _11)
#define _PP_FOR_EACH_12(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) \
        _PP_FOR_EACH_11(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) m(d, _12)
#define _PP_FOR_EACH_13(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) \
        _PP_FOR_EACH_12(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) m(d, _13)
#define _PP_FOR_EACH_14(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14) \
        _PP_FOR_EACH_13(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) m(d, _14)
#define _PP_FOR_EACH_15(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) \
        _PP_FOR_EACH_14(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14) m(d, _15)
#define _PP_FOR_EACH_16(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16) \
        _PP_FOR_EACH_15(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) m(d, _16)
#define _PP_FOR_EACH_17(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17) \
        _PP_FOR_EACH_16(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16) m(d, _17)
#define _PP_FOR_EACH_18(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18) \
        _PP_FOR_EACH_17(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17) m(d, _18)
#define _PP_FOR_EACH_19(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19) \
        _PP_FOR_EACH_18(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18) m(d, _19)
#define _PP_FOR_EACH_20(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20) \
        _PP_FOR_EACH_19(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19) m(d, _20)
#define _PP_FOR_EACH_21(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21) \
        _PP_FOR_EACH_20(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20) m(d, _21)
#define _PP_FOR_EACH_22(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22) \
        _PP_FOR_EACH_21(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21) m(d, _22)
#define _PP_FOR_EACH_23(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23) \
        _PP_FOR_EACH_22(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22) m(d, _23)
#define _PP_FOR_EACH_24(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24) \
        _PP_FOR_EACH_23(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23) m(d, _24)
#define _PP_FOR_EACH_25(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25) \
        _PP_FOR_EACH_24(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24) m(d, _25)
#define _PP_FOR_EACH_26(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26) \
        _PP_FOR_EACH_25(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25) m(d, _26)
#define _PP_FOR_EACH_27(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27) \
        _PP_FOR_EACH_26(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26) m(d, _27)
#define _PP_FOR_EACH_28(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28) \
        _PP_FOR_EACH_27(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27) m(d, _28)
#define _PP_FOR_EACH_29(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29) \
        _PP_FOR_EACH_28(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28) m(d, _29)
#define _PP_FOR_EACH_30(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30) \
        _PP_FOR_EACH_29(m, d, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29) m(d, _30)

#endif // _PREPROCESSOR_H
