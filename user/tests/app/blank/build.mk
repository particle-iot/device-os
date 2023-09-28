USER_APP_PATH = $(SOURCE_PATH)/$(USRSRC)
INCLUDE_DIRS += $(USER_APP_PATH)  # add user sources to include path
# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(USER_APP_PATH),*.cpp)
CSRC += $(call target_files,$(USER_APP_PATH),*.c)

APPSOURCES=$(call target_files,$(USER_APP_PATH),*.cpp)
APPSOURCES+=$(call target_files,$(USER_APP_PATH),*.c)
ifeq ($(strip $(APPSOURCES)),)
$(error "No sources found in $(USER_APP_PATH)")
endif

ifeq ("$(LOG_SERIAL)","y")
CFLAGS += -DLOG_SERIAL
CXXFLAGS += -DLOG_SERIAL
endif

ifeq ("$(LOG_SERIAL1)","y")
CFLAGS += -DLOG_SERIAL1
CXXFLAGS += -DLOG_SERIAL1
endif

MAKE_DEPENDENCIES += third_party/coremark
