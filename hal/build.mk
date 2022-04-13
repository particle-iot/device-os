
# all build.mk files are loaded recursively
# This project has these build.mk files which act as "gatekeepers"
# pulling in the required sources.
# (Include files are selected in import.mk)

HAL_PLATFORM_SRC_PATH = $(HAL_MODULE_PATH)/src/$(PLATFORM_NAME)
include $(call rwildcard,$(HAL_PLATFORM_SRC_PATH)/,sources.mk)

LOG_MODULE_CATEGORY = hal

# if PLATFORM_ID matches 13 23 25 or 26, and not DEBUG_BUILD or HYBRID_BUILD, set LOG_LEVEL_ERROR
ifneq (,$(filter $(PLATFORM_ID),13 23 25 26))
ifneq ($(DEBUG_BUILD),y)
ifneq ($(HYBRID_BUILD),y)
CFLAGS += -DLOG_COMPILE_TIME_LEVEL=LOG_LEVEL_ERROR
endif
endif
endif
