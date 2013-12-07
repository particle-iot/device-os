
# Define the compiler/tools prefix
GCC_PREFIX = arm-none-eabi-

# Define tools
CC = $(GCC_PREFIX)gcc
AR = $(GCC_PREFIX)ar
OBJCOPY = $(GCC_PREFIX)objcopy

RM = rm -f
MKDIR = mkdir -p

# Recursive wildcard function
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# Define the build path, this is where all of the dependancies and
# object files will be placed.
# Note: Currently set to <project>/build/obj directory and set relative to
# the dir which makefile is invoked. If the makefile is moved to the project
# root, BUILD_PATH = build can be used to store the build products in 
# the build directory.
BUILD_PATH = obj

# Path to the root of source files, in this case root of the project to
# inclue ../src and ../lib dirs.
# Note: Consider relocating source files in lib to src, or build a
#       seperate library.
SRC_PATH = ..

# Target this makefile is building.
TARGET = libcore-communication-lib.a

# Find all build.mk makefiles in each source directory in the src tree.
SRC_MAKEFILES := $(call rwildcard,$(SRC_PATH)/,build.mk)

# Include all build.mk defines source files.
include $(SRC_MAKEFILES)

# Additional include directories, applied to objects built for this target.
INCLUDE_DIRS += 

# Compiler flags
CFLAGS =  -g3 -gdwarf-2 -Os -mcpu=cortex-m3 -mthumb 
CFLAGS += $(patsubst %,-I$(SRC_PATH)/%,$(INCLUDE_DIRS)) -I.
CFLAGS += -ffunction-sections -fmessage-length=0 -Wall

# Generate dependancy files automatically.
CFLAGS += -MD -MP -MF $@.d

# Target specific defines
CFLAGS += -DSPARK_PRODUCT_ID=$(SPARK_PRODUCT_ID)
CFLAGS += -DPRODUCT_FIRMWARE_VERSION=$(PRODUCT_FIRMWARE_VERSION)

# C++ specific flags
CPPFLAGS = -fno-exceptions -fno-rtti

SPARK_PRODUCT_ID ?= 65535
PRODUCT_FIRMWARE_VERSION ?= 65535

# Collect all object and dep files
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o))
ALLOBJ += $(addprefix $(BUILD_PATH)/, $(ASRC:.S=.o))

ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CSRC:.c=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(CPPSRC:.cpp=.o.d))
ALLDEPS += $(addprefix $(BUILD_PATH)/, $(ASRC:.S=.o.d))

# All Target
all: check_external_deps $(TARGET)

# Check for external dependancies, none in this case.
check_external_deps:

# Tool invocations
$(TARGET) : $(ALLOBJ)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM GCC Archiver'
	$(AR) -r $@ $^
	@echo ' '

# C compiler to build .o from .c in $(BUILD_DIR)
$(BUILD_PATH)/%.o : $(SRC_PATH)/%.c
	@echo Building file: $<
	@echo Invoking: ARM GCC C Compiler
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo

# CPP compiler to build .o from .cpp in $(BUILD_DIR)
# Note: Calls standard $(CC) - gcc will invoke g++ as appropriate
$(BUILD_PATH)/%.o : $(SRC_PATH)/%.cpp
	@echo Building file: $<
	@echo Invoking: ARM GCC CPP Compiler
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
	@echo

# Other Targets
clean:
	$(RM) $(ALLOBJ) $(ALLDEPS) $(TARGET)
	@echo ' '

.PHONY: all clean
.SECONDARY:

# Include auto generated dependancy files
-include $(ALLDEPS)

