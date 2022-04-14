TARGET_LWIP_PATH = $(LWIP_MODULE_PATH)
INCLUDE_DIRS += $(LWIP_MODULE_PATH)/lwip/src/include

ifeq ("$(PLATFORM_LWIP)","posix")
INCLUDE_DIRS += $(LWIP_MODULE_PATH)/lwip-contrib/ports/unix/port/include
endif
