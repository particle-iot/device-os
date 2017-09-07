
TARGET_MAIN_SRC_PATH = $(MAIN_MODULE_PATH)/src

CPPSRC += $(call target_files_abs,$(TARGET_MAIN_SRC_PATH),*.cpp)
CSRC += $(call target_files_abs,$(TARGET_MAIN_SRC_PATH),*.c)
