TARGET_NANOPB_SRC_PATH = $(NANOPB_MODULE_PATH)/nanopb

# C source files included in this build.
CSRC += $(TARGET_NANOPB_SRC_PATH)/pb_common.c
CSRC += $(TARGET_NANOPB_SRC_PATH)/pb_encode.c
CSRC += $(TARGET_NANOPB_SRC_PATH)/pb_decode.c
