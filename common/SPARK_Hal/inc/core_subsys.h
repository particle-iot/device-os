/* 
 * File:   core_subsys.h
 * Author: mat
 *
 * Created on 09 September 2014, 02:27
 */

#ifndef CORE_SUBSYS_H
#define	CORE_SUBSYS_H

#ifdef	__cplusplus
extern "C" {
#endif


/**
 * Reads the subsystem version as a string into a given buffer.
 * @return 0 on success.
 */       
int core_read_subsystem_version(char* buf, int bufLen);
  
// cc3000


/**
 * The event name to publish for this subsystem type.
 */
#define SPARK_SUBSYSTEM_EVENT_NAME "cc3000-patch-version"
  



#ifdef	__cplusplus
}
#endif

#endif	/* CORE_SUBSYS_H */

