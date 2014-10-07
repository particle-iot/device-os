
MAKE_ARGS = --no-print-directory

# don't propagate command line definitions to submakes, specifically, TARGET,
# since this will override the local makefile's definition of that. 
MAKEOVERRIDES=

# by convention, all symbols referring to a directory end with a slash - this allows
# directories to resolve to "" when equal to the working directory
SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

# for now, assume current dir is build/
PROJECT_DIR=../

# Define the build path, this is where all of the dependancies and
# object files will be placed.
# Note: Currently set to <project>/build/obj directory and set relative to
# the dir which makefile is invoked. If the makefile is moved to the project
# root, BUILD_PATH = build can be used to store the build products in 
# the build directory.
BUILD_PATH = $(SELF_DIR)obj/

# directory containing this project
EXT_LIBS_DIR=../../


include $(SELF_DIR)tools.mk

# Recursive wildcard function
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# enumerates files in the filesystem and returns their path relative to the project root
# $1 the directory relative to the project root
# $2 the pattern to match, e.g. *.cpp
target_files = $(patsubst $(SRC_PATH)%,%,$(call rwildcard,$(SRC_PATH)$1,$2))



# URL to invoke cloud flashing
CLOUD_FLASH_URL = https://api.spark.io/v1/devices/$(SPARK_CORE_ID)\?access_token=$(SPARK_ACCESS_TOKEN)

# Path to the root of source files, in this case root of the project to
# include ../src and ../lib dirs.
# Note: Consider relocating source files in lib to src, or build a
#       separate library.
SRC_PATH = $(PROJECT_DIR)

# ensure defined
USRSRC += ""

# determine where user sources are, relative to project root
ifdef APP
USRSRC = applications/$(APP)/
CPPFLAGS += -std=gnu++11

ifndef TARGET
TARGET ?= $(notdir $(APP))
TARGETDIR ?= $(USRSRC)
endif
endif

ifdef TEST
USRSRC = tests/$(TEST)/
CPPFLAGS += -std=gnu++11
ifndef TARGET
TARGET ?= $(notdir $(TEST))
TARGETDIR ?= $(USRSRC)
endif
include $(PROJECT_DIR)tests/tests.mk
-include $(PROJECT_DIR)$(USRSRC)test.mk
endif

# user sources specified, so override application.cpp and pull in files from 
# the user source folder
ifneq ($(USRSRC),"")       
    NO_APPLICATION_CPP=1
    INCLUDE_DIRS += $(USRSRC)  # add user sources to include path
    # add C and CPP files 
    CPPSRC += $(call target_files,$(USRSRC),*.cpp)
    CSRC += $(call target_files,$(USRSRC),*.c)    
endif

TARGET ?= core-firmware
TARGETDIR ?= 


# Find all build.mk makefiles in each source directory in the src tree.
SRC_MAKEFILES := $(call rwildcard,$(SRC_PATH)src,build.mk)

# Include all build.mk defines source files.
include $(SRC_MAKEFILES)

# Paths to dependent projects, referenced from root of this project
LIB_CORE_COMMON_PATH = ../core-common-lib/
LIB_CORE_COMMUNICATION_PATH = ../core-communication-lib/
LIB_CORE_LIBRARIES_PATH = libraries/

# Include directories for optional "libraries" (eg. Serial2.h)
INCLUDE_DIRS += $(LIB_CORE_LIBRARIES_PATH)Serial2

# Additional include directories, applied to objects built for this target.
# todo - delegate this to a include.mk file in each repo so include dirs are better
# encapsulated by their owning repo
INCLUDE_DIRS += $(LIB_CORE_COMMON_PATH)CMSIS/Include
INCLUDE_DIRS += $(LIB_CORE_COMMON_PATH)CMSIS/Device/ST/STM32F10x/Include
INCLUDE_DIRS += $(LIB_CORE_COMMON_PATH)STM32F10x_StdPeriph_Driver/inc
INCLUDE_DIRS += $(LIB_CORE_COMMON_PATH)STM32_USB-FS-Device_Driver/inc
INCLUDE_DIRS += $(LIB_CORE_COMMON_PATH)CC3000_Host_Driver
INCLUDE_DIRS += $(LIB_CORE_COMMON_PATH)SPARK_Firmware_Driver/inc
INCLUDE_DIRS += $(LIB_CORE_COMMON_PATH)SPARK_Services/inc
INCLUDE_DIRS += $(LIB_CORE_COMMUNICATION_PATH)lib/tropicssl/include
INCLUDE_DIRS += $(LIB_CORE_COMMUNICATION_PATH)src

# Compiler flags
CFLAGS =  -g3 -gdwarf-2 -Os -mcpu=cortex-m3 -mthumb 
CFLAGS += $(patsubst %,-I$(SRC_PATH)%,$(INCLUDE_DIRS)) -I.
CFLAGS += -ffunction-sections -Wall -fmessage-length=0

# Flag compiler error for [-Wdeprecated-declarations]
CFLAGS += -Werror=deprecated-declarations

# Generate dependency files automatically.
CFLAGS += -MD -MP -MF $@.d

# Target specific defines
CFLAGS += -DUSE_STDPERIPH_DRIVER
CFLAGS += -DSTM32F10X_MD
CFLAGS += -DDFU_BUILD_ENABLE
CFLAGS += -DSPARK=1

ifeq ("$(USE_SWD_JTAG)","y") 
CFLAGS += -DUSE_SWD_JTAG
endif

ifeq ("$(DEBUG_BUILD)","y") 
CFLAGS += -DDEBUG_BUILD
else
CFLAGS += -DRELEASE_BUILD
endif

# C++ specific flags
CPPFLAGS += -fno-rtti -fno-exceptions

