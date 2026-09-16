suite('Panic info');

platform('gen3', 'gen4');
systemThread('disabled');
timeout(10 * 60 * 1000);

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