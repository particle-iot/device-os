MAIN_MODULE_PATH ?= ../main
include $(call rwildcard,$(MAIN_MODULE_PATH)/,include.mk)

CLOUD_FLASH_URL = https://api.spark.io/v1/devices/$(SPARK_CORE_ID)\?access_token=$(SPARK_ACCESS_TOKEN)
	
CFLAGS += -DINCLUDE_PLATFORM=1