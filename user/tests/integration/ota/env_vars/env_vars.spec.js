suite('Env vars');

platform('gen3', 'gen4');
systemThread('enabled'); // FIXME

const { generateDeviceGroup, setDevelopmentMode, getProductId } = require('../../test/product');
const { flash, waitFlashStatusEvent } = require('../../test/ota');
const { get, post, patch, devicePath, productPath, orgPath } = require('../../test/api');
const { randomString } = require('../../test/random');

const { createEnvVarsAssetModule, createApplicationAndAssetBundle } = require('binary-version-reader');
const Particle = require('particle-api-js');
const tempy = require('tempy');
const _ = require('lodash');

const { readFile } = require('node:fs/promises');

const ORG_ID = 'particle'; // Set to `undefined` to use the current user's sandbox

let testNonce;
let appBinary;
let productId;
let deviceEnvVar;
let deviceGroup;
let deviceId;
let device;
let api;

// Ensures that the default env vars are set at org and product levels. These variables don't change
async function initOrgAndProductEnvVars() {
	const { env } = await get(api, productPath('/env-vars/render', productId));
	if (!('DVOS_CI_ORG_VAR1' in env)) {
		await patch(api, orgPath('/env-vars', ORG_ID), {
			ops: [
				{ op: 'Set', key: 'DVOS_CI_ORG_VAR1', value: 'pcT3RG9xr4' }
			]
		});
		await post(api, orgPath('/env-vars/rollout', ORG_ID), {
			when: 'Connect'
		});
	}
	if (!('DVOS_CI_PROD_VAR1' in env)) {
		await patch(api, productPath('/env-vars', productId), {
			ops: [
				{ op: 'Set', key: 'DVOS_CI_PROD_VAR1', value: 'wkWStqATwW' }
			]
		});
		await post(api, productPath('/env-vars/rollout', productId), {
			when: 'Connect'
		});
	}
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

	deviceGroup = await generateDeviceGroup({ deviceId, api });
	// Generate a unique variable name based on the device's group name
	deviceEnvVar = _.snakeCase('DVOS_' + deviceGroup).toUpperCase(); // "ci_prod_ver-XXXX" -> "DVOS_CI_PROD_VER_XXXX"
	console.log('Device\'s own env var:', deviceEnvVar);

	appBinary = await readFile(device.testAppBinFile);

	await initOrgAndProductEnvVars();
});

test('01_init', async function() {
	expect(device.mailBox).to.not.be.empty;
	testNonce = device.mailBox.shift().d;
	console.log('Test nonce:', testNonce);
});

test('02_start_env_vars_local_update', async function() {
	// XXX: This will update the app env vars on the device even though the test app has no
	// dependency on an env var asset
	const assetData = await createEnvVarsAssetModule({
		APP_VAR1: 'abcdef'
	});
	// TODO: Update Device#flash to optionally take module data instead of a path
	const assetPath = await tempy.write(assetData, { name: 'env_vars.bin' });
	await device.flash(assetPath);
});

test('03_check_env_vars_local_update', async function() {
});

test('04_start_env_vars_on_connect_update', async function() {
	// Snapshot update should start automatically
	await waitFlashStatusEvent(this, { status: 'success' });
});

test('05_complete_env_vars_on_connect_update', async function() {
});

test('06_check_env_vars_on_connect_update', async function() {
});

test('07_start_env_vars_ad_hoc_update', async function() {
	const bundleZip = await createApplicationAndAssetBundle(appBinary, [] /* assets */, {
		APP_VAR1: 'abcde',
		APP_VAR2: '123',
		APP_VAR3: testNonce // Random string
	});
	await flash(this, bundleZip, { filename: 'bundle.zip' });
});

test('08_complete_env_vars_ad_hoc_update', async function() {
});

test('09_check_env_vars_ad_hoc_update', async function() {
});

test('10_start_product_env_vars_immediate_update', async function() {
	await patch(api, productPath('/env-vars', productId), {
		ops: [
			{ op: 'Set', key: deviceEnvVar, value: testNonce + '_prod' }
		]
	});
	await post(api, productPath('/env-vars/rollout', productId), {
		when: 'Immediate'
	});
	await waitFlashStatusEvent(this, { status: 'success' });
});

test('11_complete_product_env_vars_immediate_update', async function() {
});

test('12_check_product_env_vars_immediate_update', async function() {
});

test('13_start_device_env_vars_immediate_update', async function() {
	await patch(api, productPath(`/env-vars/${deviceId}`, productId), {
		ops: [
			{ op: 'Set', key: 'DEV_VAR1', value: testNonce + '_dev' }
		]
	});
	await post(api, devicePath('/env-vars/rollout', deviceId), {
		when: 'Immediate'
	});
	await waitFlashStatusEvent(this, { status: 'success' });
});

test('14_complete_device_env_vars_immediate_update', async function() {
});

test('15_check_device_env_vars_immediate_update', async function() {
});
