suite('Cleanup for HIL Testing');

platform('gen3', 'gen4');
systemThread('enabled');

const { unsetDeviceVariables } = require('../test/env');
const Particle = require('particle-api-js');

const { waitFlashStatusEvent } = require('../test/ota');

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
});

test('01_erase_factory_module', async function () {

});

test('02_remove_static_ip', async function () {

});

test('03_enable_listening_mode', async function () {

});

test('04_clear_env', async function () {
    await unsetDeviceVariables(api, deviceId);
});

test('05_restore_cloud_after_env_clear', async function () {
    await waitFlashStatusEvent(this, { status: 'success' });
});

test('06_finalize_env_clear_1', async function () {
});

test('06_finalize_env_clear_2', async function () {
});

test('07_disable_external_rtc', async function () {
});

test('08_verify_external_rtc_default_state', async function () {
});

test('09_unconfigure_muon_board', async function () {
});

test('10_verify_muon_board_unconfigured', async function () {
});

after(function() {
    // console.log('after js runs');
});
