
#include "core_subsys_hal.h"
#include "nvmem.h"
#include "stdio.h"




inline void core_read_subsystem_version_impl(char* patchstr, int bufLen, unsigned char patchver[2]) {      
    snprintf(patchstr, bufLen, "%d.%d", patchver[0], patchver[1]);    
}

int core_read_subsystem_version(char* patchstr, int bufLen) {
    unsigned char patchver[2];
    int result = nvmem_read_sp_version(patchver);
    core_read_subsystem_version_impl(patchstr, bufLen, patchver);
    return result;
}


