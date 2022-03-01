INCLUDE_DIRS += $(SOURCE_PATH)/$(USRSRC)  # add user sources to include path
# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(USRSRC_SLASH),*.cpp)
CSRC += $(call target_files,$(USRSRC_SLASH),*.c)

APPSOURCES=$(call target_files,$(USRSRC_SLASH),*.cpp)
APPSOURCES+=$(call target_files,$(USRSRC_SLASH),*.c)
ifeq ($(strip $(APPSOURCES)),)
$(error "No sources found in $(SOURCE_PATH)/$(USRSRC)")
endif

ifeq ("${USE_SPI}","SPI")
USE_SPI_VAL=0
else ifeq ("${USE_SPI}","SPI1")
USE_SPI_VAL=1
else ifeq ("${USE_SPI}","SPI2")
USE_SPI_VAL=2
else
USE_SPI_VAL=255
endif

ifeq ("${USE_CS}","A2")
USE_CS_VAL=0
else ifeq ("${USE_CS}","D5")
USE_CS_VAL=1
else ifeq ("${USE_CS}","D8")
USE_CS_VAL=2
else ifeq ("${USE_CS}","D14")
USE_CS_VAL=3
else ifeq ("${USE_CS}","C0")
USE_CS_VAL=4
else
USE_CS_VAL=255
endif

CFLAGS += -DUSE_SPI=${USE_SPI_VAL} -DUSE_CS=${USE_CS_VAL}
CXXFLAGS += -DUSE_SPI=${USE_SPI_VAL} -DUSE_CS=${USE_CS_VAL}