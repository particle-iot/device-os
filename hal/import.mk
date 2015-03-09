HAL_MODULE_NAME=hal
HAL_MODULE_PATH?=$(PROJECT_ROOT)/$(HAL_MODULE_NAME)

HAL_BUILD_PATH_EXT=$(BUILD_TARGET_PLATFORM)$(HAL_TEST_FLAVOR)$(MODULAR)
HAL_LIB_DIR = $(BUILD_PATH_BASE)/$(HAL_MODULE_NAME)/$(HAL_BUILD_PATH_EXT)
HAL_LIB_DEP = $(HAL_LIB_DIR)/lib$(HAL_MODULE_NAME).a

include $(call rwildcard,$(HAL_MODULE_PATH)/inc/,include.mk)
include $(call rwildcard,$(HAL_MODULE_PATH)/shared/,include.mk)
include $(call rwildcard,$(HAL_MODULE_PATH)/src/$(PLATFORM_NAME)/,include.mk)


ifeq "$(ARCH)" "gcc"
# additional libraries required by gcc build
ifdef SYSTEMROOT
LIBS += boost_system-mgw48-mt-1_57 ws2_32 wsock32
else
LIBS += boost_system
endif
LIB_DIRS += $(BOOST_ROOT)/stage/lib


# gcc HAL is different for test driver and test subject
ifeq "$(SPARK_TEST_DRIVER)" "1"
HAL_TEST_FLAVOR+=-driver
endif

endif
