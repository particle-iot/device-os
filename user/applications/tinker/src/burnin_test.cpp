#include "application.h"

#include "burnin_test.h"
#include "fqc_test.h"

#include "spark_wiring_logging.h"
#include "spark_wiring_random.h"
#include "spark_wiring_led.h"
#include "random.h"

namespace particle {

// Global instance of this class called from the application
BurninTest Burnin;

// Retained state variables
static retained BurninTest::BurninTestState BurninState;
static retained BurninTest::BurninTestName LastBurnInTest;
static retained uint32_t FailureUptimeMillis;
static retained char BurninErrorMessage[1024];

// TODO: instantiate at runtime instead
Serial1LogHandler logger(115200, LOG_LEVEL_ALL);

BurninTest::BurninTest() {
	tests_ = {
		&particle::BurninTest::testGpio,
		&particle::BurninTest::testWifiScan,
		&particle::BurninTest::testBleScan,
		&particle::BurninTest::testSram,
		&particle::BurninTest::testSpiFlash,
		&particle::BurninTest::testCpuLoad,
	};

	test_names_ = {"NONE", "GPIO", "WIFI_SCAN", "BLE_SCAN", "SRAM", "SPI_FLASH", "CPU_LOAD"};
}

BurninTest::~BurninTest() {
}

void BurninTest::setup() {
    // Read the trigger pin for a 1khz pulse. If present, enter burnin mode.
    hal_pin_t trigger_pin = D7; // PA27 aka SWD
    PinMode trigger_pin_mode_at_start = getPinMode(trigger_pin);
	pinMode(trigger_pin, INPUT);
	//Log.info("trigger_pin_mode_at_start %d", trigger_pin_mode_at_start);

    uint32_t pulse_width_micros = pulseIn(trigger_pin, HIGH);
    // Margin of error for rtl872xD tick/microsecond counter
    const uint32_t error_margin_micros = (31 * 2);
    // 1KHZ square wave at 50% duty cycle = 500us pulses
    const uint32_t expected_pulse_width_micros = 500;

    Log.info("trigger_pin %d pulse_width_micros: %lu ", trigger_pin, pulse_width_micros);
    if((pulse_width_micros > (expected_pulse_width_micros + error_margin_micros)) ||
       (pulse_width_micros < (expected_pulse_width_micros - error_margin_micros))) {
		BurninState = BurninTestState::DISABLED;
		// TODO: Figure out how to leave device in programmable state with SWD pin
		// Re-configure for SWD instead of GPIO?
       	pinMode(trigger_pin, trigger_pin_mode_at_start); 
    	return;
    }

	// Detect if backup SRAM has a failed test in it (IE state is "TEST FAILED")
	Log.info("BurninState: %d ErrorMessage: %s", (int)BurninState, BurninErrorMessage);

	if(BurninState == BurninTestState::IN_PROGRESS) {
		Log.warn("Previous test failed: %s", test_names_[(int)LastBurnInTest].c_str());
		BurninState = BurninTestState::FAILED;
	}
	else if(BurninState == BurninTestState::FAILED) {
		Log.info("Resetting from failed test state");
		BurninState = BurninTestState::IN_PROGRESS;
	}
	else {
		BurninState = BurninTestState::IN_PROGRESS;
		FailureUptimeMillis = 0;
	}

	RGB.control(true);
	start_time_millis_ = System.millis();
	next_blink_millis_ = start_time_millis_ + BLINK_PERIOD_MILLIS;
	next_status_report_millis_ = start_time_millis_ + BLINK_PERIOD_STATUS_REPORT_MILLIS;
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
			// pick random test to run, run it
			auto test = tests_[random(tests_.size())];
			uint32_t heap_start = System.freeMemory();
			bool test_passed = (this->*test)();
			uint32_t heap_end = System.freeMemory();

			if (!test_passed) {
				BurninState = BurninTestState::FAILED;
				FailureUptimeMillis = millis();
				Log.error("test failed: %s", test_names_[(int)LastBurnInTest].c_str());
			}
			else {
				Log.info("test passed: %s", test_names_[(int)LastBurnInTest].c_str());
			}

			Log.info("Heap delta: %.0f\n", ((double)heap_end) - ((double)heap_start)); // Debug
		}
		break;
		case BurninTestState::FAILED:
		{
			// log failure text every X seconds to UART
			Log.error("***BURNIN_FAILED***, UptimeMillis, %lu, test, %s, message, %s", 
				FailureUptimeMillis,
				test_names_[(int)LastBurnInTest].c_str(),
				BurninErrorMessage);
			delay(5000);
		}
		break;
		default:
		break;
	}
}


