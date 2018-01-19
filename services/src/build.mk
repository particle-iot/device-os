
TARGET_SPARK_SERVICES_SRC_PATH = $(SERVICES_MODULE_PATH)/src
NANOPB_SRC_PATH = $(SERVICES_MODULE_PATH)/nanopb

CPPSRC += $(call target_files,$(TARGET_SPARK_SERVICES_SRC_PATH),*.cpp)
CSRC += $(call target_files,$(TARGET_SPARK_SERVICES_SRC_PATH),*.c)

# nanopb
CSRC += $(NANOPB_SRC_PATH)/pb_common.c
CSRC += $(NANOPB_SRC_PATH)/pb_encode.c
CSRC += $(NANOPB_SRC_PATH)/pb_decode.c

CPPFLAGS += -std=gnu++11

LOG_MODULE_CATEGORY = service
