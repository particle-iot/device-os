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
#define PARTICLE_STATIC_ASSERT(name, condition) static_assert(condition, #name)
#elif defined(__STDC_VERSION__) &&  (defined(__arm__) || __STDC_VERSION__ >= 201112L)
// _Static_assert is supported starting with C11, however we can bypass the check
// because GCC supports it no matter what starting with 4.6
#define PARTICLE_STATIC_ASSERT(name, condition) _Static_assert(condition, #name)
#else
// Fallback
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

