suite('Env vars');

platform('gen3', 'gen4');
// systemThread('enabled');

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
	const orgVars = {
		'DVOS_CI_ORG_VAR1': 'org default 1 pcT3RG9xr4'
	};
	const productVars = {
		'DVOS_CI_PROD_VAR1': 'prod default 1 wkWStqATwW'
	};

	let resp = await get(api, productPath('/env', productId));
	let env = resp.last_snapshot?.rendered;
	if (!_.isMatch(env, orgVars)) {
		await patch(api, orgPath('/env', ORG_ID), {
			ops: Object.entries(orgVars).map(([key, value]) => ({ op: 'Set', key, value }))
		});
		await post(api, orgPath('/env/rollout', ORG_ID), {
			when: 'Connect'
		});
	}
	if (!_.isMatch(env, productVars)) {
		await patch(api, productPath('/env', productId), {
			ops: Object.entries(productVars).map(([key, value]) => ({ op: 'Set', key, value }))
		});
		await post(api, productPath('/env/rollout', productId), {
			when: 'Connect'
		});
	}

	// Unset all variables at device level
	resp = await get(api, `/v1/products/${productId}/env/${deviceId}`);
	env = resp.env?.own;
	if (!_.isEmpty(env)) {
		await patch(api, `/v1/products/${productId}/env/${deviceId}`, {
			ops: Object.entries(env).map(([key]) => ({ op: 'Unset', key }))
		});
		await post(api, `/v1/env/${deviceId}/rollout`, {
			when: 'Immediate'
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
	// Generate a few unique variable names based on the group name
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
	// XXX: This will update the app env vars on the device even though the test app has no
	// dependency on an env var asset
	const assetData = await createEnvVarsAssetModule({
		// Basic string and numeric values
		APP_VAR1: 'abcdef',
		APP_VAR2: '123',
		APP_VAR3: '0',
		APP_VAR4: 'true',
		APP_VAR5: 'false',
		// Bool case sensitivity tests (all invalid - only lowercase "true"/"false" valid)
		BOOL_UPPER_TRUE: 'TRUE',
		BOOL_UPPER_FALSE: 'FALSE',
		BOOL_MIXED_TRUE: 'True',
		BOOL_MIXED_FALSE: 'False',
		// Bool invalid values (not "true" or "false")
		BOOL_ONE: '1',
		BOOL_ZERO: '0',
		BOOL_YES: 'yes',
		BOOL_NO: 'no',
		BOOL_LEADING_SPACE: ' true',
		BOOL_TRAILING_SPACE: 'true ',
		// Int edge cases
		INT_NEGATIVE: '-456',
		INT_LEADING_ZEROS: '00123',
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
		// Empty value test
		EMPTY_VAR: '',
		// This is to ensure the resulting asset module gets a unique checksum
		RAND_VAR: nonce
	});
	// TODO: Update Device#flash to optionally take module data instead of a path
	const assetPath = await tempy.write(assetData, { name: 'env_vars.bin' });
	await device.flash(assetPath);
});

test('03_check_local_env_update', async function() {
});

test('04_start_on_connect_env_update', async function() {
	// Snapshot update should start automatically
	await waitFlashStatusEvent(this, { status: 'success' });
});

test('05_complete_on_connect_env_update', async function() {
});

test('06_check_on_connect_env_update', async function() {
});

test('07_start_ad_hoc_env_update', async function() {
	const bundleZip = await createApplicationAndAssetBundle(appBinary, [], {
		'APP_VAR1': 'app 1 ' + nonce,
		'APP_VAR2': 'app 2 ' + nonce,
		// These are saved to the device but don't override the snapshot variables
		'DVOS_CI_ORG_VAR1': 'app org 1 ' + nonce,
		'DVOS_CI_PROD_VAR1': 'app prod 1 ' + nonce
	});
	await flash(this, bundleZip, { filename: 'bundle.zip' });
});

test('08_complete_ad_hoc_env_update', async function() {
});

test('09_check_ad_hoc_env_update', async function() {
});

test('10_start_immediate_product_env_update', async function() {
	await patch(api, productPath('/env', productId), {
		ops: [
			{ op: 'Set', key: deviceVar1, value: 'prod 1 ' + nonce },
			{ op: 'Set', key: deviceVar2, value: 'prod 2 ' + nonce }
		]
	});
	await post(api, productPath('/env/rollout', productId), {
		when: 'Immediate'
	});
	await waitFlashStatusEvent(this, { status: 'success' });
});

test('11_complete_immediate_product_env_update', async function() {
});

test('12_check_immediate_product_env_update', async function() {
});

test('13_start_immediate_device_env_update', async function() {
	await patch(api, `/v1/products/${productId}/env/${deviceId}`, {
		ops: [
			// Override app variables
			{ op: 'Set', key: 'APP_VAR2', value: 'dev app 2 ' + nonce },
			// Override product variables
			{ op: 'Set', key: deviceVar2, value: 'dev prod 2 ' + nonce },
			// Override org variables
			{ op: 'Set', key: 'DVOS_CI_ORG_VAR1', value: 'dev org 1 ' + nonce },
			// Set device variables
			{ op: 'Set', key: 'DEV_VAR1', value: 'dev 1 ' + nonce }
		]
	});
	await post(api, `/v1/env/${deviceId}/rollout`, {
		when: 'Immediate'
	});
	await waitFlashStatusEvent(this, { status: 'success' });
});

test('14_complete_immediate_device_env_update', async function() {
});

test('15_check_immediate_device_env_update', async function() {
});
