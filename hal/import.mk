HAL_MODULE_PATH?=../hal

HAL_LIB_DIR = $(BUILD_PATH_BASE)/hal/$(BUILD_TARGET_PLATFORM)$(HAL_TEST_FLAVOR)
HAL_LIB_DEP = $(HAL_LIB_DIR)/libhal.a

include $(call rwildcard,$(HAL_MODULE_PATH)/,include.mk)


ifeq "$(ARCH)" "gcc"
# additional libraries required by gcc build
LIBS += boost_system-mgw48-mt-1_57 ws2_32 wsock32
LIB_DIRS += $(BOOST_ROOT)/stage/lib

# gcc HAL is different for test driver and test subject
ifeq "$(SPARK_TEST_DRIVER)" "1"
HAL_TEST_FLAVOR+=-driver
endif

endif
