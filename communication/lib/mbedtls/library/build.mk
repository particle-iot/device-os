# Do not build for Photon/P1
ifneq ($(PLATFORM_ID),6)
ifneq ($(PLATFORM_ID),8)

MBEDTLS_LIB=$(COMMUNICATION_MODULE_PATH)/lib/mbedtls/library

# C source files included in this build.
CSRC += $(call target_files,$(COMMUNICATION_MODULE_PATH)/lib/mbedtls/library/,*.c)

endif
endif
