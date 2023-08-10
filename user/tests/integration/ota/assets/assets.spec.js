suite('Assets OTA')

platform('gen3');

// Some platforms have pretty slow connectivity
timeout(30 * 60 * 1000);

const { createApplicationAndAssetBundle, unpackApplicationAndAssetBundle, config: binaryVersionReaderConfig,
		createAssetModule, HalModuleParser, ModuleInfo, updateModuleSuffix, updateModuleCrc32, updateModuleSha256 } = require('binary-version-reader');
const { randomBytes } = require('crypto');
const { readFile, writeFile } = require('fs').promises;

const chaiExclude = require('chai-exclude');
chai.use(chaiExclude);

let api = null;
let auth = null;
let device = null;
let deviceId = null;
let product = null;
let assets = [];
let bundle = null;
let timestamp = 0;
let productVersion = 0;

async function delayMs(ms) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

async function checkDeviceInProductAndMarkAsDevelopment() {
	let dev = await api.getDevice({ deviceId, auth });
	expect(dev.body.product_id).to.be.above(0);
	product = dev.body.product_id;
	if (!dev.body.development) {
		await api.updateDevice({ deviceId, auth, development: true, flash: false, product: dev.body.product_id });
		dev = await api.getDevice({ deviceId, auth });
		expect(dev.body.development).to.be.true;
	}
}

function randomInt(min, max) {
	return Math.floor(Math.random() * (max - min + 1)) + min;
}

async function generateAsset(maxCompressedSize, name) {
	let size = (maxCompressedSize * 2) | 0;
	while (size > 0) {
		const data = Buffer.from((await randomBytes(size)).toString('hex'));
		const module = await createAssetModule(data, name, { compress: true });
		if (module.length <= maxCompressedSize) {
			const parsed = await new HalModuleParser().parseBuffer({ fileBuffer: module });
			const hash = [].concat(parsed.prefixInfo.extensions || [])
				.concat(parsed.suffixInfo.extensions || [])
				.filter(ext => ext.type === ModuleInfo.ModuleInfoExtension.HASH);
			return { data, originalSize: data.length, size: module.length, name, crc: binaryVersionReaderConfig().crc32(data), hash: hash[0].hash };
		}
		size *= 0.95;
		size = size | 0;
	}
}

async function generateAssets() {
	const TOTAL_ASSETS_SIZE = 1 * 1024 * 1024;
	const TOTAL_ASSETS_MIN = 3;
	const TOTAL_ASSETS_MAX = 10;
	const MAX_ASSET_SIZE = 500 * 1024;
	const MIN_ASSET_SIZE = 1 * 1024;

	assets.length = 0;

	timestamp = Date.now();

	const assetsNum = randomInt(TOTAL_ASSETS_MIN, TOTAL_ASSETS_MAX);
	let totalSize = 0;
	for (let i = 0; i < assetsNum && totalSize < TOTAL_ASSETS_SIZE; i++) {
		const size = Math.min(randomInt(MIN_ASSET_SIZE, MAX_ASSET_SIZE), TOTAL_ASSETS_SIZE - totalSize);
		const name = `asset${i}_${timestamp}.bin`;
		const asset = await generateAsset(size, name);
		if (!asset) {
			break;
		}
		totalSize += asset.size;
		assets.push(asset);
	}

	let application = await readFile(device.testAppBinFile);
	const parsed = await new HalModuleParser().parseBuffer({ fileBuffer: application });
	parsed.suffixInfo.productVersion = productVersion;
	parsed.suffixInfo.productId = device.platform.id;
	updateModuleSuffix(application, parsed.suffixInfo);
	updateModuleSha256(application);
	updateModuleCrc32(application);
	bundle = await createApplicationAndAssetBundle(application, assets);

	console.log('Generated new set of assets');
	console.dir([...assets].map((val) => { let v = {...val}; delete v.data; return v; }), { depth: null });
	await writeFile(`/tmp/ota_assets_${timestamp}_bundle.zip`, bundle);
}

async function uploadProductFirmware(removeOnly) {
	const fws = await api.listProductFirmware({ product, auth });
	for (let fw of fws.body) {
		if (Number(fw.version) === Number(productVersion)) {
			await api.delete({ uri: `/v1/products/${product}/firmware/${fw.version}`, auth });
			break;
		}
	}
	if (removeOnly) {
		return;
	}

	await api.request({ uri: `/v1/products/${product}/firmware`, method: 'post', auth,
		form: {
			version: productVersion,
			title: `v${productVersion}_device_os_test`,
			description: 'Device OS Test Runner (ota/assets) test'
		},
		files: {
			[`bundle_${timestamp}.zip`]: bundle
		}
	});
}

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
		console.log('spark/flash/status:', data);
		if (data.startsWith('success')) {
			return true;
		}
		if (data.startsWith('failed')) {
			return false;
		}
	}
}

async function waitForAssets(timeout) {
	const timeoutAt = Date.now() + timeout;
	for (;;) {
		const t = timeoutAt - Date.now();
		if (t <= 0) {
			throw new Error("Timeout waiting for all assets to be reported from device");
		}
		let dev = null;
		try {
			dev = await api.getDevice({ deviceId, auth });
		} catch (err) {
			// Ignore
			continue;
		}
		if (!dev.body.assets || dev.body.assets.length === 0) {
			continue;
		}
		let missing = 0;
		for (let asset of dev.body.assets) {
			if (asset.status === 'missing') {
				missing++;
			}
		}
		if (missing === 0) {
			return dev.body.assets;
		}
	}
}

function generatedAssetsToReport() {
	return assets.map((val) => { return {
		name: val.name,
		size: val.originalSize,
		storageSize: val.size,
		valid: true,
		readable: true,
		hash: val.hash,
		crc: val.crc.readUInt32BE(0),
		error: 0
	}});
}

