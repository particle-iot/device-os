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
  console.log(`Nth percentile variation: ${msg[0]}us`);
  console.log(`Maximum variation: ${msg[1]}us`);
});
