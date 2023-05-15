/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "hal_platform.h"
#if HAL_PLATFORM_RTL872X && defined(ENABLE_FQC_FUNCTIONALITY) 
#include "application.h"

// Needed for hal_storage_read
#define PARTICLE_USE_UNSTABLE_API 1 

#include "spark_wiring_logging.h"
#include "spark_wiring_random.h"
#include "spark_wiring_led.h"
#include "random.h"
#include "storage_hal.h"
#include "softcrc32.h"

#include "fqc_test.h"
#include "burnin_test.h"
extern "C" {
#include "core_portme.h"
}

extern uintptr_t platform_km0_part1_flash_start;
extern uintptr_t platform_bootloader_module_info_flash_start;
extern uintptr_t platform_system_part1_flash_start;

namespace particle {

// Retained state variables
static retained BurninTest::BurninTestState BurninState;
static retained char LastBurnInTest[16];
static retained uint32_t UptimeMillis;
static retained char BurninErrorMessage[1024];

BurninTest::BurninTest() {
	tests_ = {
		&particle::BurninTest::testGpio,
		&particle::BurninTest::testWifiScan,
		&particle::BurninTest::testBleScan,
		&particle::BurninTest::testSram,
		&particle::BurninTest::testSpiFlash,
		&particle::BurninTest::testCpuLoad,
	};

	test_names_ = {
		"GPIO", 
		"WIFI_SCAN", 
		"BLE_SCAN", 
		"SRAM", 
		"SPI_FLASH", 
		"CPU_LOAD"
	};
}

BurninTest::~BurninTest() {
}

BurninTest* BurninTest::instance() {
    static BurninTest tester;
    return &tester;
}

void BurninTest::setup(bool forceEnable) {
	if (!forceEnable) {
	    hal_pin_t trigger_pin = D7; // PA27 aka SWD
		// Read the trigger pin for a 1khz pulse. If present, enter burnin mode.
		pinMode(trigger_pin, INPUT);
	    uint32_t pulse_width_micros = pulseIn(trigger_pin, HIGH);
	    pinMode(trigger_pin, PIN_MODE_SWD);

	    const uint32_t error_margin_micros = 31;
	    // 1KHZ square wave at 50% duty cycle = 500us pulses
	    const uint32_t expected_pulse_width_micros = 500;

	    if((pulse_width_micros > (expected_pulse_width_micros + error_margin_micros)) ||
	       (pulse_width_micros < (expected_pulse_width_micros - error_margin_micros))) {
			BurninState = BurninTestState::DISABLED;
	    	return;
	    }
	}

    logger_ = std::make_unique<Serial1LogHandler>(115200, LOG_LEVEL_INFO);
    
	// Detect if backup SRAM has a failed test in it (IE state is "TEST FAILED")
	Log.info("BURN IN START: ResetReason: %d State: %d ErrorMessage: %s", System.resetReason(), (int)BurninState, BurninErrorMessage);

	if(BurninState == BurninTestState::IN_PROGRESS) {
		Log.warn("Previous test failed: %s", LastBurnInTest);
		BurninState = BurninTestState::FAILED;
	}
	else if(BurninState == BurninTestState::FAILED) {
		Log.info("Resetting from failed test state: %s", LastBurnInTest);
		BurninState = BurninTestState::IN_PROGRESS;
		UptimeMillis = 0;
	}
	else {
		BurninState = BurninTestState::IN_PROGRESS;
		UptimeMillis = 0;
	}

	RGB.control(true);
    os_thread_create(&led_thread_, "led", OS_THREAD_PRIORITY_DEFAULT, ledLoop, this, OS_THREAD_STACK_SIZE_DEFAULT);
}

static uint32_t printRuntimeInfo(void) {
    runtime_info_t info;
    memset(&info, 0, sizeof(info));
    info.size = sizeof(info);
    HAL_Core_Runtime_Info(&info, NULL);
    Log.info("freeheap: %lu total_heap %lu largest_free_block_heap %lu max_used_heap %lu", 
    	info.freeheap, 
    	info.total_heap, 
    	info.largest_free_block_heap,
    	info.max_used_heap);

    return info.freeheap;
}

void BurninTest::loop() {
	switch(BurninState){
		case BurninTestState::DISABLED:
			return;
		case BurninTestState::IN_PROGRESS:
		{
			UptimeMillis = millis();
			auto startMillis = UptimeMillis;

			// pick random test to run, run it
			auto test_number = random(tests_.size());
			auto test = tests_[test_number];
			strlcpy(LastBurnInTest, test_names_[test_number].c_str(), sizeof(LastBurnInTest));
			Log.info("Running test: %s", LastBurnInTest);
			bool test_passed = (this->*test)();

			if (!test_passed) {
				BurninState = BurninTestState::FAILED;
				Log.error("Elapsed: %lu test failed: %s\n", millis() - startMillis, LastBurnInTest);
			}
			else {
				Log.info("Elapsed: %lu test passed: %s\n", millis() - startMillis, LastBurnInTest);
			}
		}
		break;
		case BurninTestState::FAILED:
		{
			// log failure text every X seconds to UART
			Log.error("FAILED: Uptime: %lu Test: %s ResetReason %d Message: %s", 
				UptimeMillis,
				LastBurnInTest,
				System.resetReason(),
				BurninErrorMessage);
			delay(5000);
		}
		break;
		default:
		break;
	}
}

static int blueBlinksFromRuntime(uint64_t run_time_millis) {
	if(run_time_millis >= std::chrono::duration_cast<std::chrono::milliseconds>(72h).count()){
		return 4;
	} else if(run_time_millis >= std::chrono::duration_cast<std::chrono::milliseconds>(48h).count()){
		return 3;
	} else if(run_time_millis >= std::chrono::duration_cast<std::chrono::milliseconds>(24h).count()){
		return 2;
	} else {
		return 1;
	}
}

static void blinkLed(int blink_time, int red, int green, int blue) {
	RGB.color(red, green, blue);
	delay(blink_time / 2);
	RGB.color(0, 0, 0);
	delay(blink_time / 2);
}

// Blink LED / signal uptime / failure state
void BurninTest::ledLoop(void * arg) {
	system_tick_t total_runtime_millis = 0;
	int blinks_this_period = 0;

    while (true) {
		// If failed = solid red
		if(BurninState == BurninTestState::FAILED) {
			RGB.color(255, 0, 0);
		}
		// If running = blink led
		else if (BurninState == BurninTestState::IN_PROGRESS) {

			blinkLed(BLINK_PERIOD_MILLIS, 0, 255, 0);
			total_runtime_millis += BLINK_PERIOD_MILLIS;
			blinks_this_period++;

			int blue_blinks = blueBlinksFromRuntime(total_runtime_millis);
			Log.trace("total_runtime_millis: %lu blinks_this_period: %d blue_blinks: %d", total_runtime_millis, blinks_this_period, blue_blinks);

			if (blue_blinks + blinks_this_period == BLINK_COUNT_PER_CYCLE) {
				for (int i = 0; i < blue_blinks; i++) {
					blinkLed(BLINK_PERIOD_MILLIS, 0, 0, 255);
					total_runtime_millis += BLINK_PERIOD_MILLIS;
				}

				blinks_this_period = 0;
			}
		}
	}
}


static JSONValue getValue(const JSONValue& obj, const char* name) {
    JSONObjectIterator it(obj);
    while (it.next()) {
        if (it.name() == name) {
            return it.value();
        }
    }
    return JSONValue();
}

bool BurninTest::callFqcTest(String testName){
	bool testPassed = true;
	char buffer[256] = {};

	JSONBufferWriter writer(buffer, sizeof(buffer)-1);
	writer.beginObject().name(testName).value("").endObject();

	JSONValue gpioTestCommand = JSONValue::parseCopy(buffer);
	FqcTest::instance()->process(gpioTestCommand);

	char * fqcTestRawReply = FqcTest::instance()->reply();
	JSONValue testResult = getValue(JSONValue::parseCopy(fqcTestRawReply), "pass");
	if (!testResult.isValid()) {
		const char* jsonPassString = "\"pass\":true";
		if(!strstr(fqcTestRawReply, jsonPassString)) {
			strlcpy(BurninErrorMessage, fqcTestRawReply, sizeof(BurninErrorMessage));
			testPassed = false;
		}
	}
	else if(testResult.toString() != "true") {
		strlcpy(BurninErrorMessage, fqcTestRawReply, sizeof(BurninErrorMessage));
		testPassed = false;
	}
	
	return testPassed;
}

bool BurninTest::testGpio() {
	return callFqcTest("IO_TEST");
}

bool BurninTest::testWifiScan() {
	return callFqcTest("WIFI_SCAN_NETWORKS");
}

bool BurninTest::testBleScan() {
	const size_t SCAN_RESULT_MAX = 30;
	BleScanResult scanResults[SCAN_RESULT_MAX];
	BLE.on();
	BLE.setScanTimeout(50);
	int count = BLE.scan(scanResults, SCAN_RESULT_MAX);
	if (count < 0) {
		Log.error("BLE scan failed: %s", get_system_error_message(count));
	}
	else {
		Log.info("Found %d beacons", count);

		for (int ii = 0; ii < count; ii++) {
			uint8_t buf[BLE_MAX_ADV_DATA_LEN];
			size_t len = scanResults[ii].advertisingData().get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, BLE_MAX_ADV_DATA_LEN);
			Log.info("Beacon %d: rssi %d Advertising Data Len %u", ii, scanResults[ii].rssi(), len);
			if (len > 0) {
				Log.print("Advertising Data: ");
				Log.dump(buf, len);	
				Log.print("\r\n");
			}
		}
	}

