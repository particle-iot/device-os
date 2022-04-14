# all build.mk files are loaded recursively
# This project has these build.mk files one level down which act as "gatekeepers" only
# pulling in the required sources.
# (Include files are selected in import.mk)

PLATFORM_NET_PATH = $(PLATFORM_MODULE_PATH)/NET/$(PLATFORM_NET)
include $(call rwildcard,$(PLATFORM_NET_PATH)/,sources.mk)
