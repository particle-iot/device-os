TARGET_WIZNET_DRIVER_SRC_PATH = $(WIZNET_DRIVER_MODULE_PATH)/wiznet_driver

# C source files included in this build.
CSRC += $(TARGET_WIZNET_DRIVER_SRC_PATH)/Ethernet/wizchip_conf.c

CFLAGS += -Wno-missing-braces -Wno-parentheses -Wno-unused-variable

ifeq ("$(PLATFORM_WIZNET)","W5500")
CSRC += $(TARGET_WIZNET_DRIVER_SRC_PATH)/Ethernet/W5500/w5500.c
endif