	BLE.off();
	return true;
}

bool BurninTest::testSram() {
	bool test_passed = true;
 	Random rng;
	MemoryChunk* root = nullptr;
    size_t total_memory_allocated = 0;
    size_t chunk_count = 0;

    // Generate a random test pattern
    char * test_pattern = (char *)malloc(CHUNK_DATA_SIZE);
    if(!test_pattern) {
		strlcpy(BurninErrorMessage, "Failed to alloc test pattern chunk", sizeof(BurninErrorMessage));
		printRuntimeInfo();
    	test_passed = false;
    	// return early, no other allocations to free
    	return test_passed; 
    }
 	rng.gen(test_pattern, CHUNK_DATA_SIZE);
   
   	printRuntimeInfo();
    // Exhaust memory, leaving some free space for OS operation
    while(System.freeMemory() > (10 * 1024)) {
		MemoryChunk* b = new MemoryChunk();
		if (!b) {
			break;
		} else {
			total_memory_allocated += sizeof(MemoryChunk);
			chunk_count++;
	    	memcpy(b->data, test_pattern, CHUNK_DATA_SIZE);
			b->next = root;
			root = b;
		}
	}
	printRuntimeInfo();
    Log.info("Allocated %u chunks with total size %u, free memory now %lu", chunk_count, total_memory_allocated, System.freeMemory());

	MemoryChunk* current = root;
	while(test_passed && current) {
		if(memcmp(current->data, test_pattern, CHUNK_DATA_SIZE) != 0) {
			String errorMessage = "Chunk failed to match test pattern. ptr @:0x";
			errorMessage += String(current->data, HEX);
			strlcpy(BurninErrorMessage, errorMessage.c_str(), sizeof(BurninErrorMessage));
			test_passed = false;
			break;		
		}
		current = current->next;
	}

    SCOPE_GUARD({
	    // Free all allocated memory
		free(test_pattern);

		current = root;
		while(current) {
			MemoryChunk* block_to_free = current;
			current = current->next;
			delete block_to_free;
		}
	});
	return test_passed;
}

