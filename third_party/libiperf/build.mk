TARGET_LIBIPERF_SRC_PATH = $(LIBIPERF_MODULE_PATH)/libiperf/src

LIBIPERF_VERSION := $(shell $(AWK) -F'[][]' '/^AC_INIT/{print $$4}' $(LIBIPERF_MODULE_PATH)/libiperf/configure.ac)
ifeq ($(LIBIPERF_VERSION),)
$(error failed to extract iperf version from configure.ac)
endif

CFLAGS += -DIPERF_VERSION=\"$(LIBIPERF_VERSION)\"
CFLAGS += -DUNAME_MACHINE=\"$(ARM_CPU)\"
CFLAGS += -Wno-sign-compare

CSRC += $(TARGET_LIBIPERF_SRC_PATH)/cjson.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_api.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_error.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_client_api.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_locale.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_server_api.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_tcp.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_udp.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_sctp.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_util.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_time.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/iperf_pthread.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/dscp.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/net.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/tcp_info.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/timer.c
CSRC += $(TARGET_LIBIPERF_SRC_PATH)/units.c

CSRC += $(call target_files,$(LIBIPERF_MODULE_PATH)/particle/,*.c)
CPPSRC += $(call target_files,$(LIBIPERF_MODULE_PATH)/particle/,*.cpp)
