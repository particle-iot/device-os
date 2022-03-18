/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "system_string_interpolate.h"
#include "system_version.h"
#include "spark_macros.h"
#include "spark_wiring_string.h"
#include "ota_flash_hal.h"
#include "system_cloud_internal.h"
#include <string.h>

#undef WARN
#undef INFO

#include "catch2/catch.hpp"
#include "hippomocks.h"

using Catch::Matchers::Equals;

// $id.udp.particle.io
const unsigned char backup_udp_public_server_address[] = {
  0x01, 0x13, 0x24, 0x69, 0x64, 0x2e, 0x75, 0x64, 0x70, 0x2e, 0x70, 0x61,
  0x72, 0x74, 0x69, 0x63, 0x6c, 0x65, 0x2e, 0x69, 0x6f, 0x00
};
const size_t backup_udp_public_server_address_size = sizeof(backup_udp_public_server_address);

// $id.udp-mesh.particle.io
const unsigned char backup_udp_mesh_public_server_address[] = {
  0x01, 0x18, 0x24, 0x69, 0x64, 0x2e, 0x75, 0x64, 0x70, 0x2d, 0x6d, 0x65,
  0x73, 0x68, 0x2e, 0x70, 0x61, 0x72, 0x74, 0x69, 0x63, 0x6c, 0x65, 0x2e,
  0x69, 0x6f, 0x00
};
const size_t backup_udp_mesh_public_server_address_size = sizeof(backup_udp_mesh_public_server_address);

// devices.spark.io
const unsigned char backup_tcp_public_server_address[] = {
  0x01, 0x0f, 0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x2e, 0x73, 0x70, 0x61,
  0x72, 0x6b, 0x2e, 0x69, 0x6f, 0x00
};
const size_t backup_tcp_public_server_address_size = sizeof(backup_tcp_public_server_address);

namespace {

#define MOCK_SYSTEM_VERSION_STRING ("vX.Y.Z-unused")
uint32_t mock_system_version = 0;

uint8_t get_major_version(SystemVersionInfo* sys_ver) {
    if (sys_ver) {
        int sys_ver_size = system_version_info(sys_ver, nullptr);
        return BYTE_N(sys_ver->versionNumber, 3);
    }
    return -1;
}

int get_major_version_size(int ver) {
    if (ver > 99) {
        return 3;
    } else if (ver > 9) {
        return 2;
    } else {
        return 1;
    }
}

// NOTE: For this to link, system_interpolate_cloud_server_hostname() was stubbed in unit/stubs/system_cloud.cpp,
// and spark_deviceID() was also stubbed in unit/stubs/system_cloud_internal.cpp.
// system_interpolate_cloud_server_hostname() implementation was copied from system_cloud.cpp
// Just leaving this breadcrumb trail for those who look at this Mock example ;-) -Brett
class Mocks {
public:
    Mocks() {
        mocks_.OnCallFunc(system_version_info).Do([](SystemVersionInfo* info, void*)->int
        {
            if (info)
            {
                if (info->size>=28)
                {
                    info->versionNumber = mock_system_version;
                    strncpy(info->versionString, PP_STR(MOCK_SYSTEM_VERSION_STRING), sizeof(info->versionString));
                }
            }
            return sizeof(SystemVersionInfo);
        });

        // We could make this dynamic like above in system_version_info, but this is fine.
        mocks_.OnCallFunc(spark_deviceID).Do([](void)->String
        {
            return "123412341234123412341234";
        });

        // Implementation matches that found in system_cloud.cpp
        mocks_.OnCallFunc(system_interpolate_cloud_server_hostname).Do([](const char* var, size_t var_len, char* buf, size_t buf_len)->size_t
        {
            if (var_len==2 && memcmp("id", var, 2)==0)
            {
                String deviceID = spark_deviceID();
                size_t id_len = deviceID.length();

                SystemVersionInfo sys_ver = {};
                sys_ver.size = sizeof(SystemVersionInfo);

                system_version_info(&sys_ver, nullptr);
                uint8_t mv = BYTE_N(sys_ver.versionNumber, 3);
                String majorVer = String::format(".v%d", mv);
                size_t mv_len = majorVer.length();

                if (buf_len > (id_len + mv_len)) {
                    memcpy(buf, deviceID.c_str(), id_len);
                    memcpy(buf + id_len, majorVer.c_str(), mv_len);
                    return id_len + mv_len;
                }
            }
            return 0;
        });
    }

private:
    MockRepository mocks_;
};

} // namespace