static int blueBlinksFromRuntime(uint64_t run_time_millis) {
	//#define HOURS_TO_MILLIS(hours) (hours * 60 * 60 * 1000)
	#define HOURS_TO_MILLIS(hours) (hours * 1000) // DEBUG: Based on seconds instead

	if(run_time_millis >= HOURS_TO_MILLIS(72)){
		return 4;
	} else if(run_time_millis >= HOURS_TO_MILLIS(48)){
		return 3;
	} else if(run_time_millis >= HOURS_TO_MILLIS(24)){
		return 2;
	} else {
		return 1;
	}
}

static void setLed(bool on, bool blue){
	if(!on){
		RGB.color(0, 0, 0);
	}
	else if(blue) {
		RGB.color(0, 0, 255);
	}
	else {
		RGB.color(0, 255, 0);
	}
}

// Blink LED / signal uptime / failure state
void BurninTest::ledLoop(void * arg) {
    BurninTest* self = static_cast<BurninTest*>(arg);

    while (true) {
		static bool led_on = false;
		
		static bool blink_blue = false;
		static int blink_blue_count = 0;

		uint64_t current_millis = System.millis();

		// If failed = solid red
		if(BurninState == BurninTestState::FAILED) {
			RGB.color(255, 0, 0);
		}
		// If running = blink led
		else if (BurninState == BurninTestState::IN_PROGRESS) {
			// Time to blink?
			if (current_millis > self->next_blink_millis_) {
				// Time to report status? 
				if ((current_millis > self->next_status_report_millis_) && (blink_blue_count == 0)) {
					blink_blue = true;
					blink_blue_count = blueBlinksFromRuntime(current_millis - self->start_time_millis_);
				}

				setLed(led_on, blink_blue);
				led_on = !led_on;
				self->next_blink_millis_ = current_millis + BLINK_PERIOD_MILLIS;

				// Count down blue blinks, stop reporting status when done 
				if (blink_blue && !led_on) {
					blink_blue_count -= 1;
					if (blink_blue_count == 0) {
						blink_blue = false;
						self->next_status_report_millis_ = current_millis + BLINK_PERIOD_STATUS_REPORT_MILLIS;
					}
				}
			}
		}
	}
}


bool BurninTest::testGpio() {
	LastBurnInTest = BurninTestName::GPIO;

	char buffer[256] = {};
	JSONBufferWriter writer(buffer, sizeof(buffer)-1);
	writer.beginObject().name("IO_TEST").value("").endObject();

	const char * gpio_test_pass_response = "{\"pass\":true}";
	bool testPassed = true;

	JSONValue gpioTestCommand = JSONValue::parseCopy(buffer);
	FqcTest::instance()->process(gpioTestCommand);
	String testResult = String(FqcTest::instance()->reply());
	if (testResult != gpio_test_pass_response) {
		testPassed = false;
		strlcpy(BurninErrorMessage, testResult.c_str(), sizeof(BurninErrorMessage));
	}
	
	return testPassed;
}

