INCLUDE_DIRS += $(SOURCE_PATH)/$(USRSRC)  # add user sources to include path
# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(USRSRC_SLASH),*.cpp)
CSRC += $(call target_files,$(USRSRC_SLASH),*.c)

APPSOURCES=$(call target_files,$(USRSRC_SLASH),*.cpp)
APPSOURCES+=$(call target_files,$(USRSRC_SLASH),*.c)
ifeq ($(strip $(APPSOURCES)),)
$(error "No sources found in $(SOURCE_PATH)/$(USRSRC)")
endif

ifeq ("${SIZE_TEST}","y")
CFLAGS += -DSIZE_TEST
CXXFLAGS += -DSIZE_TEST

ifeq ("${SYSTEM_TICK_T_LITERALS}","y")
CFLAGS += -DSIZE_TEST_SYSTEM_TICK_T_LITERALS
CXXFLAGS += -DSIZE_TEST_SYSTEM_TICK_T_LITERALS
endif

endif

