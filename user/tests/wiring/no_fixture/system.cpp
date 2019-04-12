
#include "application.h"
#include "unit-test/unit-test.h"


#if PLATFORM_ID >= 3
test(SYSTEM_01_freeMemory)
{
    // this test didn't work on the core attempting to allocate the current value of
    // freeMemory(), presumably because of fragmented heap from
    // relatively large allocations during the handshake, so the request was satisfied
    // without calling _sbrk()
    // 4096 was chosen to be small enough to allocate, but large enough to force _sbrk() to be called.)

    uint32_t free1 = System.freeMemory();
    if (free1>128) {
        void* m1 = malloc(1024*6);
        uint32_t free2 = System.freeMemory();
        free(m1);
        assertLess(free2, free1);
    }
}
#endif

test(SYSTEM_02_version)
{
    uint32_t versionNumber = System.versionNumber();
    // Serial.println(System.versionNumber()); // 16908417 -> 0x01020081
    // Serial.println(System.version().c_str()); // 1.2.0-rc.1
    char expected[20];
    // Order of testing here is important to retain
    if ((SYSTEM_VERSION & 0xFF) == 0xFF) {
        sprintf(expected, "%d.%d.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1));
    } else if ((SYSTEM_VERSION & 0xC0) == 0x00) {
        sprintf(expected, "%d.%d.%d-alpha.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1), (int)BYTE_N(versionNumber,0) & 0x3F);
    } else if ((SYSTEM_VERSION & 0xC0) == 0x40) {
        sprintf(expected, "%d.%d.%d-beta.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1), (int)BYTE_N(versionNumber,0) & 0x3F);
    } else if ((SYSTEM_VERSION & 0xC0) == 0x80) {
        sprintf(expected, "%d.%d.%d-rc.%d", (int)BYTE_N(versionNumber,3), (int)BYTE_N(versionNumber,2), (int)BYTE_N(versionNumber,1), (int)BYTE_N(versionNumber,0) & 0x3F);
    } else if ((SYSTEM_VERSION & 0xC0) >= 0xC0) {
        Serial.println("expected \"alpha\", \"beta\", \"rc\", or \"default\" version!");
        fail();
    }

    assertEqual( expected, System.version().c_str());
}

// todo - use platform feature flags
#if defined(STM32F2XX)
    // subtract 4 bytes for signature (3068 bytes)
    #define USER_BACKUP_RAM ((1024*3)-4)
#endif // defined(STM32F2XX)

#if defined(USER_BACKUP_RAM)

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

static retained uint8_t app_backup[USER_BACKUP_RAM];
static uint8_t app_ram[USER_BACKUP_RAM];

test(SYSTEM_03_user_backup_ram)
{
    int total_backup = 0;
    int total_ram = 0;
    for (unsigned i=0; i<(sizeof(app_backup)/sizeof(app_backup[0])); i++) {
        app_backup[i] = 1;
        app_ram[i] = 1;
        total_backup += app_backup[i];
        total_ram += app_ram[i];
    }
    // Serial.printlnf("app_backup(0x%x), app_ram(0x%x)", &app_backup, &app_ram);
    // Serial.printlnf("total_backup: %d, total_ram: %d", total_backup, total_ram);
    assertTrue(total_backup==(USER_BACKUP_RAM));
    assertTrue(total_ram==(USER_BACKUP_RAM));

    if (int(&app_backup) < 0x40024000) {
        Serial.printlnf("ERROR: expected app_backup in user backup memory, but was at %x", &app_backup);
    }
    assertTrue(int(&app_backup)>=0x40024000);

    if (int(&app_ram) >= 0x40024000) {
        Serial.printlnf("ERROR: expected app_ram in user sram memory, but was at %x", &app_ram);
    }
    assertTrue(int(&app_ram)<0x40024000);
}

#endif // defined(USER_BACKUP_RAM)

#if !HAL_PLATFORM_NRF52840 // TODO

#if defined(BUTTON1_MIRROR_SUPPORTED)
static int s_button_clicks = 0;
static void onButtonClick(system_event_t ev, int data) {
    s_button_clicks = data;
}

test(SYSTEM_04_button_mirror)
{
    System.buttonMirror(D1, FALLING, false);
    auto pinmap = HAL_Pin_Map();
    System.on(button_click, onButtonClick);

    // "Click" setup button 3 times
    // First click
    pinMode(D1, INPUT_PULLDOWN);
    // Just in case manually trigger EXTI interrupt
    EXTI_GenerateSWInterrupt(pinmap[D1].gpio_pin);
    delay(300);
    pinMode(D1, INPUT_PULLUP);
    delay(100);

    // Second click
    pinMode(D1, INPUT_PULLDOWN);
    // Just in case manually trigger EXTI interrupt
    EXTI_GenerateSWInterrupt(pinmap[D1].gpio_pin);
    delay(300);
    pinMode(D1, INPUT_PULLUP);
    delay(100);

    // Third click
    pinMode(D1, INPUT_PULLDOWN);
    // Just in case manually trigger EXTI interrupt
    EXTI_GenerateSWInterrupt(pinmap[D1].gpio_pin);
    delay(300);
    pinMode(D1, INPUT_PULLUP);
    delay(300);

    assertEqual(s_button_clicks, 3);
}

test(SYSTEM_05_button_mirror_disable)
{
    System.disableButtonMirror(false);
}
#endif // defined(BUTTON1_MIRROR_SUPPORTED)

#endif // !HAL_PLATFORM_NRF52840

#if PLATFORM_ID!=0
// platform supports out of memory notifiation

bool oomEventReceived = false;
size_t oomSizeReceived = 0;
void handle_oom(system_event_t event, int param, void*) {
	Serial.printlnf("got event %d %d", event, param);
	if (out_of_memory==event) {
		oomEventReceived = true;
		oomSizeReceived = param;
	}
};

void register_oom() {
	oomEventReceived = false;
	oomSizeReceived = 0;
	System.on(out_of_memory, handle_oom);
}

void unregister_oom() {
	System.off(out_of_memory, handle_oom);
}


test(SYSTEM_06_out_of_memory)
{
	const size_t size = 1024*1024*1024;
	register_oom();
	malloc(size);
	Particle.process();
	unregister_oom();

	assertTrue(oomEventReceived);
	assertEqual(oomSizeReceived, size);
}

test(SYSTEM_07_fragmented_heap) {
	struct block {
		char data[508];
		block* next;
	};
	register_oom();

	block* next = nullptr;

	// exhaust memory
	for (;;) {
		block* b = new block();
		if (!b) {
			break;
		} else {
			b->next = next;
			next = b;
		}
	}

	assertTrue(oomEventReceived);
	assertEqual(oomSizeReceived, sizeof(block));

	runtime_info_t info;
	info.size = sizeof(info);
	HAL_Core_Runtime_Info(&info, nullptr);

	// we can't really say about the free heap but the block size should be less
	assertLessOrEqual(info.largest_free_block_heap, sizeof(block));
	size_t low_heap = info.freeheap;

	// free every 2nd block
	block* head = next;
	int count = 0;
	for (;head;) {
		block* free = head->next;
		if (free) {
			// skip the next block
			head->next = free->next;
			delete free;
			count++;
			head = head->next;
		} else {
			head = nullptr;
		}
	}

	HAL_Core_Runtime_Info(&info, nullptr);
	const size_t half_fragment_block_size = info.largest_free_block_heap;
	const size_t half_fragment_free = info.freeheap;

	unregister_oom();
	register_oom();
	const size_t BLOCKS_TO_MALLOC = 3;
	block* b = new block[BLOCKS_TO_MALLOC];  // no room for 3 blocks, memory is clearly fragmented
	delete b;

	// free the remaining blocks
	for (;next;) {
		block* b = next;
		next = b->next;
		delete b;
	}

	assertMoreOrEqual(half_fragment_block_size, sizeof(block)); // there should definitely be one block available
	assertLessOrEqual(half_fragment_block_size, BLOCKS_TO_MALLOC*sizeof(block)-1); // we expect malloc of 3 blocks to fail, so this better allow up to that size less 1
	assertMoreOrEqual(half_fragment_free, low_heap+(sizeof(block)*count));

	assertTrue(oomEventReceived);
	assertMoreOrEqual(oomSizeReceived, sizeof(block)*BLOCKS_TO_MALLOC);
}

test(SYSTEM_08_out_of_memory_not_raised_for_0_size_malloc)
{
	const size_t size = 0;
	register_oom();
	malloc(size);
	Particle.process();
	unregister_oom();

	assertFalse(oomEventReceived);
}



#endif
