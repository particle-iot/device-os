# all build.mk files are loaded recursively
# This project has these build.mk files which act as "gatekeepers" only
# pulling in the required sources.
# (Include files are selected in import.mk)

PLATFORM_MCU_PATH = $(PLATFORM_MODULE_PATH)/MCU/$(PLATFORM_MCU)
include $(call rwildcard,$(PLATFORM_MCU_PATH)/,sources.mk)
