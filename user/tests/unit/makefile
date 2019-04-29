## -*- Makefile -*-

CCC = gcc
CXX = g++
LD = g++
CFLAGS = -g
CCFLAGS = $(CFLAGS)
CXXFLAGS = $(CFLAGS)
RM = rm -f
RMDIR = rm -f -r
MKDIR = mkdir -p

# root of core-firmware project relative to this folder
SRC_ROOT=../../../

# location of this folder relative to the root
SRC_PATH=user/tests/unit/
COMMUNICATION=communication/
WIRING=wiring/
SYSTEM=system/
HAL=hal/
PLATFORM=platform/
WIRING_SRC=$(WIRING)src/
WIRING_GLOBALS=wiring_globals/
WIRING_GLOBALS_SRC=${WIRING_GLOBALS}src

TARGETDIR=obj/
TARGET=runner

include $(SRC_ROOT)/build/version.mk
include $(SRC_ROOT)/build/lang-std.mk
CCFLAGS += $(CONLYFLAGS)

BUILD_PATH=$(TARGETDIR)core-firmware/
# Define the target directories. Nest 2 levels deep since we also include
# sources from libraries via ../core-common-lib

# Recursive wildcard function
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# enumerates files in the filesystem and returns their path relative to the project root
# $1 the directory relative to the project root
# $2 the pattern to match, e.g. *.cpp
target_files = $(patsubst $(SRC_ROOT)%,%,$(call rwildcard,$(SRC_ROOT)$1,$2))

CPPSRC += $(call target_files,$(SRC_PATH),*.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_random.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_string.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_ipaddress.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_print.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_logging.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_json.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_async.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_fuel.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_power.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_character.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_led.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_diagnostics.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_cellular_printable.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_i2c.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_wifi.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),spark_wiring_network.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),string_convert.cpp)
CPPSRC += $(call target_files,$(WIRING_SRC),string_convert.cpp)
CPPSRC += $(call target_files,$(WIRING_GLOBALS_SRC),wiring_globals_i2c.cpp)
CPPSRC += $(call target_files,$(SYSTEM)src/,system_utilities.cpp)
CPPSRC += $(call target_files,$(SYSTEM)src/,system_mode.cpp)
CPPSRC += $(call target_files,$(SYSTEM)src/,system_string_interpolate.cpp)
CPPSRC += $(call target_files,$(SYSTEM)src/,system_led_signal.cpp)
CPPSRC += $(call target_files,$(SYSTEM)src/,active_object.cpp)
CPPSRC += $(call target_files,$(SYSTEM)src/,usb_control_request_channel.cpp)
CPPSRC += $(call target_files,$(SYSTEM)src/,control_request_handler.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,filesystem.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,device_config.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,core_hal.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,timer_hal.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,rgbled_hal.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,wlan_hal.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,net_hal.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,delay_hal.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,usb_hal.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,deviceid_hal.cpp)
CPPSRC += $(call target_files,$(HAL)src/gcc,interrupts_hal.cpp)
CPPSRC += $(call target_files,$(HAL)src/electron,cellular_internal.cpp)
CPPSRC += $(call target_files,$(HAL)src/template,i2c_hal.cpp)

# Paths to dependent projects, referenced from root of this project
LIB_SERVICES = services/

CSRC += $(call target_files,$(SRC_PATH),*.c)
CSRC += $(call target_files,$(LIB_SERVICES)src,rgbled.c)
CSRC += $(call target_files,$(LIB_SERVICES)src,debug.c)
CSRC += $(call target_files,$(LIB_SERVICES)src,jsmn.c)
CSRC += $(call target_files,$(PLATFORM)MCU/STM32F2xx/SPARK_Firmware_Driver/src,system_flags_impl.c)
CPPSRC += $(call target_files,$(LIB_SERVICES)src,logging.cpp)
CPPSRC += $(call target_files,$(LIB_SERVICES)src,system_error.cpp)
CPPSRC += $(call target_files,$(LIB_SERVICES)src,led_service.cpp)
CPPSRC += $(call target_files,$(LIB_SERVICES)src,completion_handler.cpp)
CPPSRC += $(call target_files,$(LIB_SERVICES)src,diagnostics.cpp)


