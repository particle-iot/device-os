suite('Multiple OTA updates with disabled resets');

platform('gen3');
systemThread('enabled');

const { HalModuleParser, ModuleInfo, updateModulePrefix, updateModuleSuffix, updateModuleCrc32 } = require('binary-version-reader');
const tempy = require('tempy');

const { readFile } = require('fs').promises;
const path = require('path');

let api = null;
let auth = null;
let device = null;
let deviceId = null;

async function flash(ctx, binFile, { timeout = 120000, mayFail = false } = {}) {
	const appName = path.basename(binFile, '.bin');
	await api.flashDevice({ deviceId, files: { [appName]: binFile }, auth });
	const ok = await waitFlashStatusEvent(ctx, timeout);
	if (!ok && !mayFail) {
		throw new Error('Update failed');
	}
	return ok;
}

async function waitFlashStatusEvent(ctx, timeout) {
	let timeoutAt = Date.now() + timeout;
	let data = null;
	for (;;) {
		const t = timeoutAt - Date.now();
		if (t <= 0) {
			throw new Error("Event timeout");
		}
		data = await ctx.particle.receiveEvent('spark/flash/status', { timeout: t });
		ctx.particle.log.verbose('spark/flash/status:', data);
		if (data.startsWith('success')) {
			return true;
		}
		if (data.startsWith('failed')) {
			return false;
		}
	}
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

test('02_flash_binaries', async function () {
	// Get the module binary of the test app that is currently running on the device
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
	updateModuleCrc32(appData);
	let appFile = await tempy.write(appData, { name: 'app1.bin' });
	// The device should accept this update
	await flash(this, appFile);
	await delay(2000);
	// The second binary has an incompatible platform ID and an invalid CRC checksum
	appData = Buffer.from(origAppData);
	suffix = { ...origSuffix };
	suffix.fwUniqueId = 'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb';
	expect(suffix.fwUniqueId).to.not.equal(origSuffix.fwUniqueId);
	updateModuleSuffix(appData, suffix);
	updateModuleCrc32(appData);
	const prefix = { ...origPrefix };
	// The platform ID has all bits set so that the app can overwrite it without erasing the flash
	prefix.platformID = 0xffff;
	updateModulePrefix(appData, prefix);
	appFile = await tempy.write(appData, { name: 'app2.bin' });
	// The device may accept or reject this update depending on the platform and Device OS version.
	// In either case, the previously received update should be invalidated
	await flash(this, appFile, { mayFail: true });
});

test('03_fix_ota_binary_and_reset', async function () {
	// Reset the device so that it can apply pending updates (it shouldn't have any)
	await device.reset();
});

test('04_validate_module_info', async function () {
	// See the test app
});
