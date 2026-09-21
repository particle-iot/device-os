suite('No fixture cellular long running');

platform('cellular', 'msom');
// AT_RECOVERY_01 budgets 15 minutes of connection cycling, plus ~9 for the recovery rounds
// REG_BACKOFF_02 walks three shortened cooldowns with a modem power cycle between each, ~8 minutes
timeout(60 * 60 * 1000);

const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

// Node resolves the integration/wiring symlink before searching for node_modules, so a bare
// require() never reaches the runner's own
const INTEGRATION_ROOT = path.join(__dirname, '..', '..', 'integration');
const { createEnvVarsAssetModule } = require(
    require.resolve('binary-version-reader', { paths: [INTEGRATION_ROOT] }));

let device;
let bandMaskValue;
// The device clears the lock itself in REG_BACKOFF_97, so after() only steps in if the run died
let bandLockApplied = false;

before(function() {
  device = this.particle.devices[0];
});

// Env vars are read at boot and cannot be set over the wire, so we flash them as an asset
// Same mechanism as system/cellular_env
async function setEnvVarsAndFlash(vars) {
  const assetData = await createEnvVarsAssetModule(vars);
  // fs rather than tempy, which would need resolving from the integration tree too
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'reg-backoff-'));
  const assetPath = path.join(dir, 'reg_backoff_env_vars.bin');
  fs.writeFileSync(assetPath, assetData);
  return device.flash(assetPath);
}

test('AT_RECOVERY_00_init', async function() {
  // Selects a cellular device, so platform('msom') does not pick up a Wi-Fi configured M-SoM
  this.test.parent.particle.network = 'cellular';
  this.test.parent.particle.suiteInitialized = false;
});

test('AT_RECOVERY_01_data_mode_cycling_provokes_unresponsive_at', async () => {
});

test('AT_RECOVERY_02_device_os_recovers_the_modem', async () => {
  expect(device.mailBox).to.not.be.empty;
  const msg = JSON.parse(device.mailBox[0].d);
  if (!msg.reproduced) {
    // Probabilistic, roughly 1 in 8 data mode transitions. Not a regression.
    console.log(`AT interface stayed responsive across ${msg.cycles} data mode transitions, not reproduced this run`);
    return;
  }
  console.log(`Reproduced after ${msg.cycles} data mode transitions`);
  console.log(`Stood down -> connectivity lost: ${msg.detectMs} ms`);
  console.log(`Stood down -> AT working again: ${msg.recoveryMs} ms`);
  if (msg.recurrences) {
    // The fault can recur on the first transition after a recovery. Device OS recovering it again
    // is the correct outcome, not a failure.
    console.log(`Fault recurred ${msg.recurrences} more time(s) before a clean connection`);
  }
});

test('REG_BACKOFF_00_init', async function() {
  // Same cellular device selection as AT_RECOVERY_00_init, harmless to repeat.
  this.test.parent.particle.network = 'cellular';
  this.test.parent.particle.suiteInitialized = false;

  expect(device.mailBox).to.not.be.empty;
  const msg = device.mailBox[0].d;
  const match = msg.match(/^REG_BACKOFF_BANDS=(.+)$/);
  expect(match).to.not.be.null;
  bandMaskValue = match[1];

  // Flashed here, not in REG_BACKOFF_01's hook
  // A spec body runs after the device test of the same name, so 01 would be too late
  console.log(`Locking modem to LTE band mask ${bandMaskValue}`);
  await setEnvVarsAndFlash({
    PARTICLE_CELLULAR_PREFERRED_BANDS: bandMaskValue,
    PARTICLE_CELLULAR_FORBIDDEN_BANDS: '0',
  });
  bandLockApplied = true;
});

test('REG_BACKOFF_01_the_band_lock_is_in_force', async () => {
});

test('REG_BACKOFF_02_failed_windows_walk_the_schedule', async () => {
});

test('REG_BACKOFF_03_reset_breaks_a_cooldown', async () => {
});

test('REG_BACKOFF_97_restore_bands', async function() {
  // The device does the clearing, recording it here tells after() to stand down
  bandLockApplied = false;
});

test('REG_BACKOFF_98_the_device_recovers_on_the_shipping_schedule', async () => {
});

after(async function() {
  if (!bandLockApplied) {
    return;
  }
  // The run died with the modem still pinned to a dead band, which fails every later suite
  // The client intersects an all-ones mask with the defaults, so it reads as no restriction
  console.log('Band lock still applied after failure, restoring the default mask');
  this.timeout(5 * 60 * 1000);
  await setEnvVarsAndFlash({
    PARTICLE_CELLULAR_PREFERRED_BANDS: 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF',
    PARTICLE_CELLULAR_FORBIDDEN_BANDS: '0',
  });
});