# Additional include directories, applied to objects built for this target.
# todo - delegate this to a include.mk file in each repo so include dirs are better
# encapsulated by their owning repo
INCLUDE_DIRS += $(SRC_PATH)stubs
INCLUDE_DIRS += $(LIB_SERVICES)inc
INCLUDE_DIRS += $(WIRING)inc
INCLUDE_DIRS += $(SYSTEM)inc
INCLUDE_DIRS += $(SYSTEM)src
INCLUDE_DIRS += $(HAL)shared
INCLUDE_DIRS += $(HAL)inc
INCLUDE_DIRS += $(HAL)src/electron
INCLUDE_DIRS += $(HAL)src/gcc
INCLUDE_DIRS += $(COMMUNICATION)src
INCLUDE_DIRS += dynalib/inc
INCLUDE_DIRS += $(PLATFORM)shared/inc
INCLUDE_DIRS += $(PLATFORM)MCU/gcc/inc
INCLUDE_DIRS += $(PLATFORM)MCU/STM32F2xx/SPARK_Firmware_Driver/inc

# prefix $(SRC_ROOT)
ABS_INCLUDE_DIRS += $(patsubst %,$(SRC_ROOT)/%,$(INCLUDE_DIRS))

DEFINES += SPARK_NO_PLATFORM

ifeq ("$(BOOST_ROOT)","")
$(error BOOST_ROOT not defined)
else
$(info BOOST_ROOT "$(BOOST_ROOT)")
endif

DEFINES += UNIT_TEST BOOST_NO_AUTO_PTR USE_STDPERIPH_DRIVER
ABS_INCLUDE_DIRS += $(BOOST_ROOT)
LIB_DIRS += $(BOOST_ROOT)/stage/lib
LIBS += boost_program_options boost_regex boost_system boost_thread
export DYLD_LIBRARY_PATH=$(BOOST_ROOT)/stage/lib

CFLAGS += $(patsubst %,-I%,$(ABS_INCLUDE_DIRS)) -I.
CFLAGS += -ffunction-sections -fdata-sections -Wall

# Flag compiler error for [-Wdeprecated-declarations]
CFLAGS += -Werror=deprecated-declarations

# Generate dependency files automatically.
CFLAGS += -MD -MP -MF $@.d
CFLAGS += -DSPARK=1 -DPLATFORM_ID=3
CFLAGS += -DDEBUG_BUILD
CFLAGS += $(DEFINES:%=-D%)

CPPFLAGS += -DCATCH_CONFIG_SFINAE

LDFLAGS += $(LIB_DIRS:%=-L%) $(LIBS:%=-l%)

# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH), $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH), $(CPPSRC:.cpp=.o))

ALLDEPS += $(addprefix $(BUILD_PATH), $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH), $(CPPSRC:.cpp=.o.d))

all: runner run

run:
	$(TARGETDIR)$(TARGET)

runner: $(TARGETDIR)$(TARGET)

$(TARGETDIR)$(TARGET) : $(BUILD_PATH) $(ALLOBJ)
	@echo Building target: $@
	@echo Invoking: GCC C++ Linker
	$(MKDIR) $(dir $@)
	$(LD) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	@echo

$(BUILD_PATH):
	$(MKDIR) $(BUILD_PATH)

# Tool invocations

# C compiler to build .o from .c in $(BUILD_DIR)
$(BUILD_PATH)%.o : $(SRC_ROOT)%.c
	@echo Building file: $<
	@echo Invoking: GCC C Compiler
	$(MKDIR) $(dir $@)
	$(CCC) $(CCFLAGS) -c -o $@ $<
	@echo

# CPP compiler to build .o from .cpp in $(BUILD_DIR)
# Note: Calls standard $(CC) - gcc will invoke g++ as appropriate
$(BUILD_PATH)%.o : $(SRC_ROOT)%.cpp
	@echo Building file: $<
	@echo Invoking: GCC CPP Compiler
	$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
	@echo

# Other Targets
clean:
	$(RM) $(ALLOBJ) $(ALLDEPS) $(TARGETDIR)$(TARGET)
	$(RMDIR) $(TARGETDIR)
	@echo

.PHONY: all clean runner
.SECONDARY:

# Include auto generated dependency files
-include $(ALLDEPS)



