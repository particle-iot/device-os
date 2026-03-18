suite('PRE_STARTUP() compatibility');

platform('gen3', 'gen4');
systemThread('enabled');
const flashTimeoutMinutes = 40;
timeout(flashTimeoutMinutes * 60 * 1000);

const { HalModuleParser, ModuleInfo, compressModule, updateModuleCrc32, updateModulePrefix, updateModuleSuffix, updateModuleSha256 } = require('binary-version-reader');
const tempy = require('tempy');

const { readFile, writeFile } = require('fs').promises;
const path = require('path');

const HAL_USER_MODULE_MIN_VERSION_WITH_PRE_STARTUP = 10;
const TEST_COMPAT_USER_MODULE_VERSION = 6;

let api = null;
let auth = null;
let device = null;
let deviceId = null;
let origAppData = null;
let compatAppData = null;

async function flash(ctx, data, name, { timeout = flashTimeoutMinutes * 60 * 1000, retry = 5 } = {}) {
	let ok = false;
	const timeoutAt = Date.now() + timeout;
	let noDelay = false;
	for (let i = 0; i < retry && !ok; i++) {
		if (!noDelay) {
			await delayMs(i * 5000);
		}
		noDelay = false;
		try {
			console.log(`Flashing ${i}/${retry}`);
			const resp = await api.flashDevice({ deviceId, files: { [name]: data }, auth });
			if (resp.body.ok === false) {
				console.log('Error from API', resp);
				if (resp.body.errors && resp.body.errors.indexOf('Update failed - Another update is in progress') > -1) {
					// Retry
					i = Math.max(0, i - 1);
				}
			}
			let seenStarted = false;
			while (true) {
				const t = timeoutAt - Date.now();
				if (t <= 0) {
					i = retry;
					throw new Error('Flashing timeout exceeded');
				}
				let status = await waitFlashStatusEvent(ctx, seenStarted ? t : 60000);
				console.log(`Flash status event: ${status}`);
				if (status === 'started') {
					seenStarted = true;
				} else if (status === 'success') {
					ok = true;
					break;
				} else if (status === 'failed') {
					break;
				} else if (!seenStarted) {
					break;
				} else if (status === 'online' && seenStarted) {
					noDelay = true;
					break;
				}
			}
		} catch (err) {
			console.log(err.message);
		}
	}
	if (!ok) {
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
		data = null;
		try {
			data = await Promise.race([
				ctx.particle.receiveEvent('spark/flash/status', { timeout: t }),
				ctx.particle.receiveEvent('test/ota', { timeout: t }),
				ctx.particle.receiveEvent('spark/status', { timeout: t })
			]);
		} catch (err) {

		}
		if (data) {
			ctx.particle.log.verbose('(spark/runner)/flash/status:', data);
			if (data.startsWith('success')) {
				return 'success';
			}
			if (data.startsWith('failed')) {
				return 'failed';
			}
			if (data.startsWith('offline')) {
				return 'offline';
			}
			if (data.startsWith('online')) {
				return 'online';
			}
			if (data.startsWith('started')) {
				return 'started';
			}
		}
	}
}

async function delayMs(ms) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

async function generateCompatApp() {
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: origAppData });
	compatAppData = Buffer.alloc(origAppData.length, 0xaa);
	origAppData.copy(compatAppData);
	prefixInfo.moduleVersion = TEST_COMPAT_USER_MODULE_VERSION;
	updateModulePrefix(compatAppData, prefixInfo);
	updateModuleSuffix(compatAppData, suffixInfo);
	updateModuleSha256(compatAppData);
	updateModuleCrc32(compatAppData);
	return compatAppData;
}

before(async function() {
	api = this.particle.apiClient.instance;
	auth = this.particle.apiClient.token;
	device = this.particle.devices[0];
	deviceId = device.id;

	origAppData = await readFile(device.testAppBinFile);
});

async function getDeviceUserFirmwareModuleInfo() {
	const usbDevice = await device.getUsbDevice();
	const modules = await usbDevice.getFirmwareModuleInfo();
	const app = modules.find((v) => v.type === 'USER_PART' && v.store === 'MAIN');
	return app;
}

async function checkCompatApplication() {
	const app = await getDeviceUserFirmwareModuleInfo();
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: compatAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
	expect(prefixInfo.moduleVersion).to.equal(TEST_COMPAT_USER_MODULE_VERSION);
};

async function checkOriginalApplication() {
	const app = await getDeviceUserFirmwareModuleInfo();
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: origAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
	expect(prefixInfo.moduleVersion).to.equal(HAL_USER_MODULE_MIN_VERSION_WITH_PRE_STARTUP);
}

test('01_check_current_application', async function () {
	await checkOriginalApplication();
});

test('02_ota_before_pre_startup_application_start', async function () {
	await generateCompatApp();
	const appFile = await tempy.write(compatAppData, { name: 'compat_app.bin' });
	this.timeout(35 * 60 * 1000);
	await flash(this, appFile);
});

test('03_ota_before_pre_startup_application_wait_1', async function () {
});

test('03_ota_before_pre_startup_application_wait_2', async function () {
});

test('04_check_before_pre_startup_application', async function() {
	await checkCompatApplication();
});

test('05_ota_original_application_start', async function () {
	const appFile = await tempy.write(origAppData, { name: 'orig_app.bin' });
	await flash(this, appFile);
});

test('06_ota_original_application_wait_1', async function () {
});

test('06_ota_original_application_wait_2', async function () {
});

test('07_check_original_application', async function() {
	await checkOriginalApplication();
});

test('99_cleanup', async function () {

});
