#ifndef SERVICES2_DYNALIB_H
#define SERVICES2_DYNALIB_H

#include "dynalib.h"

DYNALIB_BEGIN(services2)

DYNALIB_FN(0, services2, diagnostic_set_callbacks_, int(diagnostic_callbacks_t*, void*))
DYNALIB_FN(1, services2, diagnostic_save_checkpoint__, int(diagnostic_checkpoint_t*, uint32_t, void*))

DYNALIB_END(services2)

#endif  /* SERVICES2_DYNALIB_H */
