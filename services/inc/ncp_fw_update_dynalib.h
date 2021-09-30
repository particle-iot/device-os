/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#ifdef __cplusplus
struct NcpFwUpdateConfig {
    int start_version;
    int end_version;
    char filename[255 + 1];
    char md5sum[32 + 1];
};
#else
typedef struct NcpFwUpdateConfig NcpFwUpdateConfig;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)
int ncp_fw_udpate_config(const NcpFwUpdateConfig* data, void* reserved);
#endif // !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)

#ifdef __cplusplus
}
#endif
