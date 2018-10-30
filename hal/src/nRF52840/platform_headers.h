#pragma once

/**
 * This header file can be used to control access to low-level platform
 * features in user code. Normally user code should access the platform via
 * the HAL.
 */

#define retained __attribute__((section(".retained_user")))
#define retained_system __attribute__((section(".retained_system")))
