suite('Multiple OTA updates without a system reset');

platform('gen2', 'gen3');
systemThread('enabled');

const { HalModuleParser, ModuleInfo, updateModulePrefix, updateModuleSuffix } = require('binary-version-reader');
const tempy = require('tempy');

const { readFile } = require('fs').promises;
const path = require('path');

let api = null;
let auth = null;
let device = null;
let deviceId = null;

async function flash(ctx, binFile, { expectedStatus = 'success' } = {}) {
	const appName = path.basename(binFile, '.bin');
	await api.flashDevice({ deviceId, files: { [appName]: binFile }, auth });
	await waitFlashStatusEvent(ctx, expectedStatus);
	await delay(1000);
}

async function waitFlashStatusEvent(ctx, status) {
	let data = null;
	do {
		data = await ctx.particle.receiveEvent('spark/flash/status');
		ctx.particle.log.verbose('spark/flash/status:', data);
	} while (!data.startsWith(status));
}

async function delay(ms) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

before(function() {
	api = this.particle.apiClient.instance;
	auth = this.particle.apiClient.token;
	device = this.particle.devices[0];
	deviceId = device.id;
});

test('01_disable_resets_and_connect', async function () {
	// See the test app
});

test('02_flash_binaries_and_reset', async function () {
	// Get the module binary of the test application that is currently running on the device
	const origAppData = await readFile(device.testAppBinFile);
	const parser = new HalModuleParser();
	const { prefixInfo: origPrefix, suffixInfo: origSuffix } = await parser.parseBuffer({ fileBuffer: origAppData });
	// The first binary is exactly the same as the one already flashed on the device but it has a
	// different SHA checksum so that we can identify it later
	let appData = Buffer.from(origAppData);
	let suffix = { ...origSuffix };
	suffix.fwUniqueId = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
	expect(suffix.fwUniqueId).to.not.equal(origSuffix.fwUniqueId);
	updateModuleSuffix(appData, suffix);
	let appFile = await tempy.write(appData, { name: 'app1.bin' });
	// The device should accept this update
	await flash(this, appFile);
	// The second binary has an incompatible platform ID
	appData = Buffer.from(origAppData);
	const prefix = { ...origPrefix };
	++prefix.platformID;
	updateModulePrefix(appData, prefix);
	suffix = { ...origSuffix };
	suffix.fwUniqueId = 'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb';
	expect(suffix.fwUniqueId).to.not.equal(origSuffix.fwUniqueId);
	updateModuleSuffix(appData, suffix);
	appFile = await tempy.write(appData, { name: 'app2.bin' });
	// The device should reject this update as well as clear the previously received one
	await flash(this, appFile, { expectedStatus: 'failed' });
	// Reset the device so that it can apply pending updates (it shouldn't have any)
	await device.reset();
});

test('03_validate_module_info', async function () {
	// See the test app
});