function cloudReportedToReport(rep) {
	return rep.map((val) => { return {
		name: val.name,
		size: 0,
		storageSize: 0,
		valid: val.status === 'available',
		readable: val.status === 'available',
		hash: '',
		crc: 0,
		error: 0
	}});
}


async function generateProductVersion() {
	if (productVersion) {
		return productVersion;
	}

	const dev = await api.getDevice({ deviceId, auth });
	let version = null;
	for (let group of dev.body.groups) {
		if (group.startsWith('ci_prod_ver-')) {
			version = group.split('-')[1];
		}
	}

	if (!version) {
		version = randomInt(1, 0xfffe);
	}

	for (;; version = randomInt(1, 0xfffe)) {
		console.log(`try version ${version}`);
		let devs = await api.listDevices({ auth, product, groups: [`ci_prod_ver-${version}`] });
		if (devs.body.devices.length === 1 && devs.body.devices[0].id === deviceId) {
			break;
		}
		if (devs.body.devices.length !== 0) {
			continue;
		}
		try {
			await api.post({ uri: `/v1/products/${product}/groups`, auth, data: {
				name: `ci_prod_ver-${version}`,
				product_id: product
			}});
		} catch (err) {
			console.log(err);
			if ('already exists' in err.body.error) {
				continue;
			}
			throw err;
		}
		devs = await api.listDevices({ auth, product, groups: [`ci_prod_ver-${version}`] });
		if (devs.body.devices.length !== 0) {
			continue;
		}
		await api.put({ uri: api.deviceUri({ deviceId, product }), auth, data: {
			groups: [`ci_prod_ver-${version}`]
		}});
		devs = await api.listDevices({ auth, product, groups: [`ci_prod_ver-${version}`] });
		console.log(devs);
		if (devs.body.devices.length !== 1) {
			await api.updateDevice({ auth, deviceId, product, groups: [] });
			continue;
		}
		break;
	}
	console.log('Product version', version);
	productVersion = Number(version);
	return productVersion;
}

before(async function() {
	api = this.particle.apiClient.instance;
	auth = this.particle.apiClient.token;
	device = this.particle.devices[0];
	deviceId = device.id;
	await checkDeviceInProductAndMarkAsDevelopment();
	await generateProductVersion();
	await generateAssets();
	await uploadProductFirmware(true /* removeOnly */);
});

test('01_ad_hoc_ota_start', async function() {
	await flash(this, bundle, `bundle_${timestamp}.zip`);
});

test('02_ad_hoc_ota_wait', async function() {
});

test('03_ad_hoc_ota_complete', async function() {
	const deviceReported = JSON.parse(device.mailBox.pop().d);
	const cloudReported = cloudReportedToReport(await waitForAssets(5 * 60 * 1000));
	const local = generatedAssetsToReport();
	expect(deviceReported.available).to.deep.equal(local);
	expect(deviceReported.required).excludingEvery(['crc', 'size', 'storageSize', 'readable']).to.deep.equal(local);
	expect(cloudReported).excludingEvery(['hash', 'crc', 'size', 'storageSize']).to.deep.equal(local);
});

test('04_ad_hoc_ota_restore', async function() {
	await device.flash(device.testAppBinFile);
});

test('05_product_ota_start', async function() {
	// Regenerate
	await generateAssets();
	await uploadProductFirmware(false /* removeOnly */);

	await api.updateDevice({ deviceId, auth, development: false, flash: true, product, desiredFirmwareVersion: productVersion });
	const dev = await api.getDevice({ deviceId, auth });
	expect(dev.body.development).to.be.false;
	expect(Number(dev.body.desired_firmware_version)).to.equal(Number(productVersion));
	expect(Number(dev.body.firmware_version)).to.not.equal(Number(productVersion));
});

test('06_product_ota_wait', async function() {
});

test('07_product_ota_complete', async function() {
	const deviceReported = JSON.parse(device.mailBox.shift().d);
	const cloudReported = cloudReportedToReport(await waitForAssets(5 * 60 * 1000));
	const local = generatedAssetsToReport();
	expect(deviceReported.available).to.deep.equal(local);
	expect(deviceReported.required).excludingEvery(['crc', 'size', 'readable', 'storageSize']).to.deep.equal(local);
	expect(cloudReported).excludingEvery(['hash', 'crc', 'size', 'storageSize']).to.deep.equal(local);
});

test('08_product_ota_complete_handled', async function() {

});

test('09_assets_handled_hook', async function() {

});

test('10_assets_read_skip_reset', async function() {
    const deviceReported = JSON.parse(device.mailBox.shift().d);
	const local = generatedAssetsToReport();
	expect(deviceReported.available).to.deep.equal(local);
	expect(deviceReported.required).excludingEvery(['crc', 'size', 'readable', 'storageSize']).to.deep.equal(local);
});

test('11_assets_available_after_eof_reports_zero', async function() {

});

test('12_assets_read_using_filesystem', async function() {
	const deviceReported = JSON.parse(device.mailBox.shift().d);
	const local = generatedAssetsToReport();
	console.dir(deviceReported, { depth: null });
	console.dir(local, { depth: null });
	expect(local).to.deep.include(deviceReported.available[0]);
	expect(deviceReported.required).excludingEvery(['crc', 'size', 'readable', 'storageSize']).to.deep.equal(local);
});

test('99_product_ota_restore', async function() {
	await checkDeviceInProductAndMarkAsDevelopment();
	await device.flash(device.testAppBinFile);
});

after(async function() {
	await checkDeviceInProductAndMarkAsDevelopment();
});
