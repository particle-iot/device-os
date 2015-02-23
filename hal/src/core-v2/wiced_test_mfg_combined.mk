PLATFORM_ID=6
CORE=../../../..
WICED?=$(CORE)/WICED/WICED-SDK-3.1.1/WICED-SDK
$(info WICED $(WICED))
FIRMWARE=$(CORE)/firmware
FIRMWARE_BUILD=$(FIRMWARE)/build
OUT=$(WICED)/build
DCT_MEM=$(OUT)/dct_pad.bin
BOOTLOADER_BIN=$(FIRMWARE_BUILD)/target/bootloader/platform-$(PLATFORM_ID)/bootloader.bin
BOOTLOADER_MEM=$(OUT)/bootloader_pad.bin
BOOTLOADER_DIR=$(FIRMWARE)/bootloader

FIRMWARE_BIN=$(FIRMWARE_BUILD)/target/main/platform-$(PLATFORM_ID)/main.bin
FIRMWARE_MEM=$(OUT)/main_pad.bin
FIRMWARE_DIR=$(FIRMWARE)/main
COMBINED_MEM=$(OUT)/combined.bin

CMD=test.mfg_test-BCM9WCDUSI09-ThreadX-NetX-SDIO
BUILD_NAME=test_mfg_test-BCM9WCDUSI09-ThreadX-NetX-SDIO
MFG_TEST_BIN=$(WICED)/build/$(BUILD_NAME)/binary/$(BUILD_NAME).bin
MFG_TEST_MEM=$(OUT)/mfg_test_pad.bin

OPTS=

all: bootloader
	
	
# bootloader mfg_test


clean:
	cd "$(WICED)"; "$(WICED)/make" clean
	-rm $(MFG_TEST_BIN)
	-rm $(BOOTLOADER)
	-rm $(DCT)
	
mfg_test: $(MFG_TEST_PAD)
	dd if=/dev/zero ibs=1k count=16 | tr "\000" "\377" > $(BOOTLOADER_MEM)
	dd if=$(BOOTLOADER_BIN) of=$(BOOTLOADER_MEM) conv=notrunc
	


bootloader:
	echo building $(BOOTLOADER_MEM)
	$(MAKE) -C $(BOOTLOADER_DIR) PLATFORM_ID=$(PLATFORM_ID) all 
	dd if=/dev/zero ibs=1k count=16 | tr "\000" "\377" > $(BOOTLOADER_MEM)
	dd if=$(BOOTLOADER_BIN) of=$(BOOTLOADER_MEM) conv=notrunc
	

$(MFG_TEST_BIN):
	cd "$(WICED)"; "./make" $(CMD) $(OPTS)
	$(call,echo,'Appending: CRC32 to the Flash Image')
	$(VERBOSE)cp $@ $@.no_crc	
	$(VERBOSE)$(CRC) $@.no_crc | cut -c 1-10 | $(XXD) -r -p >> $@
	$(call,echo,)	

$(MFG_TEST_MEM): $(MFG_TEST_BIN)
	dd if=/dev/zero ibs=1k count=384 | tr "\000" "\377" > $(MFG_TEST_MEM)
	dd if=$(MFG_TEST_BIN) of=$(MFG_TEST_MEM) conv=notrunc
			
mfg_test: $(MFG_TEST_MEM)

dct: $(DCT_MEM)


$(DCT_MEM):
	dd if=/dev/zero ibs=1k count=112 | tr "\000" "\377" > $(DCT_MEM)

firmware:
	echo building $(FIRMWARE_MEM)
	$(MAKE) -C $(FIRMWARE_DIR) PLATFORM_ID=$(PLATFORM_ID) PRODUCT_ID=$(PLATFORM_ID) PRODUCT_FIRMWARE_REVISION=0 all 
	dd if=/dev/zero ibs=1k count=384 | tr "\000" "\377" > $(FIRMWARE_MEM)
	dd if=$(FIRMWARE_BIN) of=$(FIRMWARE_MEM) conv=notrunc
	

combined: bootloader dct mfg_test firmware
	cat $(BOOTLOADER_MEM) $(DCT_MEM) $(MFG_TEST_MEM) $(FIRMWARE_MEM) > $(COMBINED_MEM)


flash: combined
	st-flash write $(COMBINED_MEM) 0x8000000

.PHONY: mfg_test clean all bootloader dct mfg_test firmware





