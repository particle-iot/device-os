TARGET_MBEDTLS_SRC_PATH = $(MBEDTLS_MODULE_PATH)/mbedtls/library

# C source files included in this build.
CSRC += $(call target_files,$(TARGET_MBEDTLS_SRC_PATH)/,*.c)
