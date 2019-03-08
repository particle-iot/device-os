TINKER_PATH = $(SOURCE_PATH)/$(USRSRC)
INCLUDE_DIRS += $(TINKER_PATH)  # add user sources to include path
# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(TINKER_PATH),*.cpp)
CSRC += $(call target_files,$(TINKER_PATH),*.c)

APPSOURCES=$(call target_files,$(TINKER_PATH),*.cpp)
APPSOURCES+=$(call target_files,$(TINKER_PATH),*.c)
ifeq ($(strip $(APPSOURCES)),)
$(error "No sources found in $(TINKER_PATH)")
endif

ifeq ("$(LOG_SERIAL)","y")
CFLAGS += -DLOG_SERIAL
CXXFLAGS += -DLOG_SERIAL
$(error "test")
endif

ifeq ("$(LOG_SERIAL1)","y")
CFLAGS += -DLOG_SERIAL1
CXXFLAGS += -DLOG_SERIAL1
endif

