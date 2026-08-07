#pragma once

#include "hal_platform.h"

#if HAL_PLATFORM_BOOT_LOG

#include "logging.h"
#include "system_config.h"

#include <cstdarg>

namespace particle::system {

// Called by the system
int initBootLog();
void stopWritingBootLog();

// Called by the logging service
void bootLogMessage(const char* msg, int level, const char* category, const LogAttributes* attrs);
void writeBootLog(const char* data, size_t size, int level, const char* category);
bool isBootLogEnabled(int level, const char* category);

} // namespace particle::system

#endif // HAL_PLATFORM_BOOT_LOG
