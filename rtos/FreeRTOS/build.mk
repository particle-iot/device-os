# all build.mk files are loaded recursively
# This project has these build.mk files which act as "gatekeepers" only
# pulling in the required sources.
# (Include files are selected in import.mk)

FREERTOS_PATH = $(RTOS_MODULE_PATH)/FreeRTOS
include $(call rwildcard,$(FREERTOS_PATH)/,sources.mk)