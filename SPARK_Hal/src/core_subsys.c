
#include "core_subsys.h"


int core_read_subsystem_version(char* patchstr, int bufLen) {
  unsigned char patchver[2];
  int result = nvmem_read_sp_version(patchver);    
  snprintf(patchstr, bufLen, "%d.%d", patchver[0], patchver[1]);
  return result;
}

