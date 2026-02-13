suite('System level env vars');

platform('gen3', 'gen4');
systemMode('manual');

const { setDevelopmentMode, getProductId } = require('../../test/product');
const { get, post, patch } = require('../../test/api');

const { createEnvVarsAssetModule } = require('binary-version-reader');
const Particle = require('particle-api-js');
const tempy = require('tempy');
const _ = require('lodash');

const { readFile } = require('node:fs/promises');

let nonce;
let appBinary;
let deviceId;
let productId;
let device;
let api;

// Ensures that the default env vars are set at org and product levels. These variables don't change
async function initDefaultEnv() {
	// Unset all variables at device level
	resp = await get(api, `/v1/products/${productId}/env/${deviceId}`);
	env = resp.env?.own;
	if (!_.isEmpty(env)) {
		await patch(api, `/v1/products/${productId}/env/${deviceId}`, {
			ops: Object.entries(env).map(([key]) => ({ op: 'Unset', key }))
		});
		await post(api, `/v1/env/${deviceId}/rollout`, {
			when: 'Connect'
		});
	}
}

async function setEnvVarsAndFlash(vars) {
    const assetData = await createEnvVarsAssetModule(vars);
    const assetPath = await tempy.write(assetData, { name: 'env_vars.bin' });
	return device.flash(assetPath);
}

before(async function() {
	api = new Particle({
		baseUrl: this.particle.apiClient.instance.baseUrl, // TODO: Expose as an ApiClient property
		auth: this.particle.apiClient.token
	});

	device = this.particle.devices[0];
	deviceId = device.id;
	productId = await getProductId({ deviceId, api });

	await setDevelopmentMode({ deviceId, api }); // Env vars updates still work in development mode

	appBinary = await readFile(device.testAppBinFile);

	await initDefaultEnv();
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
});

test('05_particle_wifi_enable_init', async function() {
    // This is a bit of a hack, but should work fine
    this.test.parent.particle.network = 'wifi';
    this.test.parent.particle.suiteInitialized = false;
});

test('06_particle_wifi_enable_default', async function() {
    await setEnvVarsAndFlash({ PARTICLE_WIFI_ENABLE: 'true' });
});

test('07_particle_wifi_enable_true', async function() {
    await setEnvVarsAndFlash({ PARTICLE_WIFI_ENABLE: 'false' });
});

test('08_particle_wifi_enable_false', async function() {
    // This is a bit of a hack, but should work fine
    // NOTE: runs after on-device test
    delete this.test.parent.particle.network;
    this.test.parent.particle.suiteInitialized = false;
});

test('09_particle_wifi_enable_false_connect_through_other_ifaces', async function() {

});

test('10_particle_wifi_enable_cleanup', async function() {

});

test('11_particle_ethernet_enable_init', async function () {
    // This is a bit of a hack, but should work fine
    this.test.parent.particle.network = 'ethernet';
    this.test.parent.particle.suiteInitialized = false;
});

test('12_particle_ethernet_enable_default', async function () {
    await setEnvVarsAndFlash({ PARTICLE_ETHERNET_ENABLE: 'true' });
});

test('13_particle_ethernet_enable_true', async function () {
    await setEnvVarsAndFlash({ PARTICLE_ETHERNET_ENABLE: 'false' });
});

test('14_particle_ethernet_enable_false', async function () {
    delete this.test.parent.particle.network;
    this.test.parent.particle.suiteInitialized = false;
});

test('15_particle_ethernet_enable_false_connect_through_other_ifaces', async function () {

});

test('16_particle_ethernet_enable_cleanup', async function () {

});


test('99_cleanup', async function() {

});
