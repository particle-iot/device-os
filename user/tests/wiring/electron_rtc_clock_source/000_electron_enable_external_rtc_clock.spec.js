suite('Electron enable external RTC clock (LSE)');

platform('electron');
systemThread('enabled');

let device = null;

before(function() {
    device = this.particle.devices[0];
});

test('01_connect_set_feature_reset', async function() {
    await device.reset();
});

test('02_validate', async function() {
});