bool BurninTest::testWifiScan() {
	LastBurnInTest = BurninTestName::WIFI_SCAN;

	char buffer[256] = {};
	JSONBufferWriter writer(buffer, sizeof(buffer)-1);
	writer.beginObject().name("WIFI_SCAN_NETWORKS").value("").endObject();

	const char * wifi_scan_pass_response = "{\"pass\":true";
	bool testPassed = true;

	JSONValue wifiScanCommand = JSONValue::parseCopy(buffer);
	FqcTest::instance()->process(wifiScanCommand);
	String testResult = String(FqcTest::instance()->reply());
	if(!testResult.startsWith(wifi_scan_pass_response))
	{
		testPassed = false;
		strlcpy(BurninErrorMessage, testResult.c_str(), sizeof(BurninErrorMessage));
	}

	return testPassed;
}

bool BurninTest::testBleScan() {
	LastBurnInTest = BurninTestName::BLE_SCAN;
	// TODO:
	return true;
}


// Linked list based, using fixed sized mallocs
bool BurninTest::testSram() {
	LastBurnInTest = BurninTestName::SRAM;
	const uint32_t MAX_CHUNK_SIZE = 1024;
	bool test_passed = true;
 	Random rng;
	MemoryChunk* root = nullptr;
    size_t total_memory_allocated = 0;
    size_t chunk_count = 0;

    // Generate a random test pattern
    char * test_pattern = (char *)malloc(MAX_CHUNK_SIZE);
    if(!test_pattern) {
		strlcpy(BurninErrorMessage, "Failed to alloc test pattern chunk", sizeof(BurninErrorMessage));
		printRuntimeInfo();
    	test_passed = false;
    	// return early, no other allocations to free
    	return test_passed; 
    }
 
 	rng.gen(test_pattern, sizeof(test_pattern));
   
    // Exhaust most of memory
	while(System.freeMemory() > (MAX_CHUNK_SIZE * 3)){
		MemoryChunk* b = new MemoryChunk();
		if (!b) {
			String errorMessage = "Failed to alloc test chunk of size: ";
			errorMessage += String(sizeof(MemoryChunk), DEC);
			strlcpy(BurninErrorMessage, errorMessage.c_str(), sizeof(BurninErrorMessage));
			printRuntimeInfo();
    		test_passed = false;
			break;
		} else {
			total_memory_allocated += sizeof(MemoryChunk);
			chunk_count++;
	    	memcpy(b->data, test_pattern, CHUNK_DATA_SIZE);
			b->next = root;
			root = b;
		}
	}

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

bool BurninTest::testSpiFlash(){
	LastBurnInTest = BurninTestName::SPI_FLASH;
	bool test_passed = true;

	// MBR part1, Bootloader, System Parts
	// TODO: Dont use P2 specific addresses
	Vector<uint32_t> module_addresses = {0x08014000, 0x08004020, 0x08060000 /*0x08000000*/};

	for (auto & address : module_addresses) {
		module_info_t *module_header = (module_info_t*)(address);
		uint32_t module_start =  (uint32_t)module_header->module_start_address;
		uint32_t module_end = (uint32_t)module_header->module_end_address;
		uint32_t module_size = module_end - module_start;
		Log.info("module start: 0x%08lX", module_start);
		Log.info("module end  : 0x%08lX", module_end);
		Log.info("module size : 0x%08lX", module_size);

		uint32_t calculated_crc = HAL_Core_Compute_CRC32((uint8_t *)module_header->module_start_address, module_size);
		uint32_t module_crc = *(uint32_t *)(module_header->module_end_address);
		uint8_t * reverse_crc = (uint8_t *)&module_crc;
		std::reverse(reverse_crc, reverse_crc + 4);
		Log.info("calculated crc: 0x%08lX module crc: 0x%08lX", calculated_crc, module_crc);

		if(calculated_crc != module_crc) {
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

	// TODO: user module: start with suffix, find module header, go from there

	return test_passed;
}

bool BurninTest::testCpuLoad() {
	LastBurnInTest = BurninTestName::CPU_LOAD;
	// TODO:
	return true;
}

}