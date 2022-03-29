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
// Retained error message to print
static retained String BurninErrorMessage;

BurninTest::BurninTest() {
	tests = {
		&particle::BurninTest::test_gpio,
		&particle::BurninTest::test_wifi_scan,
		&particle::BurninTest::test_ble_scan,
		&particle::BurninTest::test_sram,
		&particle::BurninTest::test_spi_flash,
		&particle::BurninTest::test_cpu_load,
	};

	test_names = {"NONE", "GPIO", "WIFI_SCAN", "BLE_SCAN", "SRAM", "SPI_FLASH", "CPU_LOAD"};
}

BurninTest::~BurninTest() {
}

void BurninTest::setup() {
    // Read the SWD pin for a 1khz pulse. If present, enter burnin mode.
    hal_pin_t trigger_pin = D7; // A27 aka SWD
    PinMode trigger_pin_mode_at_start = getPinMode(trigger_pin);
	pinMode(trigger_pin, INPUT);
	Log.info("trigger_pin_mode_at_start %d", trigger_pin_mode_at_start);

    unsigned long pulse_width_micros = pulseIn(trigger_pin, HIGH);
    // Margin of error for rtl872xD tick/microsecond counter
    const unsigned long error_margin_micros = 31;
    const unsigned long expected_pulse_width_micros = 500;

    Log.info("trigger_pin %d pulse_width_micros: %lu ", trigger_pin, pulse_width_micros);
    if((pulse_width_micros > (expected_pulse_width_micros + error_margin_micros)) ||
       (pulse_width_micros < (expected_pulse_width_micros - error_margin_micros))) {
		BurninState = BurninTestState::DISABLED;
       	pinMode(trigger_pin, trigger_pin_mode_at_start);
    	return;
    }

	// TODO: Implement feature flag for P2 (backup ram uses flash page)?
	//System.enableFeature(FEATURE_RETAINED_MEMORY); 

	// Detect if backup SRAM has a failed test in it (IE state is "TEST FAILED")
	Log.info("BurninState: %d ErrorMessage: %s", (int)BurninState, BurninErrorMessage.c_str());

	if(BurninState == BurninTestState::IN_PROGRESS) {
		Log.warn("Previous test failed: %s", test_names[(int)LastBurnInTest].c_str());
		BurninState = BurninTestState::FAILED;
	}
	else if(BurninState == BurninTestState::FAILED) { 
		Log.info("Resetting from failed test state");
		BurninState = BurninTestState::IN_PROGRESS;
	}
	else {
		BurninState = BurninTestState::IN_PROGRESS;
	}

	// TODO: LEDStatus is overriden when in listening mode. If that ever gets fixed we can simplify LED management
	// LEDStatus tests_running(RGB_COLOR_GREEN, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_CRITICAL);
	// tests_running.setActive(true);
	RGB.control(true);
	start_time_millis = System.millis();
	next_blink_millis = start_time_millis + BLINK_PERIOD_MILLIS;
	next_status_report_millis = start_time_millis + BLINK_PERIOD_STATUS_REPORT_MILLIS;
    os_thread_create(&led_thread_, "led", OS_THREAD_PRIORITY_DEFAULT, led_loop, this, OS_THREAD_STACK_SIZE_DEFAULT);
}

static uint32_t print_runtime_info(void) {
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
			auto test = tests[random(tests.size())];
			uint32_t heap_start = System.freeMemory();
			bool test_passed = (this->*test)();
			uint32_t heap_end = System.freeMemory();

			if (!test_passed) {
				BurninState = BurninTestState::FAILED;
				Log.error("test failed: %s", test_names[(int)LastBurnInTest].c_str());
			}
			else {
				Log.info("test passed: %s", test_names[(int)LastBurnInTest].c_str());
			}

			Log.info("Heap delta: %.0f\n", ((double)heap_end) - ((double)heap_start));
		}
		break;
		case BurninTestState::FAILED:
		{
			// log failure text every X seconds to UART
			Log.error("***BURNIN_FAILED***, test, %s, message, %s", 
				test_names[(int)LastBurnInTest].c_str(),
				BurninErrorMessage.c_str());
			delay(5000);
		}
		break;
		default:
		break;
	}

	// // DEBUG: Stop tests after a bit
	// static int loops = 0;
	// if(loops++ > 50) {
	// 	BurninState = BurninTestState::FAILED;
	// 	// retention SRAM *will* persist with system reset, but not with reset button press
	// 	//System.reset(); 
	// }
}


