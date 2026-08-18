suite('No fixture cellular long running');

platform('cellular', 'msom');
// AT_RECOVERY_01 budgets 15 minutes of connection cycling, plus up to ~9 for the recovery rounds.
timeout(40 * 60 * 1000);

let device;

before(function() {
  device = this.particle.devices[0];
});

test('AT_RECOVERY_00_init', async () => {
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
