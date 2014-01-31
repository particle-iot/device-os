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


#endif /* SPARK_MACROS_H_ */
