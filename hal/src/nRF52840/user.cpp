
#include <stdint.h>
#include "core_hal.h"
#include "flash_mal.h"
#include "user.h"
#include "module_info.h"
#include "user_hal.h"
#include "ota_flash_hal_impl.h"


#define USER_ADDR (module_user.start_address)


#ifdef HAL_REPLACE_USER
int user_update(void)
{
    uint32_t user_image_size = 0;
    const uint8_t* user_image = HAL_User_Image(&user_image_size, nullptr);

    int updated = FLASH_CopyMemory(FLASH_INTERNAL, (uint32_t)user_image,
                                   FLASH_INTERNAL, USER_ADDR, user_image_size, MODULE_FUNCTION_USER_PART,
                                   MODULE_VERIFY_DESTINATION_IS_START_ADDRESS|MODULE_VERIFY_CRC|MODULE_VERIFY_FUNCTION);

    return updated;
}
#else
int user_update(void)
{
    return FLASH_ACCESS_RESULT_ERROR;
}
#endif // HAL_REPLACE_USER

