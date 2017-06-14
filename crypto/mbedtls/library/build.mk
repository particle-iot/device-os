MBEDTLS_LIB=$(CRYPTO_MODULE_PATH)/mbedtls/library

# C source files included in this build.
CSRC += $(call target_files,$(CRYPTO_MODULE_PATH)/mbedtls/library/,*.c)