static int blue_blinks_from_runtime(uint64_t run_time_millis) {
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

static void set_led_blue_or_green(bool blue){
	blue ? RGB.color(0, 0, 255) : RGB.color(0, 255, 0);
}

// Blink LED / signal uptime / failure state
void BurninTest::led_loop(void * arg) {
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
			if (current_millis > self->next_blink_millis) {
				// Time to report status? 
				if ((current_millis > self->next_status_report_millis) && (blink_blue_count == 0)) {
					blink_blue = true;
					blink_blue_count = blue_blinks_from_runtime(current_millis - self->start_time_millis);
				}
				led_on ? set_led_blue_or_green(blink_blue) : RGB.color(0, 0, 0);
				led_on = !led_on;
				self->next_blink_millis = current_millis + BLINK_PERIOD_MILLIS;

				// Count down blue blinks, stop reporting status when done 
				if (blink_blue && !led_on) {
					blink_blue_count -= 1;
					if (blink_blue_count == 0) {
						blink_blue = false;
						self->next_status_report_millis = current_millis + BLINK_PERIOD_STATUS_REPORT_MILLIS;
					}
				}
			}
		}
	}
}


bool BurninTest::test_gpio() {
	LastBurnInTest = BurninTestName::GPIO;

	const char * gpio_test_command = "{\"IO_TEST\":\"\"}";
	const char * gpio_test_pass_response = "{\"pass\":true}";
	bool testPassed = false;

	JSONValue gpioTestCommand = JSONValue::parseCopy(gpio_test_command);
	//Log.info("ioTest is valid: %d isObject() %d", gpioTestCommand.isValid(), gpioTestCommand.isObject());

	if(FqcTest::instance()->process(gpioTestCommand)) {
		String testResult = String(FqcTest::instance()->reply());
		if(testResult == gpio_test_pass_response) {
			testPassed = true;
		}
		// TODO: else log GPIO test that failed for debug message
	}
	else {
		Log.warn("IO Test not handled");	
	}

	return testPassed;
}

bool BurninTest::test_wifi_scan() {
	LastBurnInTest = BurninTestName::WIFI_SCAN;
	return true;
}

bool BurninTest::test_ble_scan() {
	LastBurnInTest = BurninTestName::BLE_SCAN;
	//BurninErrorMessage = "fake ble scan failure";
	return true;
}


// Linked list based, using fixed sized mallocs
bool BurninTest::test_sram() {
	LastBurnInTest = BurninTestName::SRAM;
	const uint32_t MAX_CHUNK_SIZE = 1024;
	bool test_passed = true;
 	Random rng = Random();
	memory_chunk* root = nullptr;
    size_t total_memory_allocated = 0;
    size_t chunk_count = 0;

    // Generate a random test pattern
    char * test_pattern = (char *)malloc(MAX_CHUNK_SIZE);
    if(!test_pattern) {
    	Log.error("Failed to alloc test pattern chunk");
		print_runtime_info();
    	test_passed = false;
    	// return early, no other allocations to free
    	return test_passed; 
    }
 
 	rng.gen(test_pattern, sizeof(test_pattern));
   
	
	while(System.freeMemory() > (MAX_CHUNK_SIZE * 3)){
		memory_chunk* b = new memory_chunk();
		if (!b) {
    		Log.error("Failed to alloc test chunk of size: %u", sizeof(memory_chunk));
			print_runtime_info();
    		test_passed = false;
			break;
		} else {
			total_memory_allocated += sizeof(memory_chunk);
			chunk_count++;
	    	memcpy(b->data, test_pattern, CHUNK_DATA_SIZE);
			b->next = root;
			root = b;
		}
	}

    Log.info("Allocated %u chunks with total size %u, free memory now %lu", chunk_count, total_memory_allocated, System.freeMemory());

	memory_chunk* current = root;
	while(test_passed && current) {
		if(memcmp(current->data, test_pattern, CHUNK_DATA_SIZE) != 0) {
			Log.error("Chunk @%p failed to match test pattern", current->data);
			test_passed = false;
			break;		
		}
		current = current->next;
	}

    // Free all allocated memory
	free(test_pattern);

	current = root;
	while(current) {
		memory_chunk* block_to_free = current;
		current = current->next;
		delete block_to_free;
	}

	return test_passed;
}

bool BurninTest::test_spi_flash(){
	LastBurnInTest = BurninTestName::SPI_FLASH;
	bool test_passed = true;

	// MBR part1, Bootloader, System Parts
	Vector<uint32_t> module_addresses = {0x08014000, 0x08004020, 0x08060000, /*0x08000000*/};

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
			// TODO: log module that failed
			break;
		}
	}

	// TODO: user module: start with suffix, find module header, go from there

	return test_passed;
}

bool BurninTest::test_cpu_load() {
	LastBurnInTest = BurninTestName::CPU_LOAD;
	return true;
}

}