# Linker flags
LDFLAGS += -T$(PROJECT_DIR)linker/linker_stm32f10x_md_dfu.ld -nostartfiles -Xlinker --gc-sections 
LDFLAGS += -L$(PROJECT_DIR)$(LIB_CORE_COMMON_PATH)build -lcore-common-lib
LDFLAGS += -L$(PROJECT_DIR)$(LIB_CORE_COMMUNICATION_PATH)build -lcore-communication-lib
LDFLAGS += -Wl,-Map,$(TARGETDIR)$(TARGET).map
LDFLAGS += --specs=nano.specs -lc -lnosys
LDFLAGS += -u _printf_float

# Assembler flags
ASFLAGS =  -g3 -gdwarf-2 -mcpu=cortex-m3 -mthumb 
ASFLAGS += -x assembler-with-cpp -fmessage-length=0

# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH), $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH), $(CPPSRC:.cpp=.o))
ALLOBJ += $(addprefix $(BUILD_PATH), $(ASRC:.S=.o))

ALLDEPS += $(addprefix $(BUILD_PATH), $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH), $(CPPSRC:.cpp=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH), $(ASRC:.S=.o.d))


# All Target
all: elf bin hex size

elf: $(TARGETDIR)$(TARGET).elf
bin: $(TARGETDIR)$(TARGET).bin
hex: $(TARGETDIR)$(TARGET).hex

# Program the core using dfu-util. The core should have been placed
# in bootloader mode before invoking 'make program-dfu'
program-dfu: $(TARGETDIR)$(TARGET).dfu
	@echo Flashing using dfu:
	$(DFU) -d 1d50:607f -a 0 -s 0x08005000:leave -D $<

# Program the core using the cloud. SPARK_CORE_ID and SPARK_ACCESS_TOKEN must
# have been defined in the environment before invoking 'make program-cloud'
program-cloud: $(TARGETDIR)$(TARGET).bin
	@echo Flashing using cloud API, CORE_ID=$(SPARK_CORE_ID):
	$(CURL) -X PUT -F file=@$< -F file_type=binary $(CLOUD_FLASH_URL)

# Display size
size: $(TARGETDIR)$(TARGET).elf
	@echo Invoking: ARM GNU Print Size
	$(SIZE) --format=berkeley $<
	@echo

# Create a hex file from ELF file
%.hex : %.elf
	@echo Invoking: ARM GNU Create Flash Image
	$(OBJCOPY) -O ihex $< $@
	@echo

# Create a bin file from ELF file
%.bin : %.elf
	@echo Invoking: ARM GNU Create Flash Image
	$(OBJCOPY) -O binary $< $@
	@echo

# Create a DFU file from bin file
%.dfu: %.bin
	@cp $< $@
	$(DFUSUFFIX) -v 1d50 -p 607f -a $@

$(TARGETDIR)$(TARGET).elf : check_external_deps $(ALLOBJ)
	@echo Building target: $@
	@echo Invoking: ARM GCC C++ Linker
	$(MKDIR) $(dir $@)
	$(CPP) $(CFLAGS) $(ALLOBJ) --output $@ $(LDFLAGS)
	@echo

# Check for external dependancies are up to date
# Note: Since this makefile has no knowledge of depenancies for
#       the external libs, make must be called on the libs for
#       every build. Targets which depend directly on this recipie
#       will then always be rebuilt, ie. $(TARGET).elf
check_external_deps:
	@echo Building core-common-lib
	@$(MAKE) -C $(PROJECT_DIR)$(LIB_CORE_COMMON_PATH)build $(MAKE_ARGS)
	@echo
	@echo Building core-communication-lib
	@$(MAKE) -C $(PROJECT_DIR)$(LIB_CORE_COMMUNICATION_PATH)build $(MAKE_ARGS)
	@echo

# Tool invocations

# C compiler to build .o from .c in $(BUILD_DIR)
$(BUILD_PATH)%.o : $(SRC_PATH)%.c
	@echo Building file: $<
	@echo Invoking: ARM GCC C Compiler
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo

# Assember to build .o from .S in $(BUILD_DIR)
$(BUILD_PATH)%.o : $(SRC_PATH)%.S
	@echo Building file: $<
	@echo Invoking: ARM GCC Assember
	$(MKDIR) $(dir $@)
	$(CC) $(ASFLAGS) -c -o $@ $<
	@echo

# CPP compiler to build .o from .cpp in $(BUILD_DIR)
# Note: Calls standard $(CC) - gcc will invoke g++ as appropriate
$(BUILD_PATH)%.o : $(SRC_PATH)%.cpp
	@echo Building file: $<
	@echo Invoking: ARM GCC CPP Compiler
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
	@echo

# Other Targets
clean:
	
	$(RM) $(ALLOBJ) $(ALLDEPS) $(TARGETDIR)$(TARGET).elf $(TARGETDIR)$(TARGET).bin $(TARGETDIR)$(TARGET).hex $(TARGETDIR)$(TARGET).map
	$(RMDIR) $(BUILD_PATH)
	@echo
	@echo Clean core-common-lib
# Should clean invoke clean on the dependant libs as well? Sure..
	@$(MAKE) -C $(PROJECT_DIR)$(LIB_CORE_COMMON_PATH)build clean $(MAKE_ARGS)
	@echo
	@echo Clean core-communication-lib
	@$(MAKE) -C $(PROJECT_DIR)$(LIB_CORE_COMMUNICATION_PATH)build clean $(MAKE_ARGS)
	@echo

.PHONY: all clean check_external_deps elf bin hex size program-dfu program-cloud
.SECONDARY:

# Include auto generated dependency files
-include $(ALLDEPS)

