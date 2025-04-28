suite('Minimum and maximum app size OTA');

platform('gen3', 'gen4');
systemThread('enabled');
const flashTimeoutMinutes = 40;
timeout(flashTimeoutMinutes * 60 * 1000);

const { HalModuleParser, ModuleInfo, compressModule, updateModuleCrc32, updateModulePrefix, updateModuleSuffix, updateModuleSha256 } = require('binary-version-reader');
const tempy = require('tempy');

const { readFile, writeFile } = require('fs').promises;
const path = require('path');

let api = null;
let auth = null;
let device = null;
let deviceId = null;
let origAppData = null;
let maxAppData = null;
let maxAppSize = 0;

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

async function generateMaxApp() {
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: origAppData });
	maxAppData = Buffer.alloc(maxAppSize, 0xaa);
	let growLeft = false;
	if (suffixInfo.extensions && suffixInfo.extensions.find((v) => v.type === ModuleInfo.ModuleInfoExtension.DYNAMIC_LOCATION)) {
		// TODO: look into device constants instead
		growLeft = true;
	}
	const padSize = maxAppData.length - origAppData.length;
	if (growLeft) {
		// Pad after prefix before code
		// Copy code + suffix
		origAppData.copy(maxAppData, prefixInfo.prefixSize + padSize, prefixInfo.prefixSize);
		// Copy prefix
		origAppData.copy(maxAppData, 0, 0, prefixInfo.prefixSize);
		prefixInfo.moduleStartAddy = parseInt(prefixInfo.moduleStartAddy, 16) - padSize;
		const ext = suffixInfo.extensions.find((v) => v.type === ModuleInfo.ModuleInfoExtension.DYNAMIC_LOCATION);
		delete ext.data;
		ext.moduleStartAddress = prefixInfo.moduleStartAddy;
	} else {
		// Pad before suffix after code
		// Copy prefix + code
		origAppData.copy(maxAppData, 0, 0, origAppData.length - suffixInfo.suffixSize - 4);
		// Copy suffix
		origAppData.copy(maxAppData, maxAppData.length - suffixInfo.suffixSize - 4, origAppData.length - suffixInfo.suffixSize - 4);
		prefixInfo.moduleEndAddy = parseInt(prefixInfo.moduleEndAddy, 16) + padSize;
	}
	updateModulePrefix(maxAppData, prefixInfo);
	updateModuleSuffix(maxAppData, suffixInfo);
	updateModuleSha256(maxAppData);
	updateModuleCrc32(maxAppData);
	return maxAppData;
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

async function checkCurrentApplication() {
	const app = await getDeviceUserFirmwareModuleInfo();
	expect(app).to.not.be.undefined;
	maxAppSize = app.maxSize;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: origAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
}

async function checkMaxApplication() {
	const app = await getDeviceUserFirmwareModuleInfo();
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: maxAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
	expect(app.size).to.equal(app.maxSize);
};

async function checkOriginalApplication() {
	const app = await getDeviceUserFirmwareModuleInfo();
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: origAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
	expect(app.size).to.not.equal(app.maxSize);
}

test('01_check_current_application', async function () {
	await checkCurrentApplication();
});

test('02_ota_max_application_start', async function () {
	expect(maxAppSize).to.not.equal(0);
	await generateMaxApp();
	expect(maxAppData.length).to.equal(maxAppSize);
	const appFile = await tempy.write(maxAppData, { name: 'max_app.bin' });
	this.timeout(35 * 60 * 1000);
	await flash(this, appFile);
});

test('03_ota_max_application_wait', async function () {
});

test('04_check_max_application', async function() {
	await checkMaxApplication();
});

test('05_ota_original_application_start', async function () {
	const appFile = await tempy.write(origAppData, { name: 'orig_app.bin' });
	await flash(this, appFile);
});

test('06_ota_original_application_wait', async function () {
});

test('07_check_original_application', async function() {
	await checkOriginalApplication();
});

test('08_ota_max_application_compressed_start', async function () {
	expect(maxAppSize).to.not.equal(0);
	await generateMaxApp();
	expect(maxAppData.length).to.equal(maxAppSize);
	const appFile = await tempy.write(maxAppData, { name: 'max_app.bin' });
	await flash(this, appFile);
});

test('09_ota_max_application_compressed_wait', async function () {
});

test('10_check_max_application', async function() {
	await checkMaxApplication();
});

test('11_ota_original_application_compressed_start', async function () {
	const appFile = await tempy.write(origAppData, { name: 'orig_app.bin' });
	await flash(this, appFile);
});

test('12_ota_original_application_compressed_wait', async function () {
});

test('13_check_original_application', async function() {
	await checkOriginalApplication();
});

test('14_usb_flash_max_application_start', async function () {
	expect(maxAppSize).to.not.equal(0);
	expect(maxAppData.length).to.equal(maxAppSize);
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(maxAppData, { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('15_usb_flash_max_application_wait', async function () {
});

test('16_check_max_application', async function() {
	await checkMaxApplication();
});

test('17_usb_flash_original_application_start', async function () {
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(origAppData, { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('18_usb_flash_original_application_wait', async function () {
});

test('19_check_original_application', async function() {
	await checkOriginalApplication();
});

test('20_usb_flash_max_application_compressed_start', async function () {
	expect(maxAppSize).to.not.equal(0);
	expect(maxAppData.length).to.equal(maxAppSize);
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(await compressModule(maxAppData), { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('21_usb_flash_max_application_compressed_wait', async function () {
});

test('22_check_max_application', async function() {
	await checkMaxApplication();
});

test('23_usb_flash_original_application_compressed_start', async function () {
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(await compressModule(origAppData), { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('24_usb_flash_original_application_compressed_wait', async function () {
});

test('25_check_original_application', async function() {
	await checkOriginalApplication();
});

test('26_ota_max_application_busy_start', async function () {
	expect(maxAppSize).to.not.equal(0);
	await generateMaxApp();
	expect(maxAppData.length).to.equal(maxAppSize);
	const appFile = await tempy.write(maxAppData, { name: 'max_app.bin' });
	this.timeout(flashTimeoutMinutes * 60 * 1000);
	await flash(this, appFile);
});

test('27_ota_max_application_busy_wait', async function () {
});

test('28_check_max_application', async function() {
	await checkMaxApplication();
});

test('29_ota_original_application_busy_start', async function () {
	const appFile = await tempy.write(origAppData, { name: 'orig_app.bin' });
	await flash(this, appFile);
});

test('30_ota_original_application_busy_wait', async function () {
});

test('31_check_original_application', async function() {
	await checkOriginalApplication();
});

test('32_usb_flash_max_application_busy_start', async function () {
	expect(maxAppSize).to.not.equal(0);
	expect(maxAppData.length).to.equal(maxAppSize);
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(maxAppData, { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('33_usb_flash_max_application_busy_wait', async function () {
});

test('34_check_max_application', async function() {
	await checkMaxApplication();
});

test('35_usb_flash_original_application_busy_start', async function () {
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(origAppData, { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('36_usb_flash_original_application_busy_wait', async function () {
});

test('37_check_original_application', async function() {
	await checkOriginalApplication();
});
