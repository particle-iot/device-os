suite('Ledger')

platform('gen3');

const Particle = require('particle-api-js');

const DEVICE_TO_CLOUD_LEDGER = 'test-device-to-cloud';
const CLOUD_TO_DEVICE_LEDGER = 'test-cloud-to-device';
const ORG_ID = 'particle'; // Leave this constant undefined to use the sandbox account

let api;
let auth;
let device;
let deviceId;

async function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

before(function() {
  api = new Particle();
  auth = this.particle.apiClient.token;
  device = this.particle.devices[0];
  deviceId = device.id;
});

test('01_init_ledgers', async function() {
  // See ledger.cpp
});

test('02_sync_device_to_cloud', async function() {
  await delay(1000);
  const { body: { instance } } = await api.getLedgerInstance({ ledgerName: DEVICE_TO_CLOUD_LEDGER, scopeValue: deviceId, org: ORG_ID, auth });
  expect(instance.data).to.deep.equal({ a: 1, b: 2, c: 3 });
});

test('03_update_cloud_to_device', async function() {
  await api.setLedgerInstance({ ledgerName: CLOUD_TO_DEVICE_LEDGER, instance: { data: { d: 4, e: 5, f: 6 } }, scopeValue: deviceId, org: ORG_ID, auth });
});

test('04_validate_cloud_to_device_sync', async function() {
  // See ledger.cpp
});
