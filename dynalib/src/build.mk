
TARGET_DYNALIB_SRC_PATH = $(DYNALIB_MODULE_PATH)/src

CPPSRC += $(call target_files,$(TARGET_DYNALIB_SRC_PATH),*.cpp)
CSRC += $(call target_files,$(TARGET_DYNALIB_SRC_PATH),*.c)

