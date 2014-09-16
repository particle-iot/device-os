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


int core_read_subsystem_version(char* buf, int bufLen);
  
// cc3000
#define SPARK_SUBSYSTEM_EVENT_NAME "spark/cc3000-patch-version"
  



#ifdef	__cplusplus
}
#endif

#endif	/* CORE_SUBSYS_H */

