suite('Cellular env vars');

platform('cellular', 'msom');

const { createEnvVarsAssetModule } = require('binary-version-reader');
const tempy = require('tempy');
const _ = require('lodash');

const { readFile } = require('node:fs/promises');

const { waitFlashStatusEvent } = require('../../test/ota');

let appBinary;
let deviceId;
let device;
let preferredBandsValue;
let forbiddenBandsValue;

async function setEnvVarsAndFlash(vars) {
    const assetData = await createEnvVarsAssetModule(vars);
    const assetPath = await tempy.write(assetData, { name: 'cellular_env_vars.bin' });
    return device.flash(assetPath);
}

before(async function() {
    device = this.particle.devices[0];
    deviceId = device.id;

    appBinary = await readFile(device.testAppBinFile);
});

test('1_particle_cellular_preferred_bands_init', async function () {
    this.test.parent.particle.network = 'cellular';
    this.test.parent.particle.suiteInitialized = false;
    expect(device.mailBox).to.not.be.empty;
    const msg = device.mailBox[0].d;
    console.log(msg);
    const match = msg.match(/^PARTICLE_CELLULAR_PREFERRED_BANDS=(.+)$/);
    expect(match).to.not.be.null;
    preferredBandsValue = match[1];
});

test('2_particle_cellular_preferred_bands_default', async function () {
    expect(preferredBandsValue).to.be.a('string');
    await setEnvVarsAndFlash({
        PARTICLE_CELLULAR_PREFERRED_BANDS: preferredBandsValue,
        PARTICLE_CELLULAR_FORBIDDEN_BANDS: '0',
    });
    expect(device.mailBox).to.not.be.empty;
    console.log(device.mailBox[0].d);
});

test('3_particle_cellular_preferred_bands_set', async function () {
    expect(device.mailBox).to.not.be.empty;
    console.log(device.mailBox[0].d);
});

test('4_particle_cellular_forbidden_bands_init', async function () {
    this.test.parent.particle.network = 'cellular';
    this.test.parent.particle.suiteInitialized = false;
    expect(device.mailBox).to.not.be.empty;
    const msg = device.mailBox[0].d;
    console.log(msg);
    const match = msg.match(/^PARTICLE_CELLULAR_FORBIDDEN_BANDS=(.+)$/);
    expect(match).to.not.be.null;
    forbiddenBandsValue = match[1];
});

test('5_particle_cellular_forbidden_bands_default', async function () {
    expect(forbiddenBandsValue).to.be.a('string');
    await setEnvVarsAndFlash({
        PARTICLE_CELLULAR_PREFERRED_BANDS: 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF',
        PARTICLE_CELLULAR_FORBIDDEN_BANDS: forbiddenBandsValue,
    });
    expect(device.mailBox).to.not.be.empty;
    console.log(device.mailBox[0].d);
});

test('6_particle_cellular_forbidden_bands_set', async function () {
    expect(device.mailBox).to.not.be.empty;
    console.log(device.mailBox[0].d);
});

test('7_particle_cellular_preferred_plmn_init', async function () {
    this.test.parent.particle.network = 'cellular';
    this.test.parent.particle.suiteInitialized = false;
});

test('8_particle_cellular_preferred_plmn_default', async function () {
    await setEnvVarsAndFlash({
        PARTICLE_CELLULAR_PREFERRED_PLMN: '310410,310260,311480',
    });
    expect(device.mailBox).to.not.be.empty;
    console.log(device.mailBox[0].d);
});

test('9_particle_cellular_preferred_plmn_set', async function () {
    expect(device.mailBox).to.not.be.empty;
    console.log(device.mailBox[0].d);

    await setEnvVarsAndFlash({
        PARTICLE_CELLULAR_CLOUD_KEEP_ALIVE: '60',
        PARTICLE_CLOUD_KEEP_ALIVE: '70'
    });
});

test('10_particle_cellular_keepalive', async function () {

});

test('11_particle_cellular_preferred_plmn_cleanup', async function () {
    delete this.test.parent.particle.network;
    this.test.parent.particle.suiteInitialized = false;
});

test('12_particle_cellular_env_vars_cleared_verify_defaults', async function () {
    expect(device.mailBox).to.not.be.empty;
    console.log(device.mailBox[0].d);
});

test('97_cleanup', async function() {

});

test('98_cleanup', async function() {
    await waitFlashStatusEvent(this, { status: 'success' });
});

test('99_cleanup_1', async function() {

});

test('99_cleanup_2', async function() {

});
