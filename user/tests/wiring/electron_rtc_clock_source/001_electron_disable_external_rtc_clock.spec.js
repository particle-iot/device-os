suite('Electron disable external RTC clock (LSE)');

platform('electron');
systemThread('enabled');

// This tag should be filtered out by default
tag('electron_disable_external_rtc_clock');

let device = null;

before(function() {
    device = this.particle.devices[0];
});

test('01_connect_set_feature_reset', async function() {
    await device.reset();
});

test('02_validate', async function() {
});
