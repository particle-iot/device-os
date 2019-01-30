TARGET_OPENTHREAD_PATH = $(OPENTHREAD_MODULE_PATH)
CFLAGS += -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"openthread-config-project.h\"

INCLUDE_DIRS += $(TARGET_OPENTHREAD_PATH)/openthread/include
INCLUDE_DIRS += $(TARGET_OPENTHREAD_PATH)/openthread/src/core

INCLUDE_DIRS += $(TARGET_OPENTHREAD_PATH)/openthread/examples/platforms

ifeq ($(PLATFORM_OPENTHREAD),nrf52840)
#CFLAGS += -DDISABLE_CC310=1
CFLAGS += -DENABLE_FEM=1
CFLAGS += -DNRF_802154_PROJECT_CONFIG=\"openthread-platform-config.h\"
CFLAGS += -DRAAL_SOFTDEVICE=1
INCLUDE_DIRS += $(TARGET_OPENTHREAD_PATH)/openthread/third_party/NordicSemiconductor/drivers/radio
INCLUDE_DIRS += $(TARGET_OPENTHREAD_PATH)/openthread/third_party/NordicSemiconductor/drivers/radio/hal
INCLUDE_DIRS += $(TARGET_OPENTHREAD_PATH)/openthread/third_party/NordicSemiconductor/drivers/radio/rsch
INCLUDE_DIRS += $(TARGET_OPENTHREAD_PATH)/openthread/third_party/NordicSemiconductor/drivers/radio/rsch/raal
INCLUDE_DIRS += $(TARGET_OPENTHREAD_PATH)/openthread/third_party/NordicSemiconductor/drivers/radio/rsch/raal/softdevice
endif
