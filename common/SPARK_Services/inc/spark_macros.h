/*
 * spark_macros.h
 *
 *  Created on: Jan 31, 2014
 *      Author: david_s5
 */

#ifndef SPARK_MACROS_H_
#define SPARK_MACROS_H_

#if !defined(arraySize)
#   define arraySize(a)            (sizeof((a))/sizeof((a[0])))
#endif

#define INVALID_CASE(c) PANIC(InvalidCase,"Invalid Case %d",(c))
#define UNUSED(var) (void)(var)

#define _CAT(a, b) a ## b
#define CAT(a, b) _CAT(a, b)

#define CCASSERT(predicate) _x_CCASSERT_LINE(predicate, __LINE__)
#define _x_CCASSERT_LINE(predicate, line) typedef char CAT(constraint_violated_on_line_,line)[2*((predicate)!=0)-1];

// Seconds to Us
#define S2u(s) ((s)*1000000)
// Mili Seconds to Us
#define MS2u(m) ((m)*1000)

// Seconds to Ms
#define S2M(s) ((s)*1000)

#endif /* SPARK_MACROS_H_ */
