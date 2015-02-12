MODULE=user
USER_MODULE_PATH=.

BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(USER_FLAVOR)

ifeq (y,$(DYNALIB))
HAL_LINK := 
include ../user-part/import.mk
include ../user-part/build.mk
include ../user-part/makefile

else

DEPENDENCIES = wiring system services communication hal

include ../build/platform-id.mk

ifeq ("","$(SPARK_NO_PLATFORM)")
DEPENDENCIES += platform
ifneq ("$(PLATFORM_NET)","BCM9WCDUSI09")
CFLAGS += -DSPARK_PLATFORM
CFLAGS += -DFLASHEE_EEPROM
endif
endif

MAKE_DEPENDENCIES = $(DEPENDENCIES)
LIBS += $(MAKE_DEPENDENCIES)
LIB_DEPS += $(WIRING_LIB_DEP) $(SYSTEM_LIB_DEP) $(SERVICES_LIB_DEP) $(COMMUNICATION_LIB_DEP) $(HAL_LIB_DEP) $(PLATFORM_LIB_DEP)
LIB_DIRS += $(dir $(LIB_DEPS))


# Target this makefile is building.
ifeq ("$(ARCH)","gcc")
TARGET=exe
else
TARGET=elf bin lst hex size 
endif


include ../build/arm-tlm.mk

endif
