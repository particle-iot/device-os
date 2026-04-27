suite('Manufacturing: empty external flash');

platform('gen3', 'gen4');
systemThread('enabled');

const usb = require('particle-usb');
const { platforms } = require('@particle/device-constants');
const { OpenOcdFlashInterface } = require('@particle/device-os-flash-util/lib/openocd');
const { UsbFlashInterface } = require('@particle/device-os-flash-util/lib/usb');
const { Flasher } = require('@particle/device-os-flash-util/lib/flasher');
const { ModuleCache } = require('@particle/device-os-flash-util/lib/module');
const { platformForId } = require('@particle/device-os-flash-util/lib/platform');
const { Logger, LogLevel } = require('@particle/device-os-flash-util/lib/log');
const fs = require('fs');
const path = require('path');
const os = require('os');
const mkdirp = require('mkdirp');

const { setTimeout: delay } = require('node:timers/promises');

const DEVICE_OS_TEST_RUNNER_KNOWN_GOOD_RELEASE = '6.3.5';

let device = null;
let logger = null;

let dumps = null;

let deviceOnline = false;

let currentDeviceOsVersion = null;

let adapterSerial = null;

async function resolveAdapterSerial(deviceId, logger) {
    let serial = process.env.DEVICE_OS_TEST_RUNNER_ADAPTER_SERIAL || null;
    const adapterMapPath = process.env.DEVICE_OS_TEST_RUNNER_ADAPTER_MAP || null;
    serial = serial || (adapterMapPath ? (JSON.parse(fs.readFileSync(adapterMapPath, 'utf8')))[deviceId] : null);

    const openocd = new OpenOcdFlashInterface({ log: logger });
    const adapters = await openocd.listDevices();
    let adapter = null;
    if (serial) {
        adapter = adapters.find(a => a.serialNumber === serial);
    } else {
        for (const ad of adapters) {
            try {
                await ad.open();
                if (ad.id === deviceId) {
                    adapter = ad;
                    serial = ad.serialNumber;
                    await ad.close();
                    break;
                }
                await ad.close();
            } catch (_err) {
                // Skip adapters that fail to open
            } finally {
                await ad.close();
            }
        }
    }

    await openocd.shutdown();

    if (!adapter || !serial) {
        throw new Error(`Debugger for ${deviceId} not found`);
    }

    adapterSerial = serial;
}

async function openWithRetry(deviceId, opts) {
    let err = null;
    for (let i = 0; i < 20; i++) {
        try {
            const device = await usb.openDeviceById(deviceId, opts);
            return device;
        } catch (e) {
            console.log(e);
            err = e;
        }
        await delay(i * 1000);
    }
    if (err) {
        throw err;
    }
}

before(async function() {
    device = this.particle.devices[0];
    logger = new Logger({ level: LogLevel.VERBOSE });
    device.on('mailbox', (msg) => {
        console.log('mailbox msg', msg);
    });

    await resolveAdapterSerial(device.id, logger);
});

after(function() {
    device.removeAllListeners('mailbox');
});

function getExternalFlashInfo(platformId) {
    for (const [, plat] of Object.entries(platforms)) {
        if (plat.id === platformId && plat.dfu && plat.dfu.storage) {
            const extFlash = plat.dfu.storage.find(s => s.type === 'externalFlash');
            if (extFlash) {
                return { altSetting: extFlash.alt, platform: plat };
            }
        }
    }
    return null;
}

async function getDfuSegments(dfu, altSetting) {
    await dfu.setAltSetting(altSetting);
    return dfu._memoryInfo.segments;
}

