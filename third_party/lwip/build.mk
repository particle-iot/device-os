TARGET_LWIP_SRC_PATH = $(LWIP_MODULE_PATH)/lwip/src

LWIPDIR = $(TARGET_LWIP_SRC_PATH)
include $(TARGET_LWIP_SRC_PATH)/Filelists.mk

CSRC += $(LWIPNOAPPSFILES)

ifeq ("$(PLATFORM_LWIP)","posix")
CSRC += $(LWIP_MODULE_PATH)/lwip-contrib/ports/unix/port/netif/tapif.c
endif
