suite('No fixture long running');

platform('gen3', 'gen4');
timeout(32 * 60 * 1000);

let device;

before(function() {
  device = this.particle.devices[0];
});

test('DELAY_01_init', async () => {
});

test('DELAY_02_accuracy_is_within_tolerance', async () => {
  expect(device.mailBox).to.not.be.empty;
  const msg = device.mailBox[0].d.split(',');
  console.log(`95th percentile variation: ${msg[0]}us`);
  console.log(`99th percentile variation: ${msg[1]}us`);
  console.log(`Maximum variation: ${msg[2]}us`);
});

test('DELAY_03_app_events_are_processed_at_expected_rate_in_threaded_mode', async () => {
});

test('TICKS_04_call_cost_is_within_tolerance', async () => {
  expect(device.mailBox).to.not.be.empty;
  const msg = device.mailBox[0].d.split(',');
  console.log(`micros() call cost: ${msg[0]}ns`);
  console.log(`millis() call cost: ${msg[1]}ns`);
  console.log(`System.millis() call cost: ${msg[2]}ns`);
});

test('TICKS_05_micros_resolution_is_sub_millisecond', async () => {
});

test('NETWORK_01_LargePacketsDontCauseIssues_ResolveMtu', async() => {
  expect(device.mailBox).to.not.be.empty;
  const msg = JSON.parse(device.mailBox[0].d);
  console.log(`MTU: ${msg.mtu}`);
});