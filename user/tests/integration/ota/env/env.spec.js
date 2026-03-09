suite('Env vars');

platform('gen3', 'gen4');
systemThread('enabled'); // FIXME

const { generateDeviceGroup, setDevelopmentMode, getProductId } = require('../../test/product');
const { flash, waitFlashStatusEvent } = require('../../test/ota');
const { get, post, patch, devicePath, productPath, orgPath } = require('../../test/api');
const { randomString } = require('../../test/random');

const { unsetDeviceVariables } = require('../../test/env');

const { createEnvVarsAssetModule, createApplicationAndAssetBundle } = require('binary-version-reader');
const Particle = require('particle-api-js');
const tempy = require('tempy');
const _ = require('lodash');

const { readFile } = require('node:fs/promises');

const ORG_ID = 'particle-hil'; // Set to `undefined` to use the current user's sandbox

let nonce;
let appBinary;
let productId;
let deviceVar1;
let deviceVar2;
let deviceGroup;
let deviceId;
let device;
let api;

// Ensures that the default env vars are set at org and product levels. These variables don't change
async function initDefaultEnv() {
	// const defaultOrgVars = {
	// 	'DVOS_CI_ORG_VAR1': 'org default 1 pcT3RG9xr4'
	// };
	// const defaultProductVars = {
	// 	'DVOS_CI_PROD_VAR1': 'prod default 1 wkWStqATwW'
	// };

	// // Set default product variables
	// let resp = await get(api, productPath('/env', productId));
	// const ownProductVars = Object.fromEntries(Object.entries(resp.last_snapshot?.own || {}).map(([k, v]) => [k, v.value]));
	// if (!_.isMatch(ownProductVars, defaultProductVars)) {
	// 	await patch(api, productPath('/env', productId), {
	// 		ops: Object.entries(defaultProductVars).map(([key, value]) => ({ op: 'Set', key, value }))
	// 	});
	// 	await post(api, productPath('/env/rollout', productId), {
	// 		when: 'Connect'
	// 	});
	// }

	// // Set default org variables
	// const inheritedProductVars = Object.fromEntries(Object.entries(resp.last_snapshot?.inherited || {}).map(([k, v]) => [k, v.value]));
	// if (!_.isMatch(inheritedProductVars, defaultOrgVars)) {
	// 	await patch(api, orgPath('/env', ORG_ID), {
	// 		ops: Object.entries(defaultOrgVars).map(([key, value]) => ({ op: 'Set', key, value }))
	// 	});
	// 	await post(api, orgPath('/env/rollout', ORG_ID), {
	// 		when: 'Connect'
	// 	});
	// }

	// Unset all device variables
	await unsetDeviceVariables(api, deviceId);
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
	// Generate a couple unique variable names based on the group name
	deviceVar1 = _.snakeCase('DVOS_' + deviceGroup).toUpperCase(); // "ci_prod_ver-XXXX" -> "DVOS_CI_PROD_VER_XXXX"
	deviceVar2 = deviceVar1 + '_2';
	console.log('Device variable 1:', deviceVar1);
	console.log('Device variable 2:', deviceVar2);

	appBinary = await readFile(device.testAppBinFile);

	await initDefaultEnv();
});

test('01_init', async function() {
	expect(device.mailBox).to.not.be.empty;
	nonce = device.mailBox.shift().d;
	console.log('Nonce:', nonce);
	expect(nonce).to.not.be.empty;
});

test('02_start_local_env_update', async function() {
	// This will update the app env vars on the device even though the test app has no dependency
	// on an env asset
	const assetData = await createEnvVarsAssetModule({
		STR_SHORT: 'abc',
		STR_LONG: 'a'.repeat(1000),
		STR_RANDOM: nonce,
		STR_EMPTY: '',
		INT_POSITIVE: '123',
		INT_NEGATIVE: '-456',
		INT_LEADING_ZEROS: '00123',
		INT_ZERO: '0',
		INT_NEGATIVE_ZERO: '-0',
		INT_PLUS_SIGN: '+123',
		INT_MAX: '2147483647',
		INT_MIN: '-2147483648',
		INT_OVERFLOW: '2147483648',
		INT_UNDERFLOW: '-2147483649',
		INT_FLOAT: '12.34',
		INT_LEADING_SPACE: ' 123',
		INT_TRAILING_SPACE: '123 ',
		INT_HEX: '0x1F',
		INT_WITH_CHARS: '123abc',
		BOOL_TRUE: 'true',
		BOOL_FALSE: 'false',
		BOOL_UPPER_TRUE: 'TRUE',
		BOOL_UPPER_FALSE: 'FALSE',
		BOOL_MIXED_TRUE: 'True',
		BOOL_MIXED_FALSE: 'False',
		BOOL_ONE: '1',
		BOOL_ZERO: '0',
		BOOL_YES: 'yes',
		BOOL_NO: 'no',
		BOOL_LEADING_SPACE: ' false',
		BOOL_TRAILING_SPACE: 'false '
	});
	// TODO: Update Device#flash to optionally take module data instead of a path
	const assetPath = await tempy.write(assetData, { name: 'env_vars.bin' });
	await device.flash(assetPath);
});

test('03_check_local_env_update', async function() {
});

test('04_prepare_on_connect_env_update', async function() {
	await patch(api, `/v1/products/${productId}/env/${deviceId}`, {
		ops: [
			{ op: 'Set', key: 'DEV_VAR2', value: 'dev 22 ' + nonce },
			{ op: 'Set', key: 'DEV_VAR1', value: 'dev 11 ' + nonce }
		]
	});
	await post(api, `/v1/env/${deviceId}/rollout`, {
		when: 'Connect'
	});
});

test('05_start_on_connect_env_update', async function() {
	// Snapshot update should start automatically
	await waitFlashStatusEvent(this, { status: 'success' });
});

test('06_complete_on_connect_env_update', async function() {
});

test('07_check_on_connect_env_update', async function() {
});

test('08_start_ad_hoc_env_update', async function() {
	const bundleZip = await createApplicationAndAssetBundle(appBinary, [], {
		'APP_VAR1': 'app 1 ' + nonce,
		'APP_VAR2': 'app 2 ' + nonce
	});
	await flash(this, bundleZip, { filename: 'bundle.zip' });
});

test('09_complete_ad_hoc_env_update', async function() {
});

test('10_check_ad_hoc_env_update', async function() {
});

test('11_start_immediate_device_env_update', async function() {
	await patch(api, `/v1/products/${productId}/env/${deviceId}`, {
		ops: [
			// Override app variables
			{ op: 'Set', key: 'APP_VAR2', value: 'dev app 2 ' + nonce },
			// Set device variables
			{ op: 'Set', key: 'DEV_VAR1', value: 'dev 1 ' + nonce },
			{ op: 'Set', key: 'DEV_VAR2', value: 'dev 2 ' + nonce },
		]
	});
	await post(api, `/v1/env/${deviceId}/rollout`, {
		when: 'Immediate'
	});
	await waitFlashStatusEvent(this, { status: 'success' });
});

test('12_complete_immediate_device_env_update', async function() {
});

test('13_check_immediate_device_env_update', async function() {
});

test('97_cleanup', async function() {
	await unsetDeviceVariables(api, deviceId);
});

test('98_cleanup', async function() {
	await waitFlashStatusEvent(this, { status: 'success' });
});

test('99_cleanup', async function() {
	
});