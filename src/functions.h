/* 
 * File:   functions.h
 * Author: mat
 *
 * Created on 04 September 2014, 00:10
 */

#ifndef FUNCTIONS_H
#define	FUNCTIONS_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Handle the cryptographically secure random seed from the cloud.
 * @param seed  A random value. This is typically used to seed a pseudo-random generator. 
 */
extern __attribute__((weak)) void random_seed_from_cloud(unsigned int seed);



#ifdef	__cplusplus
}
#endif

#endif	/* FUNCTIONS_H */

