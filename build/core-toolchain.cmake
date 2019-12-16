set(PARTICLE_COMPILE_LTO ON)
set(PARTICLE_PLATFORM core)
set(PARTICLE_PLATFORM_GEN 1)
set(PARTICLE_PLATFORM_ID 0)
set(PARTICLE_PLATFORM_MCU STM32F1xx)
set(PARTICLE_PLATFORM_NET CC3000)
set(PARTICLE_PLATFORM_THREADING 0)
set(PARTICLE_USBD_VID 0x1D50)
set(PARTICLE_USBD_PID 0x607D)

#[=[
Process build options

Create the legacy build arguments from platform specific variables and process
options supplied to top-level project.
#]=]
include(${CMAKE_CURRENT_LIST_DIR}/process-options.cmake)

# Add platform specific build arguments
add_compile_definitions(
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)

#[=[
Inject compiler definitions into mbedTLS project

mbedTLS is a third-party project, which can be modified by overriding options,
before including the project. However, the project also requires compile
definitions which can be injected via the toolchain file.
#]=]
add_compile_definitions(
    MBEDTLS_CONFIG_FILE=<${CMAKE_SOURCE_DIR}/crypto/inc/mbedtls_config.h>
)

add_compile_options(
    -mcpu=cortex-m3 # #error "std::atomic is not always lock-free for required types"
    -mthumb # error: target CPU does not support ARM mode
)

add_link_options(
    -nostartfiles # undefined reference to `_exit'
)

# Set system variables
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m3)
#set(CMAKE_SYSTEM_VERSION )

# Set toolchain variables
set(CMAKE_AR arm-none-eabi-gcc-ar)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_RANLIB arm-none-eabi-gcc-ranlib)

# Set override rules
# https://gitlab.kitware.com/cmake/cmake/issues/18713
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_DIR}/extension-override.cmake)
