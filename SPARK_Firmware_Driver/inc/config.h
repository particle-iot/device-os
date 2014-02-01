/*
 * config.h
 *
 *  Created on: Jan 31, 2014
 *      Author: david_s5
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#if !defined(RELEASE_BUILD) && !defined(DEBUG_BUILD)
#warning  "Defaulting to Release Build"
#define RELEASE_BUILD
#undef  DEBUG_BUILD
#endif

// define to include __FILE__ information within the debug output
#define INCLUDE_FILE_INFO_IN_DEBUG
#define MAX_DEBUG_MESSAGE_LENGTH 120


#endif /* CONFIG_H_ */
