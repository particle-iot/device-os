suite('Factory Reset');

platform('argon','boron','bsom','b5som','tracker','esomx');
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

async function flashBinary(ctx, fwUniqueId) {
	// Get the module binary of the test app that is currently running on the device
	const origAppData = await readFile(device.testAppBinFile);
	const parser = new HalModuleParser();
	const { prefixInfo: origPrefix, suffixInfo: origSuffix } = await parser.parseBuffer({ fileBuffer: origAppData });
	// The first binary is exactly the same as the one already flashed on the device but it has a
	// different SHA checksum so that we can identify it later
	let appData = Buffer.from(origAppData);
	let suffix = { ...origSuffix };
	suffix.fwUniqueId = fwUniqueId;
	expect(suffix.fwUniqueId).to.not.equal(origSuffix.fwUniqueId);
	updateModuleSuffix(appData, suffix);
	updateModuleCrc32(appData);
	let appFile = await tempy.write(appData, { name: 'app1.bin' });
	// The device should accept this update
	await flash(ctx, appFile);
	await delay(2000);
}

test('01_disable_resets_and_connect', async function () {
	// See the test app
});

test('02_flash_binary', async function () {
	await flashBinary(this, 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa');
});

test('03_move_ota_binary_to_factory_slot', async function () {
	// See the test app: Copy OTA binary to Factory Reset slot, update DCT to denote valid Factory Reset module present
});

test('04_device_factory_reset', async function () {
	// Test Device firmware initiated factory reset
});

test('05_validate_factory_reset_worked', async function () {
	// See the test app: Verify that `System.factoryReset()` applied the modified factory_test binary
});

// Test USB initiated reset
test('06_disable_resets_and_connect', async function () {
	// See the test app
});

test('07_flash_binary', async function () {
	await flashBinary(this, 'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb');
});

test('08_move_ota_binary_to_factory_slot', async function () {
	// See the test app: Copy OTA binary to Factory Reset slot, update DCT to denote valid Factory Reset module present
});

test('09_usb_command_factory_reset', async function () {
	let usbDevice = await device.getUsbDevice();
	device.setWillDetach(true);
	await usbDevice.factoryReset({ timeout: 10000 });
});

test('10_validate_factory_reset_worked', async function () {
	// See the test app: Verify that `System.factoryReset()` applied the modified factory_test binary
});