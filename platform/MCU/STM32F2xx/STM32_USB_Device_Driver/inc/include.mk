
TARGET_USB_FS_PATH = $(PLATFORM_MCU_PATH)/STM32_USB_Device_Driver
INCLUDE_DIRS += $(TARGET_USB_FS_PATH)/inc

# bootloader_dct.h is referenced in usbd_flash_if.c
INCLUDE_DIRS += $(PLATFORM_MODULE_PATH)/../bootloader/src/photon
