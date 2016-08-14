INCLUDE_DIRS += $(SOURCE_PATH)/$(USRSRC)  # add user sources to include path
# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(USRSRC_SLASH),*.cpp)
CSRC += $(call target_files,$(USRSRC_SLASH),*.c)

APPSOURCES=$(call target_files,$(USRSRC_SLASH),*.cpp)
APPSOURCES+=$(call target_files,$(USRSRC_SLASH),*.c)
ifeq ($(strip $(APPSOURCES)),)
$(error "No sources found in $(SOURCE_PATH)/$(USRSRC)")
endif

ifeq ("${USE_SPI}",)
USE_SPI=SPI
USE_CS=A2
else ifeq ("${USE_SPI}","SPI1")
USE_SPI=SPI1
USE_CS=D5
else ifeq ("${USE_SPI}","SPI2")
USE_SPI=SPI2
USE_CS=C0
else
USE_SPI=SPI
USE_CS=A2
endif

CFLAGS += -DUSE_SPI=${USE_SPI} -DUSE_CS=${USE_CS}
CXXFLAGS += -DUSE_SPI=${USE_SPI} -DUSE_CS=${USE_CS}

