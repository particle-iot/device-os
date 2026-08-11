#pragma once

#include "hal_platform.h"

#if HAL_PLATFORM_BOOT_LOG

#include "logging.h"

namespace particle::system {

// Called by the system
int initBootLog();
int flushBootLog();
void closeBootLog();

// Called by the logging service
void bootLogMessage(const char* msg, int level, const char* category, const LogAttributes* attrs);
void writeBootLog(const char* data, size_t size, int level, const char* category);
bool isBootLogEnabled(int level, const char* category);

} // namespace particle::system

#endif // HAL_PLATFORM_BOOT_LOG