const int SPI_FLASH_BUFFER_SIZE = 4096;
static uint8_t spi_flash_buffer[SPI_FLASH_BUFFER_SIZE] = {};

bool BurninTest::testSpiFlash(){
	bool test_passed = true;

#if HAL_PLATFORM_RTL872X
	// MBR part1, Bootloader, System Parts
	Vector<uint32_t> module_addresses = {(uint32_t)&platform_km0_part1_flash_start, (uint32_t)&platform_bootloader_module_info_flash_start, (uint32_t)&platform_system_part1_flash_start};
#else
	// TODO: Non P2 platforms
	Vector<uint32_t> module_addresses = {};
#endif

	int hal_read_result = 0;

	for (auto & address : module_addresses) {
		module_info_t module = {};
		Log.trace("Module Addr 0x%08lx", address);
		hal_read_result = hal_storage_read(HAL_STORAGE_ID_INTERNAL_FLASH, address, (uint8_t*)&module, sizeof(module));
		if (hal_read_result != sizeof(module)) {
			test_passed = false;
			break;
		}
		
		uint32_t module_start =  (uint32_t)module.module_start_address;
		uint32_t module_end = (uint32_t)module.module_end_address;
		uint32_t module_size = module_end - module_start;

		Log.info("module start: 0x%08lX end: 0x%08lX size: 0x%08lX", module_start, module_end, module_size);

		uint32_t calculated_crc = 0;
		uint32_t bytesRemaining = module_size;
		uint32_t readAddress = module_start;
		while (bytesRemaining) {
			memset(spi_flash_buffer, 0x00, sizeof(spi_flash_buffer));
			// Determine chunk size to read
			uint32_t chunk_size = bytesRemaining >= SPI_FLASH_BUFFER_SIZE ? SPI_FLASH_BUFFER_SIZE : bytesRemaining;
			
			// Read image bytes into buff
			Log.trace("hal_storage_read addr 0x%08lx size 0x%04lx", readAddress, chunk_size);
			hal_read_result = hal_storage_read(HAL_STORAGE_ID_INTERNAL_FLASH, readAddress, spi_flash_buffer, chunk_size);

			if (hal_read_result < 0) {
				test_passed = false;
				break;
			}

			// crc buffer
			calculated_crc = particle::softCrc32(spi_flash_buffer, chunk_size, &calculated_crc);

			readAddress += chunk_size;
			bytesRemaining -= chunk_size;
		}

		if (!test_passed) {
			// If we failed to read module data, dont bother reading the other modules
			break;
		}

		uint32_t module_crc;
		hal_read_result = hal_storage_read(HAL_STORAGE_ID_INTERNAL_FLASH, module_end, (uint8_t *)&module_crc, sizeof(module_crc));
		if (hal_read_result < 0) {
			test_passed = false;
			break;
		}

		uint8_t * reverse_crc = (uint8_t *)&module_crc;
		std::reverse(reverse_crc, reverse_crc + 4);
		Log.info("calculated crc: 0x%08lX module crc: 0x%08lX", calculated_crc, module_crc);

		if (calculated_crc != module_crc) {
			test_passed = false;
			// log module that failed
			String errorMessage = String(module_start, HEX);
			errorMessage += String(" module crc: ");
			errorMessage += String(module_crc, HEX);
			errorMessage += String(" calculated crc: ");
			errorMessage += String(calculated_crc, HEX);
			strlcpy(BurninErrorMessage, errorMessage.c_str(), sizeof(BurninErrorMessage));
			break;
		}
	}

	if (!test_passed && hal_read_result < 0) {
		Log.error("hal_storage_read failed %d", hal_read_result);
		String errorMessage = String("hal_storage_read failed ");
		errorMessage += String(hal_read_result);
		strlcpy(BurninErrorMessage, errorMessage.c_str(), sizeof(BurninErrorMessage));
	}

	return test_passed;
}

bool BurninTest::testCpuLoad() {
	// 4500 iterations is about enough to meet the 10 second test duration minimum
	coremark_set_iterations(random(4500, 6000));
	coremark_main();
	return true;
}

} // particle
#endif // HAL_PLATFORM_RTL872X
