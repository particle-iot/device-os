#ifndef NRFX_TYPES_H__
#define NRFX_TYPES_H__

typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;
#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))

#endif // NRFX_TYPES_H__
