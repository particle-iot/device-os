suite('System level env vars');

platform('gen3', 'gen4');

const { createEnvVarsAssetModule } = require('binary-version-reader');
const tempy = require('tempy');
const _ = require('lodash');

const { readFile } = require('node:fs/promises');

const { waitFlashStatusEvent } = require('../../test/ota');

let appBinary;
let deviceId;
let device;

async function setEnvVarsAndFlash(vars) {
    const assetData = await createEnvVarsAssetModule(vars);
    const assetPath = await tempy.write(assetData, { name: 'env_vars.bin' });
	return device.flash(assetPath);
}

before(async function() {
	device = this.particle.devices[0];
	deviceId = device.id;

	appBinary = await readFile(device.testAppBinFile);
});

test('01_particle_ble_enable_init', async function() {
});

test('02_particle_ble_enable_default', async function() {
    await setEnvVarsAndFlash({ PARTICLE_BLUETOOTH_ENABLE: 'true' });
});

test('03_particle_ble_enable_true', async function() {
    await setEnvVarsAndFlash({ PARTICLE_BLUETOOTH_ENABLE: 'false' });
});

test('04_particle_ble_enable_false', async function() {
    await setEnvVarsAndFlash({ PARTICLE_CLOUD_KEEP_ALIVE: '50' });
});

test('05_particle_default_cloud_keepalive', async function() {

});

test('06_particle_wifi_enable_init', async function() {
    // This is a bit of a hack, but should work fine
    this.test.parent.particle.network = 'wifi';
    this.test.parent.particle.suiteInitialized = false;
});

test('07_particle_wifi_enable_default', async function() {
    await setEnvVarsAndFlash({ 
        PARTICLE_WIFI_ENABLE: 'true',
        PARTICLE_WIFI_CLOUD_KEEP_ALIVE: '30',
        PARTICLE_CLOUD_KEEP_ALIVE: '50'
     });
});

test('08_particle_wifi_enable_true', async function() {
    await setEnvVarsAndFlash({ PARTICLE_WIFI_ENABLE: 'false' });
});

test('09_particle_wifi_enable_false', async function() {
    // This is a bit of a hack, but should work fine
    this.test.parent.particle.network = 'any'; // just set to something other than wifi
    this.test.parent.particle.suiteInitialized = false;
});

test('10_particle_wifi_enable_false_connect_through_other_ifaces', async function() {

});

test('11_particle_wifi_enable_cleanup', async function() {
});

test('12_particle_ethernet_enable_init', async function () {
    // This is a bit of a hack, but should work fine
    this.test.parent.particle.network = 'ethernet';
    this.test.parent.particle.suiteInitialized = false;
});

test('13_particle_ethernet_enable_default', async function () {
    await setEnvVarsAndFlash({ 
        PARTICLE_ETHERNET_ENABLE: 'true',
        PARTICLE_ETHERNET_CLOUD_KEEP_ALIVE: '40',
        PARTICLE_CLOUD_KEEP_ALIVE: '50'
    });
});

test('14_particle_ethernet_enable_true', async function () {
    await setEnvVarsAndFlash({ PARTICLE_ETHERNET_ENABLE: 'false' });
});

test('15_particle_ethernet_enable_false', async function () {
    this.test.parent.particle.network = 'any'; // just set to something other than ethernet
    this.test.parent.particle.suiteInitialized = false;
});

test('16_particle_ethernet_enable_false_connect_through_other_ifaces', async function () {

});

test('17_particle_ethernet_enable_cleanup', async function () {

});

test('18_particle_power_env_init', async function() {
});

test('19_particle_power_env_default', async function() {
    await setEnvVarsAndFlash({
        PARTICLE_PMIC_INPUT_CURRENT: '1200',
        PARTICLE_PMIC_CHARGE_CURRENT: '1024'
    });
});

test('20_particle_power_env_override', async function() {
    await setEnvVarsAndFlash({
        PARTICLE_PMIC_INPUT_CURRENT: '1200',
        PARTICLE_PMIC_CHARGE_CURRENT: '1408'
    });
});

test('21_particle_power_env_charge_above_input_limit', async function() {
});

test('22_particle_power_env_restore_init', async function() {
});

test('23_particle_power_env_restore', async function() {
});

test('97_cleanup', async function() {
    delete this.test.parent.particle.network;
    this.test.parent.particle.suiteInitialized = false;
});

test('98_cleanup', async function() {
});
