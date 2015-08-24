RTOS_MODULE_NAME = rtos
RTOS_MODULE_PATH ?= $(PROJECT_ROOT)/$(RTOS_MODULE_NAME)
RTOS_BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)

RTOS_LIB_DIR = $(BUILD_PATH_BASE)/$(RTOS_MODULE_NAME)/$(RTOS_BUILD_PATH_EXT)
RTOS_LIB_DEP = $(RTOS_LIB_DIR)/lib$(RTOS_MODULE_NAME).a

# RTOS specific defines
CFLAGS += 

# pull in the includes/sources corresponding to the rtos vendor

INCLUDE_DIRS += 
FREERTOS_PATH=$(RTOS_MODULE_PATH)/FreeRTOS
include $(call rwildcard,$(FREERTOS_PATH)/,include.mk)
