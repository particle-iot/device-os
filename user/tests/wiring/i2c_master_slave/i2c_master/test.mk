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
USE_ACQUIRE_WIRE_BUFFER=acquireWireBuffer
else ifeq ("${USE_WIRE}","Wire1")
USE_WIRE=Wire1
USE_ACQUIRE_WIRE_BUFFER=acquireWire1Buffer
else ifeq ("${USE_WIRE}","Wire3")
USE_WIRE=Wire3
USE_ACQUIRE_WIRE_BUFFER=acquireWire3Buffer
else
USE_WIRE=Wire
USE_ACQUIRE_WIRE_BUFFER=acquireWireBuffer
endif

ifneq (,${BUFFER_SIZE})
CFLAGS += -DTEST_I2C_BUFFER_SIZE=${BUFFER_SIZE}
CXXFLAGS += -DTEST_I2C_BUFFER_SIZE=${BUFFER_SIZE}
endif

CFLAGS += -DUSE_WIRE=${USE_WIRE} -DUSE_ACQUIRE_WIRE_BUFFER=${USE_ACQUIRE_WIRE_BUFFER}
CXXFLAGS += -DUSE_WIRE=${USE_WIRE} -DUSE_ACQUIRE_WIRE_BUFFER=${USE_ACQUIRE_WIRE_BUFFER}
