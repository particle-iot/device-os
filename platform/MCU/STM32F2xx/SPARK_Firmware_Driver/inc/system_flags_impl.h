#ifndef SYSTEM_FLAGS_IMPL_H
#define SYSTEM_FLAGS_IMPL_H

#include "platform_system_flags.h"

#ifdef __cplusplus
extern "C" {
#endif

int Load_SystemFlags_Impl(platform_system_flags_t* flags);
int Save_SystemFlags_Impl(const platform_system_flags_t* flags);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SYSTEM_FLAGS_IMPL_H
