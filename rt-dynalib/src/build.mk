
TARGET_RT_DYNALIB_SRC_PATH = $(RT_DYNALIB_MODULE_PATH)/src

CPPSRC += $(call target_files,$(TARGET_RT_DYNALIB_SRC_PATH),*.cpp)
CSRC += $(call target_files,$(TARGET_RT_DYNALIB_SRC_PATH),*.c)

# gcc includes a number of C rt functions as builtins, which causes the
# stubs to not compile since they have a different method signature to the already
# declared builtin
BUILTINS_EXCLUDE = malloc free

CFLAGS += $(addprefix -fno-builtin-,$(BUILTINS_EXCLUDE))