SCENARIO("attempting to interpolate into a smaller buffer does not interpolate")
{
    GIVEN("an interpolation function")
    {
        // this isn't stricly correct - if the buffer size "cuts" a variable substution
        // in half, then the entire variable substitution is discarded and characters appended
        // after the variable.
        Mocks mocks;
        char buf[5];
        char expected_buf[] = "abc.";
        size_t written;
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        WHEN("a string is interpolated into a too small buffer")
        {
            written = system_string_interpolate("abc$id.123", buf, sizeof(buf), fn);
            THEN("the result is incomplete, but the buffer is not overflowed")
            {
                REQUIRE_THAT( buf, Equals(expected_buf));
                REQUIRE(written == strlen(expected_buf));
            }
        }
    }
}

SCENARIO("can interpolate a simple ID into a larger buffer")
{
    GIVEN("an interpolation function")
    {
        Mocks mocks;
        mock_system_version = SYSTEM_VERSION_DEFAULT(4, 5, 6);
        SystemVersionInfo sys_ver = {};
        sys_ver.size = sizeof(SystemVersionInfo);
        char buf[40];
        char expected_buf[] = "abc123412341234123412341234.v4.123";
        size_t written;
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        WHEN("a string is interpolated into a larger buffer")
        {
            written = system_string_interpolate("abc$id.123", buf, sizeof(buf), fn);
            THEN("the variable is interpolated and non-interpolated parts are correctly copied")
            {
                REQUIRE_THAT( buf, Equals(expected_buf));
                REQUIRE(written == strlen(expected_buf));
            }
        }
    }
}

SCENARIO("can interpolate a simple ID into a buffer that is exactly the size required")
{
    GIVEN("an interpolation function")
    {
        Mocks mocks;
        mock_system_version = SYSTEM_VERSION_DEFAULT(4, 5, 6);
        SystemVersionInfo sys_ver = {};
        sys_ver.size = sizeof(SystemVersionInfo);
        char buf[35];
        char expected_buf[] = "abc123412341234123412341234.v4.123";
        size_t written;
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        WHEN("a string is interpolated into a larger buffer")
        {
            written = system_string_interpolate("abc$id.123", buf, sizeof(buf), fn);
            THEN("the variable is interpolated and non-interpolated parts are correctly copied")
            {
                REQUIRE_THAT( buf, Equals(expected_buf));
                REQUIRE(written == strlen(expected_buf));
            }
        }
    }
}

