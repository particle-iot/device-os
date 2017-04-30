#ifndef MODULE_SYSTEM_PART2_INIT_DYNALIB_H
#define	MODULE_SYSTEM_PART2_INIT_DYNALIB_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "module_system_part2_init.h"
#endif

DYNALIB_BEGIN(system_module_part2)

DYNALIB_FN(0, system_module_part2, module_system_part2_pre_init, void*(void))

DYNALIB_END(system_module_part2)

#endif /* MODULE_SYSTEM_PART2_INIT_DYNALIB_H */
