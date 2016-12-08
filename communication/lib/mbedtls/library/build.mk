
MBEDTLS_LIB=$(COMMUNICATION_MODULE_PATH)/lib/mbedtls/library

# C source files included in this build.
CSRC += $(call target_files,$(COMMUNICATION_MODULE_PATH)/lib/mbedtls/library/,*.c)

# Enable GNU extensions for libc
CFLAGS += -D_GNU_SOURCE