TEST_CASE("testing system_interpolate_cloud_server_hostname") {

    Mocks mocks;
    ServerAddress server_addr = {};

    SECTION("[UDP] Server public address is updated with major system version v1") {
        mock_system_version = SYSTEM_VERSION_DEFAULT(1, 2, 3);
        SystemVersionInfo sys_ver = {};
        sys_ver.size = sizeof(SystemVersionInfo);

        int major_version = get_major_version(&sys_ver);
        int major_version_size = get_major_version_size(major_version) + 2;

        char expected_str1[] = "123412341234123412341234.v";
        char expected_str2[] = ".udp.particle.io";
        char expected_buf[sizeof(server_addr.domain)] = {};
        sprintf(expected_buf, "%s%d%s", expected_str1, major_version, expected_str2);
        memcpy(&server_addr, backup_udp_public_server_address, backup_udp_public_server_address_size);

        char tmphost[sizeof(server_addr.domain) + 32] = {};
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        int written = system_string_interpolate(server_addr.domain, tmphost, sizeof(tmphost), fn);

        REQUIRE_THAT(tmphost, Equals(expected_buf));

        // backup_udp_public_server_address_size is already +3 due to leading .type, .size
        // and trailing null character, but we also have to remove 3 for "$id"
        REQUIRE(written == 24 + (backup_udp_public_server_address_size - 3 - 3) + major_version_size);
    }

    SECTION("[UDP-MESH] Server public address is updated with major system version v2") {

        mock_system_version = SYSTEM_VERSION_DEFAULT(2, 3, 4);
        SystemVersionInfo sys_ver = {};
        sys_ver.size = sizeof(SystemVersionInfo);

        int major_version = get_major_version(&sys_ver);
        int major_version_size = get_major_version_size(major_version) + 2;

        char expected_str1[] = "123412341234123412341234.v";
        char expected_str2[] = ".udp-mesh.particle.io";
        char expected_buf[sizeof(server_addr.domain)] = {};
        sprintf(expected_buf, "%s%d%s", expected_str1, major_version, expected_str2);
        memcpy(&server_addr, backup_udp_mesh_public_server_address, backup_udp_mesh_public_server_address_size);

        char tmphost[sizeof(server_addr.domain) + 32] = {};
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        int written = system_string_interpolate(server_addr.domain, tmphost, sizeof(tmphost), fn);

        REQUIRE_THAT(tmphost, Equals(expected_buf));

        // backup_udp_public_server_address_size is already +3 due to leading .type, .size
        // and trailing null character, but we also have to remove 3 for "$id"
        REQUIRE(written == 24 + (backup_udp_mesh_public_server_address_size - 3 - 3) + major_version_size);
    }

    SECTION("[UDP-MESH] Server public address is updated with major system version v255") {

        mock_system_version = SYSTEM_VERSION_DEFAULT(255, 3, 4);
        SystemVersionInfo sys_ver = {};
        sys_ver.size = sizeof(SystemVersionInfo);

        int major_version = get_major_version(&sys_ver);
        int major_version_size = get_major_version_size(major_version) + 2;

        char expected_str1[] = "123412341234123412341234.v";
        char expected_str2[] = ".udp-mesh.particle.io";
        char expected_buf[sizeof(server_addr.domain)] = {};
        sprintf(expected_buf, "%s%d%s", expected_str1, major_version, expected_str2);
        memcpy(&server_addr, backup_udp_mesh_public_server_address, backup_udp_mesh_public_server_address_size);

        char tmphost[sizeof(server_addr.domain) + 32] = {};
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        int written = system_string_interpolate(server_addr.domain, tmphost, sizeof(tmphost), fn);

        REQUIRE_THAT( tmphost, Equals(expected_buf));

        // backup_udp_public_server_address_size is already +3 due to leading .type, .size
        // and trailing null character, but we also have to remove 3 for "$id"
        REQUIRE(written == 24 + (backup_udp_mesh_public_server_address_size - 3 - 3) + major_version_size);
    }

    SECTION("[TCP] Server public address is NOT updated with major system version") {

        mock_system_version = SYSTEM_VERSION_DEFAULT(9, 1, 1);
        SystemVersionInfo sys_ver = {};
        sys_ver.size = sizeof(SystemVersionInfo);

        char expected_buf[] = "device.spark.io";
        memcpy(&server_addr, backup_tcp_public_server_address, sizeof(backup_tcp_public_server_address));

        char tmphost[sizeof(server_addr.domain) + 32] = {};
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        int written = system_string_interpolate(server_addr.domain, tmphost, sizeof(tmphost), fn);

        REQUIRE_THAT(tmphost, Equals(expected_buf));

        // expecting not to alter TCP PSK
        REQUIRE(written == (backup_tcp_public_server_address_size - 3));
    }

    SECTION("system_string_interpolate() returns when passed a null pointer") {

        mock_system_version = SYSTEM_VERSION_DEFAULT(1, 2, 3);
        SystemVersionInfo sys_ver = {};
        sys_ver.size = sizeof(SystemVersionInfo);

        char tmphost[sizeof(server_addr.domain) + 32] = {};
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        int written = system_string_interpolate(server_addr.domain, nullptr, sizeof(tmphost), fn);
        REQUIRE(written == 0);

        written = system_string_interpolate(nullptr, tmphost, sizeof(tmphost), fn);
        REQUIRE(written == 0);
    }

    SECTION("system_string_interpolate() cannot force a buffer overflow") {

        mock_system_version = SYSTEM_VERSION_DEFAULT(255, 2, 3);
        SystemVersionInfo sys_ver = {};
        sys_ver.size = sizeof(SystemVersionInfo);

        int major_version = get_major_version(&sys_ver);
        int major_version_size = get_major_version_size(major_version) + 2;

        // 95 chars is the max
        ServerAddress server_addr = {};
        char fake_domain1[] = "$id.udp-mesh.particle.io.udp-mesh.particle.io.udp-mesh.particle.io.ud";
        char expected_str1[] = "123412341234123412341234.v";
        char expected_str2[] = ".udp-mesh.particle.io.udp-mesh.particle.io.udp-mesh.particle.io.ud";
        char expected_buf1[sizeof(server_addr.domain) + 40] = {};
        sprintf(expected_buf1, "%s%d%s", expected_str1, major_version, expected_str2);
        memcpy(&server_addr.domain, fake_domain1, sizeof(fake_domain1));

        char tmphost1[sizeof(server_addr.domain) + 32] = {};
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        int written = system_string_interpolate(server_addr.domain, tmphost1, sizeof(tmphost1), fn);
        // printf("written1 %d, sizeof(tmphost1) %lu\r\n", written, sizeof(tmphost1));
        // for (int i=0; i< sizeof(tmphost1); i++) {
        //     printf("%02x", tmphost1[i]);
        // }
        // printf("\r\n");
        REQUIRE_THAT(tmphost1, Equals(expected_buf1));
        REQUIRE(written == 24 + strlen(fake_domain1) - 3 + major_version_size);

        // 96 chars is 1 too many
        char fake_domain2[] = "$id.udp-mesh.particle.io.udp-mesh.particle.io.udp-mesh.particle.io.udp";
        char expected_str3[] = "123412341234123412341234.v";
        char expected_str4[] = ".udp-mesh.particle.io.udp-mesh.particle.io.udp-mesh.particle.io.ud";
        char expected_buf2[sizeof(server_addr.domain) + 40] = {};
        sprintf(expected_buf2, "%s%d%s", expected_str3, major_version, expected_str4);
        memcpy(&server_addr.domain, fake_domain2, sizeof(fake_domain2));
        REQUIRE_THAT(server_addr.domain, Equals(fake_domain2));

        char tmphost2[sizeof(server_addr.domain) + 32] = {};
        written = system_string_interpolate(server_addr.domain, tmphost2, sizeof(tmphost2), fn);
        // printf("written2 %d, sizeof(tmphost2) %lu\r\n", written, sizeof(tmphost2));
        // for (int i=0; i< sizeof(tmphost2); i++) {
        //     printf("%02x", tmphost2[i]);
        // }
        // printf("\r\n");
        REQUIRE_THAT(tmphost2, Equals(expected_buf2));
        // It only wrote 1 less than original input
        REQUIRE(written == 24 + strlen(fake_domain2) - 3 - 1 + major_version_size);
    }

    SECTION("system_string_interpolate() $id token can be anywhere in the string") {

        mock_system_version = SYSTEM_VERSION_DEFAULT(255, 2, 3);
        SystemVersionInfo sys_ver = {};
        sys_ver.size = sizeof(SystemVersionInfo);

        int major_version = get_major_version(&sys_ver);
        int major_version_size = get_major_version_size(major_version) + 2;

        // 95 chars is the max
        ServerAddress server_addr = {};
        char fake_domain1[] = "udp-mesh.particle.io.udp-mesh.particle.io.udp-mesh.particle.io.udp$id";
        char expected_str2[] = "udp-mesh.particle.io.udp-mesh.particle.io.udp-mesh.particle.io.udp123412341234123412341234.v";
        char expected_buf1[sizeof(server_addr.domain) + 40] = {};
        sprintf(expected_buf1, "%s%d", expected_str2, major_version);
        memcpy(&server_addr.domain, fake_domain1, sizeof(fake_domain1));

        char tmphost1[sizeof(server_addr.domain) + 32] = {};
        string_interpolate_source_t fn = system_interpolate_cloud_server_hostname;
        int written = system_string_interpolate(server_addr.domain, tmphost1, sizeof(tmphost1), fn);
        // printf("written1 %d, sizeof(tmphost1) %lu\r\n", written, sizeof(tmphost1));
        // for (int i=0; i< sizeof(tmphost1); i++) {
        //     printf("%02x", tmphost1[i]);
        // }
        // printf("\r\n");
        REQUIRE_THAT(tmphost1, Equals(expected_buf1));
        REQUIRE(written == 24 + strlen(fake_domain1) - 3 + major_version_size);

        // 96 chars is 1 too many
        char fake_domain2[] = "udp-mesh.particle.io.udp-mesh.particle.io.udp-mesh.particle.io.udp.$id";
        char expected_str4[] = "udp-mesh.particle.io.udp-mesh.particle.io.udp-mesh.particle.io.udp.";
        char expected_buf2[sizeof(server_addr.domain) + 40] = {};
        sprintf(expected_buf2, "%s", expected_str4);
        memcpy(&server_addr.domain, fake_domain2, sizeof(fake_domain2));
        REQUIRE_THAT(server_addr.domain, Equals(fake_domain2));

        char tmphost2[sizeof(server_addr.domain) + 32] = {};
        written = system_string_interpolate(server_addr.domain, tmphost2, sizeof(tmphost2), fn);
        // printf("written2 %d, sizeof(tmphost2) %lu\r\n", written, sizeof(tmphost2));
        // for (int i=0; i< sizeof(tmphost2); i++) {
        //     printf("%02x", tmphost2[i]);
        // }
        // printf("\r\n");
        REQUIRE_THAT(tmphost2, Equals(expected_buf2));
        // It only wrote 1 less than original input
        REQUIRE(written == strlen(expected_buf2));
    }
}
