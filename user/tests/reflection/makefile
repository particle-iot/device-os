
# default product IDs for the driver and the SUT executable
SPARK_TEST_DRIVER_PRODUCT_ID ?= $(SPARK_PRODUCT_ID)
SPARK_TEST_SUBJECT_PRODUCT_ID ?= $(SPARK_PRODUCT_ID)

ifndef SPARK_PRODUCT_ID
SPARK_PRODUCT_ID=0
endif

PROJECT_ROOT=../../..
BUILD_PATH=$(PROJECT_ROOT)/build
include $(BUILD_PATH)/verbose.mk

MAKE_ARGS += TEST=reflection

all: driver subject


driver:
	$(call echo,Building Test Driver)
	$(VERBOSE)$(MAKE) -C $(PROJECT_ROOT)/main $(MAKE_ARGS) SPARK_PRODUCT_ID=$(SPARK_TEST_DRIVER_PRODUCT_ID) SPARK_TEST_DRIVER=1

subject:
	$(call echo,Building Test Subject)
	$(VERBOSE)$(MAKE) -C $(PROJECT_ROOT)/main $(MAKE_ARGS) SPARK_PRODUCT_ID=$(SPARK_TEST_SUBJECT_PRODUCT_ID) SPARK_TEST_DRIVER=0

.PHONY: all driver subject









