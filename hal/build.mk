
# all build.mk files are loaded recursively
# This project has these build.mk files which act as "gatekeepers"
# pulling in the required sources.
# (Include files are selected in import.mk)

HAL_PLATFORM_SRC_PATH = $(HAL_MODULE_PATH)/src/$(PLATFORM_NAME)
include $(call rwildcard,$(HAL_PLATFORM_SRC_PATH)/,sources.mk)

LOG_MODULE_CATEGORY = hal

ifneq (,$(filter $(PLATFORM_ID),13 15 23 25 26))
ifneq ($(DEBUG_BUILD),y)
CFLAGS += -DLOG_COMPILE_TIME_LEVEL=LOG_LEVEL_ERROR
endif
endif
