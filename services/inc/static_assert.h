/*
 * File:   static_assert.h
 * Author: mat
 *
 * Created on 16 December 2014, 04:06
 */

#ifndef STATIC_ASSERT_H
#define	STATIC_ASSERT_H

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
// C++
// static_assert is supported since C++11
#define PARTICLE_STATIC_ASSERT(name, condition) static_assert(condition, #name)
#elif defined(__STDC_VERSION__) && ((__STDC_VERSION__ >= 201112L) || \
        (defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)))
// C
// _Static_assert is supported since C11, and since GCC 4.6 no matter the standard
#define PARTICLE_STATIC_ASSERT(name, condition) _Static_assert(condition, #name)
#else
// Fallback, this doesn't work in all cases e.g. when static_asserting in a function: generates an unused warning
// causing an error with Werror
#define PARTICLE_STATIC_ASSERT(name, condition) typedef char assert_##name[(condition)?0:-1]
#endif

// Compatibility macro
#define STATIC_ASSERT_EXPR(name, condition) PARTICLE_STATIC_ASSERT(name, condition)

#if !defined(STATIC_ASSERT) && !defined(NO_STATIC_ASSERT)
#define STATIC_ASSERT(name, condition) PARTICLE_STATIC_ASSERT(name, condition)
#endif /* !defined(STATIC_ASSERT) && !defined(NO_STATIC_ASSERT) */

#ifdef	__cplusplus
}
#endif

#endif	/* STATIC_ASSERT_H */

