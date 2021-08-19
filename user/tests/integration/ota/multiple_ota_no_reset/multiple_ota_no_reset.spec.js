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

async function waitFlashSuccessEvent(ctx) {
	let data = null;
	do {
		data = await ctx.particle.receiveEvent('spark/flash/status');
		ctx.particle.log.verbose('spark/flash/status:', data);
	} while (!data.startsWith('success'));
}

async function flash(ctx, binFile) {
	const appName = path.basename(binFile, '.bin');
	await api.flashDevice({ deviceId, files: { [appName]: binFile }, auth });
	await waitFlashSuccessEvent(ctx);
}

before(function() {
	api = this.particle.apiClient.instance;
	auth = this.particle.apiClient.token;
	device = this.particle.devices[0];
	deviceId = device.id;
});

test('01_disable_resets_and_connect', () => {
	// See the test app
});

test('02_flash_binaries', async function () {
	// Get the module binary of the test application that is currently running on the device
	const origAppData = await readFile(device.testAppBinFile);
	const parser = new HalModuleParser();
	const { prefixInfo: origPrefix, suffixInfo: origSuffix } = await parser.parseBuffer({ fileBuffer: origAppData });
	// The first binary is exactly the same as the one already flashed on the device but with a
	// different SHA checksum
	let appData = Buffer.from(origAppData);
	let suffix = { ...origSuffix };
	suffix.fwUniqueId = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa';
	expect(suffix.fwUniqueId).to.not.equal(origSuffix.fwUniqueId);
	updateModuleSuffix(appData, suffix);
	const appFile1 = await tempy.write(appData, { name: 'app1.bin' });
	// The second binary has a missing dependency
	appData = Buffer.from(origAppData);
	const prefix = { ...origPrefix };
	expect(prefix.depModuleFunction).to.equal(ModuleInfo.FunctionType.SYSTEM_PART);
	++prefix.depModuleVersion;
	updateModulePrefix(appData, prefix);
	suffix = { ...origSuffix };
	suffix.fwUniqueId = 'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb';
	expect(suffix.fwUniqueId).to.not.equal(origSuffix.fwUniqueId);
	updateModuleSuffix(appData, suffix);
	const appFile2 = await tempy.write(appData, { name: 'app2.bin' });
	// Flash the binaries to the device
	await flash(this, appFile1);
	await flash(this, appFile2);
});
