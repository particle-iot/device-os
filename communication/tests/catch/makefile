## -*- Makefile -*-

CCC = gcc
CXX = g++
LD = g++
CFLAGS = -g
CCFLAGS = $(CFLAGS)
CXXFLAGS = $(CFLAGS) $(CPPFLAGS)
RM = rm -f
RMDIR = rm -f -r
MKDIR = mkdir -p

# root of core-firmware project relative to this folder
PROJECT_ROOT=../../..
SRC_ROOT=../..
COMMON_BUILD=$(PROJECT_ROOT)/build

# location of this folder relative to the root
SRC_PATH=communication/tests/catch
COMMUNICATION=communication
DYNALIB=dynalib
HAL=hal
SERVICES=services
WIRING=wiring
CRYPTO=crypto
MBEDTLS=third_party/mbedtls/mbedtls

TARGETDIR=target
TARGET=runner

include $(COMMON_BUILD)/version.mk
include $(COMMON_BUILD)/macros.mk
include $(COMMON_BUILD)/lang-std.mk

CCFLAGS += $(CONLYFLAGS)

COMMUNICATION_MODULE_PATH=$(SRC_ROOT)
BUILD_PATH=$(TARGETDIR)

# enumerates files in the filesystem and returns their path relative to the project root
# $1 the directory relative to the project root
# $2 the pattern to match, e.g. *.cpp
target_files = $(patsubst $(SRC_ROOT)/%,%,$(call rwildcard,$(SRC_ROOT)/$1,$2))

# sources are relative to the communications folder
CPPSRC += $(call target_files,tests/catch,*.cpp)

CPPSRC += src/coap.cpp src/messages.cpp src/events.cpp src/protocol.cpp
CPPSRC += src/chunked_transfer.cpp src/coap_channel.cpp src/eckeygen.cpp
CPPSRC += src/dtls_message_channel.cpp src/dtls_protocol.cpp src/publisher.cpp
CPPSRC += src/communication_diagnostic.cpp

CRYPTO_DIR = $(abspath $(PROJECT_ROOT)/$(CRYPTO))
CRYPTO_CPPSRC += $(wildcard $(CRYPTO_DIR)/src/*.cpp)
MBEDTLS_DIR = $(abspath $(PROJECT_ROOT)/$(MBEDTLS))
MBEDTLS_CSRC = $(wildcard $(MBEDTLS_DIR)/library/*.c)

include $(call rwildcard,$(PROJECT_ROOT)/$(COMMUNICATION)/,include.mk)
include $(call rwildcard,$(PROJECT_ROOT)/$(COMMUNICATION)/lib/,build.mk)

#INCLUDE_DIRS := $(addprefix $(SRC_ROOT)/,$(INCLUDE_DIRS))

# these include dirs relative to project root
INCLUDE_DIRS += $(PROJECT_ROOT)/$(SERVICES)/inc
INCLUDE_DIRS += $(PROJECT_ROOT)/$(COMMUNICATION)/src
INCLUDE_DIRS += $(PROJECT_ROOT)/$(HAL)/shared $(PROJECT_ROOT)/$(HAL)/inc
INCLUDE_DIRS += $(PROJECT_ROOT)/$(DYNALIB)/inc
INCLUDE_DIRS += $(PROJECT_ROOT)/$(WIRING)/inc
INCLUDE_DIRS += $(PROJECT_ROOT)/$(CRYPTO)/inc
INCLUDE_DIRS += $(PROJECT_ROOT)/$(MBEDTLS)/include

CFLAGS += $(patsubst %,-I%,$(INCLUDE_DIRS)) -I.
CFLAGS += -ffunction-sections -fdata-sections -Wall
CFLAGS += -DPLATFORM_ID=3

# Flag compiler error for [-Wdeprecated-declarations]
CFLAGS += -Werror=deprecated-declarations

# Generate dependency files automatically.
CFLAGS += -MD -MP -MF $@.d
CFLAGS += -DSPARK=1
CFLAGS += -DDEBUG_BUILD
CFLGAS += fprofile-arcs -ftest-coverage

CPPFLAGS += -DCATCH_CONFIG_SFINAE
CFLAGS += -DINTERRUPTS_HAL_EXCLUDE_PLATFORM_HEADERS
CFLAGS += -DMBEDTLS_SSL_RAW_PUBLIC_KEY_SUPPORT

# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o))
ALLOBJ += $(subst $(MBEDTLS_DIR),$(BUILD_PATH)/mbedtls,$(MBEDTLS_CSRC:.c=.o))
ALLOBJ += $(subst $(CRYPTO_DIR),$(BUILD_PATH)/crypto,$(CRYPTO_CPPSRC:.cpp=.o))

ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o.d))
ALLDEPS += $(subst $(MBEDTLS_DIR),$(BUILD_PATH)/mbedtls,$(MBEDTLS_CSRC:.c=.o.d))
ALLDEPS += $(subst $(CRYPTO_DIR),$(BUILD_PATH)/crypto,$(CRYPTO_CPPSRC:.cpp=.o.d))


all: runner

runner: $(TARGETDIR)/$(TARGET)

$(TARGETDIR)/$(TARGET) : $(BUILD_PATH) $(ALLOBJ)
	@echo Building target: $@
	@echo Invoking: GCC C++ Linker
	$(MKDIR) $(dir $@)
	$(LD) -Darse5 $(CFLAGS) $(ALLOBJ) $(LIBS) --output $@ $(LDFLAGS)
	@echo

$(BUILD_PATH):
	$(MKDIR) $(BUILD_PATH)

# Tool invocations

$(BUILD_PATH)/mbedtls/library/%.o : $(MBEDTLS_DIR)/library/%.c
	@echo Building file: $<
	@echo Invoking: GCC C Compiler
	$(MKDIR) $(dir $@)
	$(CCC) $(CCFLAGS) -c -o $@ $<
	@echo

$(BUILD_PATH)/crypto/%.o : $(CRYPTO_DIR)/%.cpp
	@echo Building file: $<
	@echo Invoking: GCC CPP Compiler
	$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	@echo


# C compiler to build .o from .c in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(SRC_ROOT)/%.c
	@echo Building file: $<
	@echo Invoking: GCC C Compiler
	$(MKDIR) $(dir $@)
	$(CCC) $(CCFLAGS) -c -o $@ $<
	@echo

# CPP compiler to build .o from .cpp in $(BUILD_DIR)
# Note: Calls standard $(CC) - gcc will invoke g++ as appropriate
$(BUILD_PATH)/%.o : $(SRC_ROOT)/%.cpp
	@echo Building file: $<
	@echo Invoking: GCC CPP Compiler
	$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	@echo

# Other Targets
clean:
	$(RM) $(ALLOBJ) $(ALLDEPS) $(TARGETDIR)/$(TARGET)
	$(RMDIR) $(TARGETDIR)
	@echo

test: runner
	$(TARGETDIR)/$(TARGET)

.PHONY: all clean runner
.SECONDARY:

# Include auto generated dependency files
-include $(ALLDEPS)



