suite('Log file');

platform('rtl872x');
systemThread('enabled');

let device = null;

before(function() {
	device = this.particle.devices[0];
});

test('01_init', async function () {
});

test('02_enable_and_capture', async function () {
});

test('03_level_filter', async function () {
});

test('04_category_filter', async function () {
});

test('05_clear_and_keep_capturing', async function () {
});

test('06_rotation', async function () {
});

test('07_dropped_bytes_reported', async function () {
});

test('08_system_thread_auto_flush', async function () {
});

test('09_read_size_limit', async function () {
});

test('10_enable_and_reset', async function () {
});

test('11_log_survived_reset', async function () {
});

test('12_disable_and_reset', async function () {
});

test('13_still_disabled_after_reset', async function () {
});

test('99_cleanup', async function () {
});
