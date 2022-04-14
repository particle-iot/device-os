INCLUDE_DIRS += $(SOURCE_PATH)/$(USRSRC)  # add user sources to include path
# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(USRSRC_SLASH),*.cpp)
CSRC += $(call target_files,$(USRSRC_SLASH),*.c)

APPSOURCES=$(call target_files,$(USRSRC_SLASH),*.cpp)
APPSOURCES+=$(call target_files,$(USRSRC_SLASH),*.c)
ifeq ($(strip $(APPSOURCES)),)
$(error "No sources found in $(SOURCE_PATH)/$(USRSRC)")
endif

ifeq ("${USE_THREADING}","y")
USE_THREADING_VALUE=1
else
USE_THREADING_VALUE=0
endif

CFLAGS += -DUSE_THREADING=${USE_THREADING_VALUE}
