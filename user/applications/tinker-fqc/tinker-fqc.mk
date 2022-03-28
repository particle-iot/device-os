TINKER_PATH = $(SOURCE_PATH)/$(USRSRC)/../tinker
FQC_PATH = $(SOURCE_PATH)/$(USRSRC)

# NOTE: we are duplicating tinker.mk functionality here
# It's not easy to modify SOURCE_PATH that is used in tinker.mk

INCLUDE_DIRS += $(TINKER_PATH)  # add user sources to include path
INCLUDE_DIRS += $(FQC_PATH)  # add fqc headers to include path

# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(TINKER_PATH),*.cpp)
CSRC += $(call target_files,$(TINKER_PATH),*.c)

# NOTE: `target_files` macro includes .cpp from *all* tinker app directories. Not sure if this is intended
APPSOURCES=$(call target_files,$(TINKER_PATH),*.cpp)
APPSOURCES+=$(call target_files,$(TINKER_PATH),*.c)
ifeq ($(strip $(APPSOURCES)),)
$(error "No sources found in $(TINKER_PATH)")
endif

CFLAGS += -DLOG_SERIAL
CXXFLAGS += -DLOG_SERIAL
