TARGET_AMBD_SDK_PATH = $(AMBD_SDK_MODULE_PATH)
TARGET_AMBD_SDK_SOC_PATH = $(TARGET_AMBD_SDK_PATH)/ambd_sdk/component/soc/realtek/amebad
TARGET_AMBD_SDK_OS_PATH = $(TARGET_AMBD_SDK_PATH)/ambd_sdk/component/os
TARGET_AMBD_SDK_COMMON_PATH = $(TARGET_AMBD_SDK_PATH)/ambd_sdk/component/common
TARGET_AMBD_SDK_PROJECT_LIB_PATH = $(TARGET_AMBD_SDK_PATH)/ambd_sdk/project/realtek_amebaD_va0_example/GCC-RELEASE/project_hp/asdk/lib/application

INCLUDE_DIRS += $(TARGET_AMBD_SDK_PATH)
INCLUDE_DIRS += $(TARGET_AMBD_SDK_SOC_PATH)/cmsis
INCLUDE_DIRS += $(TARGET_AMBD_SDK_SOC_PATH)/fwlib/include
INCLUDE_DIRS += $(TARGET_AMBD_SDK_SOC_PATH)/app/monitor/include
INCLUDE_DIRS += $(TARGET_AMBD_SDK_SOC_PATH)/fwlib/usb_otg/device/inc
INCLUDE_DIRS += $(TARGET_AMBD_SDK_OS_PATH)/os_dep/include
INCLUDE_DIRS += $(TARGET_AMBD_SDK_COMMON_PATH)/api/platform
INCLUDE_DIRS += $(TARGET_AMBD_SDK_SOC_PATH)/misc
INCLUDE_DIRS += $(TARGET_AMBD_SDK_SOC_PATH)

# Hack of the century!
# Disabled for now, waiting on new USB driver implementation
# LIBS_EXT_END += $(TARGET_AMBD_SDK_PROJECT_LIB_PATH)/lib_usbd.a
