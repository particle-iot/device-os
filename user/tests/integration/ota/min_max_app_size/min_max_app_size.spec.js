suite('Minimum and maximum app size OTA');

platform('gen3');
systemThread('enabled');

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

async function flash(ctx, data, name, { timeout = 5 * 60 * 1000, retry = 5 } = {}) {
	let ok = false;
	for (let i = 0; i < retry; i++) {
		await delayMs(i * 5000);
		try {
			await api.flashDevice({ deviceId, files: { [name]: data }, auth });
			ok = await waitFlashStatusEvent(ctx, timeout);
			if (ok) {
				return ok;
			} else {
				continue;
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

test('01_check_current_application', async function () {
	const usbDevice = await device.getUsbDevice();
	const modules = await usbDevice.getFirmwareModuleInfo();
	const app = modules.find((v) => v.type === 'USER_PART' && v.store === 'MAIN');
	expect(app).to.not.be.undefined;
	maxAppSize = app.maxSize;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: origAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
});

test('02_ota_max_application_start', async function () {
	expect(maxAppSize).to.not.equal(0);
	await generateMaxApp();
	expect(maxAppData.length).to.equal(maxAppSize);
	const appFile = await tempy.write(maxAppData, { name: 'max_app.bin' });
	await flash(this, appFile);
});

test('03_ota_max_application_wait', async function () {
});

test('04_check_max_application', async function() {
	const usbDevice = await device.getUsbDevice();
	const modules = await usbDevice.getFirmwareModuleInfo();
	const app = modules.find((v) => v.type === 'USER_PART' && v.store === 'MAIN');
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: maxAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
	expect(app.size).to.equal(app.maxSize);
});

test('05_ota_original_application_start', async function () {
	const appFile = await tempy.write(origAppData, { name: 'orig_app.bin' });
	await flash(this, appFile);
});

test('06_ota_original_application_wait', async function () {
});

test('07_check_original_application', async function() {
	const usbDevice = await device.getUsbDevice();
	const modules = await usbDevice.getFirmwareModuleInfo();
	const app = modules.find((v) => v.type === 'USER_PART' && v.store === 'MAIN');
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: origAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
	expect(app.size).to.not.equal(app.maxSize);
});

test('08_usb_flash_max_application_start', async function () {
	expect(maxAppSize).to.not.equal(0);
	expect(maxAppData.length).to.equal(maxAppSize);
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(maxAppData, { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('09_usb_flash_max_application_wait', async function () {
});

test('10_check_max_application', async function() {
	const usbDevice = await device.getUsbDevice();
	const modules = await usbDevice.getFirmwareModuleInfo();
	const app = modules.find((v) => v.type === 'USER_PART' && v.store === 'MAIN');
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: maxAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
	expect(app.size).to.equal(app.maxSize);
});

test('11_usb_flash_original_application_start', async function () {
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(origAppData, { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('12_usb_flash_original_application_wait', async function () {
});

test('13_check_original_application', async function() {
	const usbDevice = await device.getUsbDevice();
	const modules = await usbDevice.getFirmwareModuleInfo();
	const app = modules.find((v) => v.type === 'USER_PART' && v.store === 'MAIN');
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: origAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
});

test('14_usb_flash_max_application_compressed_start', async function () {
	expect(maxAppSize).to.not.equal(0);
	expect(maxAppData.length).to.equal(maxAppSize);
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(await compressModule(maxAppData), { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('15_usb_flash_max_application_compressed_wait', async function () {
});

test('16_check_max_application', async function() {
	const usbDevice = await device.getUsbDevice();
	const modules = await usbDevice.getFirmwareModuleInfo();
	const app = modules.find((v) => v.type === 'USER_PART' && v.store === 'MAIN');
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: maxAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
	expect(app.size).to.equal(app.maxSize);
});

test('17_usb_flash_original_application_compressed_start', async function () {
	const usbDevice = await device.getUsbDevice();
	await usbDevice.updateFirmware(await compressModule(origAppData), { timeout: 10 * 60 * 1000 /* Fails with default with old particle-usb versions */});
});

test('18_usb_flash_original_application_compressed_wait', async function () {
});

test('19_check_original_application', async function() {
	const usbDevice = await device.getUsbDevice();
	const modules = await usbDevice.getFirmwareModuleInfo();
	const app = modules.find((v) => v.type === 'USER_PART' && v.store === 'MAIN');
	expect(app).to.not.be.undefined;
	const parser = new HalModuleParser();
	const { prefixInfo, suffixInfo } = await parser.parseBuffer({ fileBuffer: origAppData });
	expect(suffixInfo.fwUniqueId).to.equal(app.hash);
	expect(parseInt(prefixInfo.moduleEndAddy, 16) - parseInt(prefixInfo.moduleStartAddy, 16) + 4 /* CRC32 */).to.equal(app.size);
});
