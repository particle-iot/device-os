suite('Cellular eSIM idle state');

platform('cellular', 'msom');

// A stalled AT command can hold the modem for up to three minutes before the parser resyncs, and
// the device side rides that out instead of failing, so every test here needs room for it.
timeout(10 * 60 * 1000);

let device;
let savedIccid;

before(async function() {
    device = this.particle.devices[0];
});

test('00_esim_setup', async function () {
    // No network filter: a Wi-Fi msom has an eUICC to provision too, and the device side skips on
    // its own when there is no eUICC present.
    if (device.mailBox.length) {
        const msg = device.mailBox[0].d;
        console.log(msg);
        const match = msg.match(/^esim_profile=(\d+)$/);
        if (match) {
            savedIccid = match[1];
        }
    }
});

test('01_esim_disable_all_profiles', async function () {
    // The device resets at the end of this test
    this.test.parent.particle.suiteInitialized = false;
});

test('02_esim_enters_idle', async function () {
});

test('03_esim_idle_survives_apdu_traffic', async function () {
});

test('04_esim_leaves_idle_when_profile_enabled', async function () {
});

test('05_esim_iccid_matches_enabled_profile', async function () {
    if (savedIccid) {
        console.log(`operational profile: ${savedIccid}`);
    }
});

test('99_esim_cleanup', async function () {
});
