suite('System level env vars');

platform('gen3', 'gen4');

const { createEnvVarsAssetModule } = require('binary-version-reader');
const tempy = require('tempy');
const _ = require('lodash');

const { readFile } = require('node:fs/promises');

let appBinary;
let deviceId;
let device;

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
});

test('2_particle_cellular_preferred_bands_default', async function () {
    // Flash env vars: PREFERRED_BANDS set to disable bands 1 & 2
    // FORBIDDEN_BANDS explicitly set to 0 so it doesn't interfere
    await setEnvVarsAndFlash({
        PARTICLE_CELLULAR_PREFERRED_BANDS: 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC',
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
});

test('5_particle_cellular_forbidden_bands_default', async function () {
    // Flash env vars: FORBIDDEN_BANDS set to forbid bands 1 & 2 (bits 0 & 1)
    // PREFERRED_BANDS set to all-ones so it doesn't restrict anything
    await setEnvVarsAndFlash({
        PARTICLE_CELLULAR_FORBIDDEN_BANDS: '3',
        PARTICLE_CELLULAR_PREFERRED_BANDS: 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF',
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
});

test('10_particle_cellular_preferred_plmn_cleanup', async function () {
    delete this.test.parent.particle.network;
    this.test.parent.particle.suiteInitialized = false;
});

test('11_particle_cellular_env_vars_cleared_verify_defaults', async function () {
    expect(device.mailBox).to.not.be.empty;
    console.log(device.mailBox[0].d);
});

test('99_cleanup', async function() {

});
