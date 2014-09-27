HAL_MODULE_PATH?=../hal

LIB_DIRS += $(BUILD_PATH_BASE)/hal/prod-$(SPARK_PRODUCT_ID)
LIBS += hal

include $(call rwildcard,$(HAL_MODULE_PATH)/,include.mk)
