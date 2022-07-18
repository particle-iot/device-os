TARGET_COREMARK_SRC_PATH = $(COREMARK_MODULE_PATH)/coremark

# C source files included in this build.
CSRC += $(TARGET_COREMARK_SRC_PATH)/core_list_join.c
CSRC += $(TARGET_COREMARK_SRC_PATH)/core_main.c
CSRC += $(TARGET_COREMARK_SRC_PATH)/core_matrix.c
CSRC += $(TARGET_COREMARK_SRC_PATH)/core_portme.c
CSRC += $(TARGET_COREMARK_SRC_PATH)/core_state.c
CSRC += $(TARGET_COREMARK_SRC_PATH)/core_util.c