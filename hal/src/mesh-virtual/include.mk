# Define the prefix to this directory.
# Note: The name must be unique within this build and should be
#       based on the root of the project
HAL_SRC_MESH_VIRTUAL_PATH = $(TARGET_HAL_PATH)/src/mesh-virtual

# if we are being compiled with platform as a dependency, then also include
# implementation headers.
ifneq (,$(findstring platform,$(DEPENDENCIES)))
INCLUDE_DIRS += $(HAL_SRC_MESH_VIRTUAL_PATH)
INCLUDE_DIRS += $(HAL_SRC_MESH_VIRTUAL_PATH)/lwip
INCLUDE_DIRS += $(HAL_SRC_MESH_VIRTUAL_PATH)/freertos
INCLUDE_DIRS += $(HAL_MODULE_PATH)/network/lwip
endif

HAL_DEPS = third_party/lwip third_party/freertos
HAL_DEPS_INCLUDE_SCRIPTS =$(foreach module,$(HAL_DEPS),$(PROJECT_ROOT)/$(module)/import.mk)
include $(HAL_DEPS_INCLUDE_SCRIPTS)

HAL_LIB_DEP += $(FREERTOS_LIB_DEP) $(LWIP_LIB_DEP)
LIBS += $(notdir $(HAL_DEPS))

ifneq (,$(findstring hal,$(MAKE_DEPENDENCIES)))

LDFLAGS += -lc


# additional libraries required by gcc build
ifdef SYSTEMROOT
LIBS += boost_system-mgw48-mt-1_57 ws2_32 wsock32
else
LIBS += boost_system
endif
LIBS += boost_program_options boost_random boost_thread

LIB_DIRS += $(BOOST_ROOT)/stage/lib

# gcc HAL is different for test driver and test subject
ifeq "$(SPARK_TEST_DRIVER)" "1"
HAL_TEST_FLAVOR+=-driver
endif



endif



