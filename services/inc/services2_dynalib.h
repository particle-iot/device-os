#ifndef SERVICES2_DYNALIB_H
#define SERVICES2_DYNALIB_H

#include "dynalib.h"

DYNALIB_BEGIN(services2)

DYNALIB_FN(0, services2, tracer_set_callbacks_, int(tracer_callbacks_t*, void*))
DYNALIB_FN(1, services2, tracer_save_checkpoint__, int(tracer_checkpoint_t*, uint32_t, void*))

DYNALIB_END(services2)

#endif  /* SERVICES2_DYNALIB_H */
