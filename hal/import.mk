HAL_MODULE_PATH?=../hal

LIB_DIRS += $(BUILD_PATH_BASE)/hal/prod-$(SPARK_PRODUCT_ID)

include $(call rwildcard,$(HAL_MODULE_PATH)/,include.mk)

ifeq "$(ARCH)" "gcc"
LIBS += boost_system-mgw48-mt-1_57 ws2_32 wsock32
LIB_DIRS += $(BOOST_ROOT)/stage/lib
endif
