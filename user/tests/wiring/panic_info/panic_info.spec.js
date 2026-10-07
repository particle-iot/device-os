suite('Panic info');

platform('gen3', 'gen4');
systemThread('disabled');
timeout(10 * 60 * 1000);

let device = null;

before(function() {
	device = this.particle.devices[0];
});

test('PANIC_INFO_01_expected_panic', async function() {
	// The device-side test notifies the runner about the upcoming panic via the
	// PANIC_PENDING mailbox message, waits for the ACK and panics; the runner then
	// expects the reboot, fetches the panic record, verifies it and clears it. All of
	// that is exercised by the waitTest() machinery itself, so this host-side body only
	// needs to observe the device mailbox message reported for the test
});

test('PANIC_INFO_02_no_panic_info_left', async function() {
	// After the expected panic the runner consumed and cleared the record; the
	// start-of-test panic check that runs before this test verifies the device is clean
	// and would fail this test if the record had reappeared after the reboot
});

test('PANIC_INFO_03_heap_access_with_scheduling_disabled', async function() {
	// Normal Mode: Recursive take succeeds on heap operation while suspended.
	// Strict Mode: Panics while suspended. 
	
	// Device notifies what HAL_PLATFORM_STRICT_HEAP_LOCK_PANIC setting it is using 
	for (const msg of device.mailBox.filter(m => m.t === 2)) {
		console.log(msg.d);
	}
});

test('PANIC_INFO_04_heap_access_with_scheduling_disabled_reason', async function() {
	// Normal Mode: nothing
	// Strict Mode: Verifies the reset reason is RESET_REASON_PANIC / HeapError 
});

test('PANIC_INFO_05_heap_access_contention_panic', async function() {
	// Normal mode: Force heap contention with scheduling disabled 
	// (ie what would be a deadlock scenario) in order to trigger panic

	// Strict mode: Skip, it is redundant
});

test('PANIC_INFO_06_heap_access_contention_did_panic', async function() {
	// Normal Mode: Verify panic happened

	// Strict Mode: Skip
});
