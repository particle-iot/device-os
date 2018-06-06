TARGET_OPENTHREAD_SRC_PATH = $(OPENTHREAD_MODULE_PATH)/openthread

CPPSRC += $(call target_files,$(TARGET_OPENTHREAD_SRC_PATH)/src/core/,*.cpp)
CSRC += $(call target_files,$(TARGET_OPENTHREAD_SRC_PATH)/src/core/,*.c)

INCLUDE_DIRS += $(TARGET_OPENTHREAD_SRC_PATH)/examples/platforms

INCLUDE_DIRS += $(TARGET_OPENTHREAD_SRC_PATH)/include/openthread