test('01_backup_external_flash_and_erase', async function() {
    const deviceId = device.id;
    const platformId = device.platform.id;

    // Capture current Device OS version before entering DFU mode
    if (!currentDeviceOsVersion) {
        const usbDevice = await device.getUsbDevice();
        currentDeviceOsVersion = usbDevice.firmwareVersion;
        await usbDevice.close();
        console.log(`Device OS version: ${currentDeviceOsVersion}`);
    }

    const extFlashInfo = getExternalFlashInfo(platformId);

    let dfuDevice = null;
    try {
        // Enter DFU mode via particle-usb
        const usbDevice = await device.getUsbDevice();
        await usbDevice.enterDfuMode();
        await usbDevice.close();

        dfuDevice = await openWithRetry(deviceId, { includeDfu: true });

        // Discover external flash region from DFU memory descriptor
        const extSegments = await getDfuSegments(dfuDevice._dfu, extFlashInfo.altSetting);
        if (!extSegments || !extSegments.length) {
            throw new Error('No memory segments found for external flash alt setting');
        }
        const extStart = extSegments[0].start;
        const extSize = extSegments[extSegments.length - 1].end - extStart;

        console.log(`Reading external flash: alt=${extFlashInfo.altSetting}, addr=0x${extStart.toString(16)}, size=${extSize}`);
        const extFlashData = await dfuDevice.readOverDfu({
            altSetting: extFlashInfo.altSetting,
            startAddr: extStart,
            size: extSize
        });
        console.log(`Read ${extFlashData.length} bytes from external flash`);
    
        dumps = {
            extFlash: {
                data: extFlashData,
                alt: extFlashInfo.altSetting,
                start: extStart,
                size: extSize
            }
        };

        await dfuDevice._dfu.setAltSetting(dumps.extFlash.alt);
        await dfuDevice._dfu._erase(dumps.extFlash.start, dumps.extFlash.size);
    } catch (err) {
        console.log(err);
        throw err;
    } finally {
        if (dfuDevice) {
            device.setWillDetach(true);
            if (dfuDevice._dfu) {
                await dfuDevice._dfu.close();
            }
            await dfuDevice.reset();
            await dfuDevice.close();
        }
    }
});

test('02_check_device_online', async function() {
    this.timeout(30000);
    try {
        const usbDevice = await device.getUsbDevice();
        await usbDevice.close();

        deviceOnline = true;
    } catch (err) {
        console.log('Device not connectable:', err.message);
    }
});

