#include <memory>
#include <cstring>
#include <cstdio>

#include "ota_flash_hal.h"
#include "device_config.h"
#include "service_debug.h"
#include "core_hal.h"
#include "filesystem.h"
#include "bytes2hexbuf.h"

using namespace particle;

namespace {

config::Describe defaultDescribe() {
    auto desc = config::Describe::fromString("{\"p\": 0,\"m\":[{\"f\":\"m\",\"n\":\"0\",\"v\":0,\"d\":[]}]}");
    desc.platformId(deviceConfig.platform_id);
    auto modules = desc.modules();
    for (auto& module: modules) {
        module.version(MODULE_VERSION);
    }
    desc.modules(modules);
    return desc;
}

module_dependency_t halModuleDependency(const config::ModuleDependencyInfo& info) {
    return {
        .module_function = info.function(),
        .module_index = info.index(),
        .module_version = info.version()
    };
}

module_bounds_location_t moduleLocation(const config::ModuleInfo& module) {
    if (module.function() == MODULE_FUNCTION_NCP_FIRMWARE) {
        return MODULE_BOUNDS_LOC_NCP_FLASH;
    }
    return MODULE_BOUNDS_LOC_INTERNAL_FLASH;
}

} // namespace

int HAL_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
    if (!create) {
        delete[] info->modules;
        return 0;
    }
    const auto& desc = deviceConfig.describe.isValid() ? deviceConfig.describe : defaultDescribe();
    std::unique_ptr<hal_module_t[]> halModules(new(std::nothrow) hal_module_t[desc.modules().size()]);
    if (!halModules) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    uint32_t moduleAddr = 0;
    for (size_t i = 0; i < desc.modules().size(); ++i) {
        const auto& module = desc.modules().at(i);
        auto& halModule = halModules[i];
        halModule = {
            .bounds = {
                .maximum_size = module.maximumSize(),
                .start_address = moduleAddr,
                .end_address = moduleAddr + module.maximumSize(),
                .module_function = module.function(),
                .module_index = module.index(),
                .store = module.store(),
                .mcu_identifier = 0,
                .location = moduleLocation(module)
            },
            .info = {
                .module_start_address = (const char*)0 + moduleAddr,
                .module_end_address = (const char*)0 + moduleAddr + module.maximumSize(),
                .reserved = 0,
                .flags = 0,
                .module_version = module.version(),
                .platform_id = deviceConfig.platform_id,
                .module_function = module.function(),
                .module_index = module.index(),
                .dependency = {
                    .module_function = MODULE_FUNCTION_NONE,
                    .module_index = 0,
                    .module_version = 0
                },
                .dependency2 = {
                    .module_function = MODULE_FUNCTION_NONE,
                    .module_index = 0,
                    .module_version = 0
                }
            },
            .crc = {
                .crc32 = 0
            },
            .suffix = {
                .reserved = 0,
                .sha = {},
                .size = 36
            },
            .validity_checked = module.validityChecked(),
            .validity_result = module.validityResult(),
            .module_info_offset = 0
        };
        if (module.dependencies().size() >= 1) {
            halModule.info.dependency = halModuleDependency(module.dependencies().at(0));
        }
        if (module.dependencies().size() >= 2) {
            halModule.info.dependency2 = halModuleDependency(module.dependencies().at(1));
        }
        moduleAddr += module.maximumSize();
    }
    info->platform_id = deviceConfig.platform_id;
    info->module_count = desc.modules().size();
    info->modules = halModules.release();
    return 0;
}

uint32_t HAL_OTA_FlashAddress()
{
    return 0;
}

uint32_t HAL_OTA_FlashLength()
{
    return 10 * 1024 * 1024;
}

uint16_t HAL_OTA_ChunkSize()
{
    return 512;
}

FILE* output_file;

bool HAL_FLASH_Begin(uint32_t sFLASH_Address, uint32_t fileSize, void* reserved)
{
    output_file = fopen("output.bin", "wb");
    DEBUG("flash started");
    return output_file;
}

int HAL_FLASH_Update(const uint8_t *pBuffer, uint32_t address, uint32_t length, void* reserved)
{
	DEBUG("flash write %d %d", address, length);
	fseek(output_file, address, SEEK_SET);
    fwrite(pBuffer, length, 1, output_file);
    return 0;
}

int HAL_FLASH_OTA_Validate(bool userDepsOptional, module_validation_flags_t flags, void* reserved)
{
    return 0; // FIXME
}

int HAL_FLASH_End(void* reserved)
{
	 fclose(output_file);
	 output_file = NULL;
     return HAL_UPDATE_APPLIED;
}

