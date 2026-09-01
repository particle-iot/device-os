suite('00_before: Preparation/Setup for HIL Testing');

platform('gen3', 'gen4');
systemThread('enabled');

const { unsetDeviceVariables } = require('../test/env');
const Particle = require('particle-api-js');

const { waitFlashStatusEvent, flash } = require('../test/ota');
const { readFile } = require('fs').promises;

let device;
let deviceId;
let api;

before(function() {
    api = new Particle({
        baseUrl: this.particle.apiClient.instance.baseUrl, // TODO: Expose as an ApiClient property
        auth: this.particle.apiClient.token
    });

    device = this.particle.devices[0];
    deviceId = device.id;
    device.on('mailbox', (msg) => {
        console.log('mailbox msg', msg);
    });
});

after(function() {
    device.removeAllListeners('mailbox');
});

test('01_ota_self_flash_start', async function () {
    this.timeout(10 * 60 * 1000);
    const appData = await readFile(device.testAppBinFile);
    // Reset device-service OTA flash attempt count by OTA flashing the current app test binary.
    // This should ensure that subsequent OTA downloads (like repeated empty env var asset OTAs) 
    // should succeed for the duration of the test runner suites as long as we do not exceed the
    // device service maxSameBinaryAttempts threshold (defaults to 30).
    await flash(this, appData, { filename: '00_before.bin' });
});

test('02_ota_self_flash_finalize', async function () {

});

test('03_erase_factory_module', async function () {

});

test('04_remove_static_ip', async function () {

});

test('05_enable_listening_mode', async function () {

});

test('06_clear_env', async function () {
    await unsetDeviceVariables(api, deviceId);
});

test('07_restore_cloud_after_env_clear', async function () {

});

test('08_finalize_env_clear', async function () {

});

test('09_disable_external_rtc', async function() {

});

test('10_verify_external_rtc_default_state', async function() {

});

test('11_report_muon_presence', async function() {
    const msg = device.mailBox.pop();
    console.log(`Muon detected: ${msg.d === 'muon=true' ? 'yes' : 'no'}`);
});

test('12_configure_muon_board_and_exrtc', async function() {

});

test('13_verify_muon_exrtc_configuration', async function() {

});

after(function() {
    // console.log('after js runs');
});
