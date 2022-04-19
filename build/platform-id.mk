ifeq ($(included_productid_mk),)
included_productid_mk := 1

# defines
# PLATFORM_NAME - a unique name for the platform, can be used to organize sources
#                 by platform
# PLATFORM_GEN  - a number that identifies the generation of device
# PLATFORM_MCU  - an identifier for the MCU family
# PLATFORM_NET  - the network subsystem
# MCU_DEVICE    - the specific device being targeted for platform builds
# ARCH          - architecture (ARM/GCC)
# PRODUCT_DESC  - text description of the product ID
# PLATFORM_DYNALIB_MODULES - if the device supports a modular build, the name
#                          - of the subdirectory containing

# Default USB Device Vendor ID for Particle Products
USBD_VID_SPARK=0x2B04
# Default USB Device Product ID for DFU Class
USBD_PID_DFU=0x607F
# Default USB Device Product ID for CDC Class
USBD_PID_CDC=0x607D

ifneq (,$(PLATFORM))
ifeq ("$(PLATFORM)","gcc")
PLATFORM_ID = 3
endif

ifeq ("$(PLATFORM)","argon")
PLATFORM_ID=12
endif

ifeq ("$(PLATFORM)","boron")
PLATFORM_ID=13
endif

ifeq ("$(PLATFORM)","asom")
PLATFORM_ID=22
endif

ifeq ("$(PLATFORM)","bsom")
PLATFORM_ID=23
endif

ifeq ("$(PLATFORM)","b5som")
PLATFORM_ID=25
endif

ifeq ("$(PLATFORM)","tracker")
PLATFORM_ID=26
endif

ifeq ("$(PLATFORM)","newhal")
PLATFORM_ID=60000
endif

