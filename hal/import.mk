HAL_MODULE_PATH?=../hal

LIB_DIRS += $(BUILD_PATH_BASE)/hal/prod-$(SPARK_PRODUCT_ID)$(HAL_TEST_FLAVOR)


include $(call rwildcard,$(HAL_MODULE_PATH)/,include.mk)

# Linker flags
LDFLAGS += -Wl,-Map,$(TARGET_BASE).map


ifeq "$(ARCH)" "gcc"
# additional libraries required by gcc build
LIBS += boost_system-mgw48-mt-1_57 ws2_32 wsock32
LIB_DIRS += $(BOOST_ROOT)/stage/lib

# gcc HAL is different for test driver and test subject
ifeq "$(SPARK_TEST_DRIVER)" "1"
HAL_TEST_FLAVOR+=-driver
endif

endif
