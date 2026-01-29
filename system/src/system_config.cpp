/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "system_config.h"

#if HAL_PLATFORM_ENV_VARS

#include <limits>

#include "env_vars.h"

using namespace particle::system;

int system_get_env(const char* name, char* buf, size_t bufSize, void* reserved) {
	return EnvVars::instance().get(name, buf, bufSize);
}

int system_get_env_int(const char* name, int* val, void* reserved) {
	return EnvVars::instance().get(name, *val);
}

int system_get_env_bool(const char* name, bool* val, void* reserved) {
	return EnvVars::instance().get(name, *val);
}

int system_list_env_vars(const char* names[], size_t count, void* reserved) {
	size_t i = 0;
	int r = EnvVars::instance().forEach([&](const char* name) -> int {
		if (i >= count) {
			return std::numeric_limits<int>::min(); // Break the loop
		}
		names[i++] = name;
		return 0;
	});
	if (r < 0 && r != std::numeric_limits<int>::min()) {
		return r;
	}
	return EnvVars::instance().count();
}

int system_clear_env_vars(void* reserved) {
	return EnvVars::instance().clear();
}

#endif // HAL_PLATFORM_ENV_VARS
