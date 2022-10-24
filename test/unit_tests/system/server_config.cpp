#include <algorithm>

#include <catch2/catch.hpp>
#include <fakeit.hpp>

#include "server_config.h"
#include "parse_server_address.h"
#include "endian_util.h"

#include "mock/core_hal_mock.h"
#include "mock/dct_hal_mock.h"
#include "mock/mbedtls_mock.h"
#include "stub/dct.h"
#include "util/random.h"

using namespace particle;
using namespace particle::test;

using namespace fakeit;

namespace {

void verifyServerPublicKey(DctHalMock& dctHal, const std::string& expectedKeyData) {
    Verify(Method(dctHal, writeAppData).Using(DCT_ALT_SERVER_PUBLIC_KEY_OFFSET, expectedKeyData));
}

void verifyServerAddress(DctHalMock& dctHal, const std::string& expectedAddrStr, uint16_t expectedPort) {
    ServerAddress expectedAddr = {};
    REQUIRE(parseServerAddressString(&expectedAddr, expectedAddrStr.data()) == 0);
    expectedAddr.port = expectedPort;
    Verify(Method(dctHal, writeAppData).Matching([&](uint32_t offs, const std::string& data) {
        if (offs != DCT_ALT_SERVER_ADDRESS_OFFSET || data.size() != DCT_ALT_SERVER_ADDRESS_SIZE) {
            return false;
        }
        ServerAddress addr = {};
        parseServerAddressData(&addr, (const uint8_t*)data.data(), data.size());
        return memcmp(&addr, &expectedAddr, sizeof(addr)) == 0;
    }));
}

std::string encodeServerAddress(const std::string& addrStr, uint16_t port) {
    ServerAddress addr = {};
    REQUIRE(parseServerAddressString(&addr, addrStr.data()) == 0);
    addr.port = port;
    std::string addrData(DCT_ALT_SERVER_ADDRESS_SIZE, 0xff);
    REQUIRE(encodeServerAddressData(&addr, (uint8_t*)addrData.data(), addrData.size()) == 0);
    return addrData;
}

} // namespace

