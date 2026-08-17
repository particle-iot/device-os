#pragma once

#include "hal_platform.h"

#if HAL_PLATFORM_LOG_FILE

#include "logging.h"

namespace particle::system {

// Called by the system
int initLogFile();
int flushLogFile();
int closeLogFile();

// Called by the logging service
void logMessageToFile(const char* msg, int level, const char* category, const LogAttributes* attrs);
void writeToLogFile(const char* data, size_t size, int level, const char* category);
bool isLogFileEnabledForLevel(int level, const char* category);
bool isLogFileEnabled();

} // namespace particle::system

#endif // HAL_PLATFORM_LOG_FILE
