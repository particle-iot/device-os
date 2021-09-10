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

#if 0
namespace particle {

namespace {

struct NcpFwUpdateConfig {
    const uint32_t start_version;
    const uint32_t end_version;
    const char* filename;
    const char* md5sum;
};

/*
a2773f2abb80df2886dd29b07f089504  SARA-R510S-00B-00_FW02.05_A00.01_IP_SARA-R510S-00B-01_FW02.06_A00.01_IP.upd
48b2d022041ea85899a15351c06a18d2  SARA-R510S-00B-01_FW02.06_A00.01_IP.dof
252ea04a324e9aab8a69678cfe097465  SARA-R510S-00B-01_FW02.06_A00.01_IP.upd
ccfdc48c0a45198d6e168b30d0740959  SARA-R510S-00B-01_FW02.06_A00.01_IP_SARA-R510S-00B-01_FW99.01_A00.01.upd
5fd6c0d3d731c097605895b86f28c2cf  SARA-R510S-00B-01_FW99.01_A00.01_SARA-R510S-00B-01_FW02.06_A00.01_IP.upd
09c1a98d03c761bcbea50355f9b2a50f  SARA-R510S-01B-00-ES-0314A0001-005K00_SARA-R510S-01B-00-XX-0314ENG0099A0001-005K00.upd
09c1a98d03c761bcbea50355f9b2a50f  SARA-R510S-01B-00-ES-0314A0001_SARA-R510S-01B-00-XX-0314ENG0099A0001.upd
136caf2883457093c9e41fda3c6a44e3  SARA-R510S-01B-00-XX-0314ENG0099A0001-005K00_SARA-R510S-01B-00-ES-0314A0001-005K00.upd
136caf2883457093c9e41fda3c6a44e3  SARA-R510S-01B-00-XX-0314ENG0099A0001_SARA-R510S-01B-00-ES-0314A0001.upd
*/

const NcpFwUpdateConfig NCP_FW_UPDATE_CONFIG[] = {
    { 3140001, 103140001, "SARA-R510S-01B-00-ES-0314A0001_SARA-R510S-01B-00-XX-0314ENG0099A0001.upd", "09c1a98d03c761bcbea50355f9b2a50f" },
    { 103140001, 3140001, "SARA-R510S-01B-00-XX-0314ENG0099A0001_SARA-R510S-01B-00-ES-0314A0001.upd", "136caf2883457093c9e41fda3c6a44e3" },
    { 2060001, 99010001, "SARA-R510S-00B-01_FW02.06_A00.01_IP_SARA-R510S-00B-01_FW99.01_A00.01.upd", "ccfdc48c0a45198d6e168b30d0740959" },
    { 99010001, 2060001, "SARA-R510S-00B-01_FW99.01_A00.01_SARA-R510S-00B-01_FW02.06_A00.01_IP.upd", "5fd6c0d3d731c097605895b86f28c2cf" },
};

const size_t NCP_FW_UPDATE_CONFIG_SIZE = sizeof(NCP_FW_UPDATE_CONFIG) / sizeof(NCP_FW_UPDATE_CONFIG[0]);

} // unnamed

int firmwareUpdateForVersion(const uint32_t version) {
    for (size_t i = 0; i < NCP_FW_UPDATE_CONFIG_SIZE; ++i) {
        if (version == NCP_FW_UPDATE_CONFIG[i].start_version) {
            return i;
        }
    }
    return -1;
}

} // particle

#endif