test('03_rollback_release', async function() {
    if (deviceOnline) {
        console.log('Device is online, skipping rollback');
        return;
    }

    this.timeout(10 * 60 * 1000);

    const knownGoodRelease = process.env.DEVICE_OS_TEST_RUNNER_KNOWN_GOOD_RELEASE || DEVICE_OS_TEST_RUNNER_KNOWN_GOOD_RELEASE;
    const currentReleaseDir = process.env.DEVICE_OS_TEST_RUNNER_RELEASE || null;

    const deviceId = device.id;
    const platformId = device.platform.id;
    const platformName = device.platform.name || platformForId(platformId)?.name;

    // Download known good release modules
    const tempDir = path.join(os.tmpdir(), 'device-os-flash-rollback');
    mkdirp.sync(tempDir);

    const moduleCache = new ModuleCache({ cacheDir: tempDir, tempDir, log: logger });
    await moduleCache.init();

    const knownGoodModules = await moduleCache.getReleaseModules(knownGoodRelease);
    const platformModules = knownGoodModules.filter(m => m.platformId === platformId);
    if (!platformModules.length) {
        throw new Error(`No modules found for platform ${platformName} in release ${knownGoodRelease}`);
    }
    console.log(`Downloaded ${knownGoodRelease}: ${platformModules.length} modules for ${platformName}`);

    // Flash known good release over debugger
    const openocd = new OpenOcdFlashInterface({ log: logger });
    await openocd.init();
    const usbInterface = new UsbFlashInterface({ log: logger });
    await usbInterface.init();

    let openocdDevice = null;
    try {
        const adapters = await openocd.listDevices();
        const adapter = adapters.find(a => a.serialNumber === adapterSerial);
        if (!adapter) {
            throw new Error(`Adapter with serial ${adapterSerial} not found`);
        } else {
            console.log(`Found matching adapter`);
        }
        openocdDevice = adapter;
        openocdDevice.platformId = platformId;

        const flasher = new Flasher({
            name: deviceId,
            device: openocdDevice,
            dfu: openocdDevice,
            usb: usbInterface,
            tempDir,
            log: logger
        });
        await flasher.run(platformModules, { maxRetries: 99 });
        console.log(`Flashed known good release ${knownGoodRelease}`);
    } finally {
        if (openocdDevice) {
            await openocdDevice.close();
        }
    }

    // Reset device from debugger
    if (openocdDevice) {
        await openocdDevice.open();
        await openocdDevice.reset();
        await openocdDevice.close();
    }

    // Wait for device to come online
    console.log('Waiting for device to come online after known good release...');
    let online = false;
    for (let i = 0; i < 60; i++) {
        try {
            const usbDevice = await device.getUsbDevice();
            await usbDevice.close();
            online = true;
            break;
        } catch (_err) {
            await new Promise(r => setTimeout(r, 2000));
        }
    }
    if (!online) {
        throw new Error('Device did not come online after flashing known good release');
    }
    console.log('Device online with known good release');

    // Determine current release modules
    let currentPlatformModules;
    if (currentReleaseDir) {
        console.log(`Using current release dir: ${currentReleaseDir}`);
        const currentModules = await moduleCache.getModulesFromPath(currentReleaseDir);
        currentPlatformModules = currentModules.filter(m => m.platformId === platformId);
    } else {
        if (!currentDeviceOsVersion) {
            throw new Error('No current Device OS version available');
        }
        console.log(`Downloading current release: ${currentDeviceOsVersion}`);
        const currentModules = await moduleCache.getReleaseModules(currentDeviceOsVersion);
        currentPlatformModules = currentModules.filter(m => m.platformId === platformId);
    }
    if (!currentPlatformModules.length) {
        throw new Error(`No modules found for platform ${platformName} in current release`);
    }

    // Replace user part with test app binary if available
    if (device.testAppBinFile && fs.existsSync(device.testAppBinFile)) {
        const testAppModules = await moduleCache.getModulesFromPath(device.testAppBinFile);
        const testAppPlatform = testAppModules.filter(m => m.platformId === platformId);
        for (const m of testAppPlatform) {
            const existingIdx = currentPlatformModules.findIndex(cm => cm.type === m.type);
            if (existingIdx >= 0) {
                currentPlatformModules[existingIdx] = m;
            } else {
                currentPlatformModules.push(m);
            }
        }
        console.log(`Added test app binary: ${device.testAppBinFile}`);
    }

    try {
        openocdDevice.platformId = platformId;

        const flasher = new Flasher({
            name: deviceId,
            device: openocdDevice,
            dfu: openocdDevice,
            usb: usbInterface,
            tempDir,
            log: logger
        });
        await flasher.run(currentPlatformModules, { maxRetries: 99 });
        console.log('Flashed current release');
    } finally {
        if (openocdDevice) {
            await openocdDevice.close();
        }
        await openocd.shutdown();
        await usbInterface.shutdown();
        await moduleCache.shutdown();
    }

    // Reset device and verify it comes online
    if (openocdDevice) {
        await openocdDevice.open();
        await openocdDevice.reset();
        await openocdDevice.close();
    }

    console.log('Waiting for device to come online after current release...');
    online = false;
    for (let i = 0; i < 60; i++) {
        try {
            const usbDevice = await device.getUsbDevice();
            await usbDevice.close();
            online = true;
            break;
        } catch (_err) {
            await new Promise(r => setTimeout(r, 2000));
        }
    }
    if (!online) {
        throw new Error('Device did not come online after flashing current release');
    }
    console.log('Device online with current release, rollback complete');
    deviceOnline = true;
});

test('98_restore_external_flash', async function() {

    let dfuDevice = null;

    const deviceId = device.id;
    
    if (!dumps) {
        console.log('No dump available');
        return;
    }

    let ok = false;

    for (let i = 0; i < 10 && !ok; i++) {
        try {
            // Enter DFU mode via particle-usb
            const usbDevice = await device.getUsbDevice();
            await usbDevice.enterDfuMode();
            await usbDevice.close();

            dfuDevice = await openWithRetry(deviceId, { includeDfu: true });

            await dfuDevice.writeOverDfu(dumps.extFlash.data, {
                altSetting: dumps.extFlash.alt,
                startAddr: dumps.extFlash.start,
                leave: false,
                noErase: false
            });
            ok = true;
        } catch (err) {
            console.log(err);
            throw err;
        } finally {
            if (dfuDevice) {
                device.setWillDetach(true);
                if (dfuDevice._dfu) {
                    await dfuDevice._dfu.close();
                }
                await dfuDevice.reset();
                await dfuDevice.close();
            }
        }
    }
});

test('99_verify', async function() {

});
