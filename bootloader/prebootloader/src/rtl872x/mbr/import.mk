PREBOOTLOADER_MBR_MODULE_PATH ?= $(PROJECT_ROOT)/bootloader/prebootloader/src/tron/mbr
# Prebootloader version is defined along with other modules in system_module_version.mk
include $(PREBOOTLOADER_MBR_MODULE_PATH)/../../../../../modules/shared/system_module_version.mk
PREBOOTLOADER_MBR_BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)

# bring in the include folders from inc and src/<platform> is includes
include $(call rwildcard,$(PREBOOTLOADER_MBR_MODULE_PATH),include.mk)
include $(call rwildcard,$(BOOTLOADER_SHARED_MODULAR)/,include.mk)
