suite('00_before: Preparation/Setup for HIL Testing');

platform('gen3', 'gen4');
systemThread('enabled');

const { unsetDeviceVariables } = require('../test/env');
const Particle = require('particle-api-js');

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

test('04_cleanup_env', async function () {
    await unsetDeviceVariables(api, deviceId);
});

after(function() {
    // console.log('after js runs');
});