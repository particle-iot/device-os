suite('Env vars');

platform('gen3', 'gen4');
systemThread('enabled'); // FIXME

const { generateDeviceGroup, setDevelopmentMode, getProductId } = require('../../test/product');
const { flashDevice } = require('../../test/ota');
const { randomString } = require('../../test/random');

const { createApplicationAndAssetBundle } = require('binary-version-reader');
const Particle = require('particle-api-js');

const { readFile } = require('node:fs/promises');

const ORG_ID = 'particle'; // Set to `undefined` to use the sandbox

let productId;
let deviceGroup;
let deviceId;
let device;
let api;

before(async function() {
	api = new Particle({
		baseUrl: this.particle.apiClient.instance.baseUrl, // TODO: Expose as an ApiClient property
		auth: this.particle.apiClient.token
	});
	device = this.particle.devices[0];
	deviceId = device.id;
	productId = await getProductId({ deviceId, api });
	await setDevelopmentMode({ deviceId, api });
	deviceGroup = await generateDeviceGroup({ deviceId, api });
});

test('01_clear_and_reset', async function() {
});

test('02_init_and_connect', async function() {
});

test('03_start_app_env_vars_update', async function() {
	const appBin = await readFile(device.testAppBinFile);
	const randomStr = randomString(30);
	const bundleZip = await createApplicationAndAssetBundle(appBin, [] /* assets */, {
		APP_VAR1: 'abcdef',
		APP_VAR2: '123',
		APP_VAR3: '0',
		APP_VAR4: 'true',
		APP_VAR5: 'false',
		// This is to ensure the resulting asset module gets a unique checksum and cause the device
		// to enter safe mode even if it happens to have an asset with env vars from a prior run of
		// this test
		RAND_VAR1: randomStr,
		RAND_VAR2: randomStr
	});
	await flashDevice(this, bundleZip, { filename: 'bundle.zip' });
});

test('04_complete_app_env_vars_update', async function() {
});

test('05_check_app_env_vars_update', async function() {
});
