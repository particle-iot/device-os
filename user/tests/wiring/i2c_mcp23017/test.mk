INCLUDE_DIRS += $(SOURCE_PATH)/$(USRSRC)  # add user sources to include path
# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(USRSRC_SLASH),*.cpp)
CSRC += $(call target_files,$(USRSRC_SLASH),*.c)

APPSOURCES=$(call target_files,$(USRSRC_SLASH),*.cpp)
APPSOURCES+=$(call target_files,$(USRSRC_SLASH),*.c)
ifeq ($(strip $(APPSOURCES)),)
$(error "No sources found in $(SOURCE_PATH)/$(USRSRC)")
endif

ifeq ("${USE_WIRE}",)
USE_WIRE=Wire
else ifeq ("${USE_WIRE}","Wire1")
USE_WIRE=Wire1
else ifeq ("${USE_WIRE}","Wire3")
USE_WIRE=Wire3
else
USE_WIRE=Wire
endif

CFLAGS += -DUSE_WIRE=${USE_WIRE}
CXXFLAGS += -DUSE_WIRE=${USE_WIRE}
