#ifndef SERVICES2_DYNALIB_H
#define SERVICES2_DYNALIB_H

#include "dynalib.h"

DYNALIB_BEGIN(services2)

DYNALIB_FN(0, services2, system_monitor_set_callbacks_, int(system_monitor_callbacks_t*, void*))

DYNALIB_END(services2)

#endif  /* SERVICES2_DYNALIB_H */