int HAL_FLASH_ApplyPendingUpdate(bool dryRun, void* reserved)
{
    return SYSTEM_ERROR_UNKNOWN;
}

/**
 * Set the claim code for this device.
 * @param code  The claim code to set. If null, clears the claim code.
 * @return 0 on success.
 */
uint16_t HAL_Set_Claim_Code(const char* code)
{
    return 0;
}

/**
 * Retrieves the claim code for this device.
 * @param buffer    The buffer to recieve the claim code.
 * @param len       The maximum size of the code to copy to the buffer, including the null terminator.
 * @return          0 on success.
 */
uint16_t HAL_Get_Claim_Code(char* buffer, unsigned len)
{
    *buffer = 0;
    return 0;
}

bool HAL_IsDeviceClaimed(void* reserved)
{
	return false;
}


#define EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH 128

// todo - duplicate from core, factor this down into a common area
void parseServerAddressData(ServerAddress* server_addr, uint8_t* buf)
{
  // Internet address stored on external flash may be
  // either a domain name or an IP address.
  // It's stored in a type-length-value encoding.
  // First byte is type, second byte is length, the rest is value.

  switch (buf[0])
  {
    case IP_ADDRESS:
      server_addr->addr_type = IP_ADDRESS;
      server_addr->ip = (buf[2] << 24) | (buf[3] << 16) |
                        (buf[4] << 8)  |  buf[5];
      break;

    case DOMAIN_NAME:
      if (buf[1] <= EXTERNAL_FLASH_SERVER_DOMAIN_LENGTH - 2)
      {
        server_addr->addr_type = DOMAIN_NAME;
        memcpy(server_addr->domain, buf + 2, buf[1]);

        // null terminate string
        char *p = server_addr->domain + buf[1];
        *p = 0;
        break;
      }
      // else fall through

    default:
      server_addr->addr_type = INVALID_INTERNET_ADDRESS;
  }

}


#define MAXIMUM_CLOUD_KEY_LEN (512)
#define SERVER_ADDRESS_OFFSET (384)
#define SERVER_ADDRESS_OFFSET_EC (192)
#define SERVER_ADDRESS_SIZE (128)
#define SERVER_PUBLIC_KEY_SIZE (294)
#define SERVER_PUBLIC_KEY_EC_SIZE (320)


void HAL_FLASH_Read_ServerAddress(ServerAddress* server_addr)
{
	memset(server_addr, 0, sizeof(ServerAddress));
	int offset = HAL_Feature_Get(FEATURE_CLOUD_UDP) ? SERVER_ADDRESS_OFFSET_EC : SERVER_ADDRESS_OFFSET;
    parseServerAddressData(server_addr, deviceConfig.server_key+offset);
}

void HAL_FLASH_Write_ServerAddress(const uint8_t *buf, bool udp)
{
    int offset = (udp) ? SERVER_ADDRESS_OFFSET_EC : SERVER_ADDRESS_OFFSET;
    memcpy(deviceConfig.server_key+offset, buf, SERVER_ADDRESS_SIZE);
}

bool HAL_OTA_Flashed_GetStatus(void)
{
    return false;
}

void HAL_OTA_Flashed_ResetStatus(void)
{
}

#define PUBLIC_KEY_LEN 294
#define PRIVATE_KEY_LEN 612

void HAL_FLASH_Read_ServerPublicKey(uint8_t *keyBuffer)
{
    memcpy(keyBuffer, deviceConfig.server_key, PUBLIC_KEY_LEN);
    char buf[PUBLIC_KEY_LEN*2];
    bytes2hexbuf(keyBuffer, PUBLIC_KEY_LEN, buf);
    INFO("server key: %s", buf);
}

void HAL_FLASH_Write_ServerPublicKey(const uint8_t *keyBuffer, bool udp)
{
    if (udp) {
        memcpy(&deviceConfig.server_key, keyBuffer, SERVER_PUBLIC_KEY_EC_SIZE);
    } else {
        memcpy(&deviceConfig.server_key, keyBuffer, SERVER_PUBLIC_KEY_SIZE);
    }
}

int HAL_FLASH_Read_CorePrivateKey(uint8_t *keyBuffer, private_key_generation_t* generation)
{
    memcpy(keyBuffer, deviceConfig.device_key, PRIVATE_KEY_LEN);
    char buf[PRIVATE_KEY_LEN*2];
    bytes2hexbuf(keyBuffer, PRIVATE_KEY_LEN, buf);
    INFO("device key: %s", buf);
    return 0;
}

extern "C" void random_seed_from_cloud(unsigned int value)
{
}

void HAL_OTA_Add_System_Info(hal_system_info_t* info, bool create, void* reserved)
{
}