TEST_CASE("ServerConfig") {
    CoreHalMock coreHal;
    DctHalMock dctHal;
    MbedtlsMock mbedtls;

    When(Method(coreHal, featureGet).Using(FEATURE_CLOUD_UDP)).AlwaysReturn(true);
    When(Method(dctHal, writeAppData)).AlwaysReturn(0);
    When(Method(mbedtls, mdStarts)).AlwaysReturn(0);
    When(Method(mbedtls, mdFinish)).AlwaysReturn(0);
    When(Method(mbedtls, mdUpdate)).AlwaysReturn(0);
    When(Method(mbedtls, pkParsePublicKey)).AlwaysReturn(0);
    When(Method(mbedtls, pkVerify)).AlwaysReturn(0);
    When(Method(mbedtls, asn1GetTag)).AlwaysReturn(0);

    auto pubKey = randString(DCT_ALT_SERVER_PUBLIC_KEY_SIZE);

    auto servConf = ServerConfig::instance();

    SECTION("updateSettings()") {
        SECTION("updates the server settings in the DCT") {
            SECTION("domain name") {
                ServerConfig::ServerSettings conf;
                conf.address = "foo.bar";
                conf.publicKey = (const uint8_t*)pubKey.data();
                conf.publicKeySize = pubKey.size();
                conf.port = 1234;
                CHECK(servConf->updateSettings(conf) == 0);
                verifyServerPublicKey(dctHal, pubKey);
                verifyServerAddress(dctHal, "foo.bar", 1234);
            }
            SECTION("IP address") {
                ServerConfig::ServerSettings conf;
                conf.address = "127.0.0.1";
                conf.publicKey = (const uint8_t*)pubKey.data();
                conf.publicKeySize = pubKey.size();
                conf.port = 1234;
                CHECK(servConf->updateSettings(conf) == 0);
                verifyServerPublicKey(dctHal, pubKey);
                verifyServerAddress(dctHal, "127.0.0.1", 1234);
            }
        }

        SECTION("clears the data in the DCT before updating the server settings") {
            ServerConfig::ServerSettings conf;
            conf.address = "foo.bar";
            conf.publicKey = (const uint8_t*)pubKey.data();
            conf.publicKeySize = pubKey.size();
            conf.port = 1234;
            CHECK(servConf->updateSettings(conf) == 0);
            Verify(
                Method(dctHal, writeAppData).Using(DCT_ALT_SERVER_ADDRESS_OFFSET, std::string(DCT_ALT_SERVER_ADDRESS_SIZE, 0xff)),
                Method(dctHal, writeAppData).Using(DCT_ALT_SERVER_PUBLIC_KEY_OFFSET, std::string(DCT_ALT_SERVER_PUBLIC_KEY_SIZE, 0xff)),
                Method(dctHal, writeAppData).Using(DCT_ALT_SERVER_PUBLIC_KEY_OFFSET, _), // The contents is validated in other tests
                Method(dctHal, writeAppData).Using(DCT_ALT_SERVER_ADDRESS_OFFSET, _) // ditto
            );
            VerifyNoOtherInvocations(Method(dctHal, writeAppData));
        }
    }

    SECTION("validateSettings()") {
        std::string addrData = encodeServerAddress("foo.bar", 1234);

        When(Method(dctHal, readAppDataCopy)).AlwaysDo([&](uint32_t offs, void* data, size_t size) {
            switch (offs) {
            case DCT_ALT_SERVER_PUBLIC_KEY_OFFSET: {
                memcpy(data, pubKey.data(), std::min(pubKey.size(), size));
                return 0;
            }
            case DCT_ALT_SERVER_ADDRESS_OFFSET: {
                memcpy(data, addrData.data(), std::min(addrData.size(), size));
                return 0;
            }
            default:
                return -1;
            }
        });

        SECTION("succeeds if the server settings are valid") {
            CHECK(servConf->validateSettings() == 0);
        }

        SECTION("fails if the server settings are invalid") {
            SECTION("invalid address") {
                addrData = std::string(DCT_ALT_SERVER_ADDRESS_SIZE, 0xff);
                CHECK(servConf->validateSettings() == SYSTEM_ERROR_INVALID_SERVER_SETTINGS);
            }
            SECTION("invalid public key size") {
                When(Method(mbedtls, asn1GetTag)).Return(-1);
                CHECK(servConf->validateSettings() == SYSTEM_ERROR_INVALID_SERVER_SETTINGS);
            }
            SECTION("invalid public key data") {
                When(Method(mbedtls, pkParsePublicKey)).Return(-1);
                CHECK(servConf->validateSettings() == SYSTEM_ERROR_INVALID_SERVER_SETTINGS);
            }
        }
    }

    SECTION("validateServerMovedRequest()") {
        std::string addr = "foo.bar";
        auto sign = randString(100);

        ServerConfig::ServerMovedRequest req;
        req.address = addr.data();
        req.publicKey = (const uint8_t*)pubKey.data();
        req.publicKeySize = pubKey.size();
        req.port = 1234;
        req.signature = (const uint8_t*)sign.data();
        req.signatureSize = sign.size();

        SECTION("succeeds if the request data is valid") {
            CHECK(servConf->validateServerMovedRequest(req) == 0);
        }

        SECTION("fails if the signature of the request is invalid") {
            When(Method(mbedtls, pkVerify)).Return(-1);
            CHECK(servConf->validateServerMovedRequest(req) == SYSTEM_ERROR_BAD_DATA);
        }

        SECTION("computes a hash of the server settings correctly") {
            servConf->validateServerMovedRequest(req);
            Verify(
                Method(mbedtls, mdStarts),
                Method(mbedtls, mdUpdate).Matching([&](mbedtls_md_context_t*, const std::string& data) {
                    auto val = nativeToLittleEndian<uint32_t>(pubKey.size());
                    return data.size() == sizeof(val) && memcmp(data.data(), &val, sizeof(val)) == 0;
                }),
                Method(mbedtls, mdUpdate).Matching([&](mbedtls_md_context_t*, const std::string& data) {
                    return data == pubKey;
                }),
                Method(mbedtls, mdUpdate).Matching([&](mbedtls_md_context_t*, const std::string& data) {
                    auto val = nativeToLittleEndian<uint32_t>(addr.size());
                    return data.size() == sizeof(val) && memcmp(data.data(), &val, sizeof(val)) == 0;
                }),
                Method(mbedtls, mdUpdate).Matching([&](mbedtls_md_context_t*, const std::string& data) {
                    return data == addr;
                }),
                Method(mbedtls, mdUpdate).Matching([&](mbedtls_md_context_t*, const std::string& data) {
                    auto val = nativeToLittleEndian<uint16_t>(req.port);
                    return data.size() == sizeof(val) && memcmp(data.data(), &val, sizeof(val)) == 0;
                }),
                Method(mbedtls, mdFinish)
            );
            VerifyNoOtherInvocations(Method(mbedtls, mdUpdate));
        }
    }

    SECTION("restoreDefaultSettings()") {
        SECTION("saves the default server settings to the DCT") {
            CHECK(servConf->restoreDefaultSettings() == 0);
            std::vector<uint8_t> defaultPubKey = {
                0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
                0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
                0x42, 0x00, 0x04, 0x50, 0x9b, 0xfc, 0x18, 0x56, 0x48, 0xc3, 0x3f, 0x80,
                0x90, 0x7a, 0xe1, 0x32, 0x60, 0xdf, 0x33, 0x28, 0x21, 0x15, 0x20, 0x9e,
                0x54, 0xa2, 0x2f, 0x2b, 0x10, 0x59, 0x84, 0xa4, 0x63, 0x62, 0xc0, 0x7c,
                0x26, 0x79, 0xf6, 0xe4, 0xce, 0x76, 0xca, 0x00, 0x2d, 0x3d, 0xe4, 0xbf,
                0x2e, 0x9e, 0x3a, 0x62, 0x15, 0x1c, 0x48, 0x17, 0x9b, 0xd8, 0x09, 0xdd,
                0xce, 0x9c, 0x5d, 0xc3, 0x0f, 0x54, 0xb8
            };
            verifyServerPublicKey(dctHal, std::string((const char*)defaultPubKey.data(), defaultPubKey.size()));
            verifyServerAddress(dctHal, "$id.udp.particle.io", 5684);
        }
    }
}