ifeq (,$(PLATFORM_ID))
$(error "Unknown platform: $(PLATFORM))
endif
endif

ifndef PLATFORM_ID
$(error PLATFORM or PLATFORM_ID not set. Please specify the platform to build.")
endif

# Determine which is the target device

ARCH=arm

ifeq ("$(PLATFORM_ID)","3")
PLATFORM=gcc
PLATFORM_NAME=gcc
PLATFORM_GEN=0
PLATFORM_MCU=gcc
PLATFORM_NET=gcc
ARCH=gcc
PRODUCT_DESC=GCC xcompile
# explicitly exclude platform headers
SPARK_NO_PLATFORM=1
DEFAULT_PRODUCT_ID=3
endif

ifeq ("$(PLATFORM_ID)","12")
PLATFORM=argon
PLATFORM_NAME=$(PLATFORM)
PLATFORM_GEN=3
PLATFORM_MCU=nRF52840
PLATFORM_NET=ESP32
PLATFORM_WIZNET=W5500
MCU_DEVICE=nRF52840
PRODUCT_DESC=Production Argon
USBD_VID_SPARK=0x2B04
USBD_PID_DFU=0xD00C
USBD_PID_CDC=0xC00C
DEFAULT_PRODUCT_ID=$(PLATFORM_ID)
PLATFORM_DYNALIB_MODULES=$(PLATFORM)
ARM_CPU=cortex-m4
PLATFORM_THREADING=1
endif

ifeq ("$(PLATFORM_ID)","13")
PLATFORM=boron
PLATFORM_NAME=$(PLATFORM)
PLATFORM_GEN=3
PLATFORM_MCU=nRF52840
PLATFORM_NET=UBLOXSARA
PLATFORM_WIZNET=W5500
MCU_DEVICE=nRF52840
PRODUCT_DESC=Production Boron
USBD_VID_SPARK=0x2B04
USBD_PID_DFU=0xD00D
USBD_PID_CDC=0xC00D
DEFAULT_PRODUCT_ID=$(PLATFORM_ID)
PLATFORM_DYNALIB_MODULES=$(PLATFORM)
ARM_CPU=cortex-m4
PLATFORM_THREADING=1
endif

ifeq ("$(PLATFORM_ID)","22")
PLATFORM=asom
PLATFORM_NAME=argon
PLATFORM_GEN=3
PLATFORM_MCU=nRF52840
PLATFORM_NET=ESP32
PLATFORM_WIZNET=W5500
MCU_DEVICE=nRF52840
PRODUCT_DESC=Production A SoM
USBD_VID_SPARK=0x2B04
USBD_PID_DFU=0xD016
USBD_PID_CDC=0xC016
DEFAULT_PRODUCT_ID=$(PLATFORM_ID)
PLATFORM_DYNALIB_MODULES=argon
ARM_CPU=cortex-m4
PLATFORM_THREADING=1
endif

ifeq ("$(PLATFORM_ID)","23")
PLATFORM=bsom
PLATFORM_NAME=boron
PLATFORM_GEN=3
PLATFORM_MCU=nRF52840
PLATFORM_NET=UBLOXSARA
PLATFORM_WIZNET=W5500
MCU_DEVICE=nRF52840
PRODUCT_DESC=Production B SoM
USBD_VID_SPARK=0x2B04
USBD_PID_DFU=0xD017
USBD_PID_CDC=0xC017
DEFAULT_PRODUCT_ID=$(PLATFORM_ID)
PLATFORM_DYNALIB_MODULES=boron
ARM_CPU=cortex-m4
PLATFORM_THREADING=1
endif

ifeq ("$(PLATFORM_ID)","25")
PLATFORM=b5som
PLATFORM_NAME=b5som
PLATFORM_GEN=3
PLATFORM_MCU=nRF52840
PLATFORM_NET=QUECTEL
PLATFORM_WIZNET=W5500
MCU_DEVICE=nRF52840
PRODUCT_DESC=Production B5 SoM
USBD_VID_SPARK=0x2B04
USBD_PID_DFU=0xD019
USBD_PID_CDC=0xC019
DEFAULT_PRODUCT_ID=$(PLATFORM_ID)
PLATFORM_DYNALIB_MODULES=$(PLATFORM_NAME)
ARM_CPU=cortex-m4
PLATFORM_THREADING=1
endif

ifeq ("$(PLATFORM_ID)","26")
PLATFORM=tracker
PLATFORM_NAME=tracker
PLATFORM_GEN=3
PLATFORM_MCU=nRF52840
PLATFORM_NET=QUECTEL
PLATFORM_WIZNET=W5500
MCU_DEVICE=nRF52840
PRODUCT_DESC=Production Tracker
USBD_VID_SPARK=0x2B04
USBD_PID_DFU=0xD01A
USBD_PID_CDC=0xC01A
DEFAULT_PRODUCT_ID=$(PLATFORM_ID)
PLATFORM_DYNALIB_MODULES=$(PLATFORM_NAME)
ARM_CPU=cortex-m4
PLATFORM_THREADING=1
PLATFORM_OPENTHREAD=nrf52840
endif

ifeq ("$(PLATFORM_ID)","60000")
PLATFORM=newhal
# needed for conditional compilation of some MCU specific files
MCU_DEVICE=newhalcpu
# used to define the sources in hal/src/new-hal
PLATFORM_NAME=newhal
PLATFORM_GEN=0
# define MCU-specific platform defines under platform/MCU/new-hal
PLATFORM_MCU=newhal-mcu
PLATFORM_NET=not-defined
PRODUCT_DESC=Test platform for producing a new HAL implementation
USBD_VID_SPARK=0x2B04
USBD_PID_DFU=0x607F
USBD_PID_CDC=0x607D
DEFAULT_PRODUCT_ID=60000
ARM_CPU=cortex-m3
endif

ifeq ("$(MCU_DEVICE)","nRF52840")
    PLATFORM_THREADING=1
    PLATFORM_DFU ?= 0x30000
endif


ifeq ("$(PLATFORM_MCU)","")
$(error PLATFORM_MCU not defined. Check platform id $(PLATFORM_ID))
endif

ifeq ("$(PLATFORM_NET)","")
$(error PLATFORM_NET not defined. Check platform id $(PLATFORM_ID))
endif

# lower case version of the MCU_DEVICE string for use in filenames
MCU_DEVICE_LC  = $(shell echo $(MCU_DEVICE) | tr A-Z a-z)

ifdef MCU_DEVICE
# needed for conditional compilation of syshealth_hal.h
CFLAGS += -DMCU_DEVICE
# needed for conditional compilation of some MCU specific files
CFLAGS += -D$(MCU_DEVICE)

ifeq ("$(MCU_DEVICE)","nRF52840")
CFLAGS += -DNRF52840_XXAA
endif

endif

ifneq ("$(SPARK_NO_PLATFORM)","")
CFLAGS += -DSPARK_NO_PLATFORM
endif

ifeq ("$(PRODUCT_ID)","")
# ifeq (,$(submake))
# $(info PRODUCT_ID not defined - using default: $(DEFAULT_PRODUCT_ID))
# endif
PRODUCT_ID = $(DEFAULT_PRODUCT_ID)
endif

PLATFORM_THREADING ?= 0
CFLAGS += -DPLATFORM_THREADING=$(PLATFORM_THREADING)

# Using PLATFORM here, as for platforms based on another platform
# PLATFORM_NAME is the name of the base platform
CFLAGS += -DPLATFORM_ID=$(PLATFORM_ID) -DPLATFORM_NAME=$(PLATFORM) -DPLATFORM_GEN=$(PLATFORM_GEN)

CFLAGS += -DUSBD_VID_SPARK=$(USBD_VID_SPARK)
CFLAGS += -DUSBD_PID_DFU=$(USBD_PID_DFU)
CFLAGS += -DUSBD_PID_CDC=$(USBD_PID_CDC)

MODULE_FUNCTION_NONE            :=0
MODULE_FUNCTION_RESOURCE        :=1
MODULE_FUNCTION_BOOTLOADER      :=2
MODULE_FUNCTION_MONO_FIRMWARE   :=3
MODULE_FUNCTION_SYSTEM_PART     :=4
MODULE_FUNCTION_USER_PART       :=5
MODULE_FUNCTION_SETTINGS        :=6
MODULE_FUNCTION_NCP_FIRMWARE    :=7
MODULE_FUNCTION_RADIO_STACK     :=8

endif
