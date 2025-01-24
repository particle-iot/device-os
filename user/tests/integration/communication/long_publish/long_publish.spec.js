suite('Cloud events (long running)');

platform('gen3', 'gen4');

const TEST_DURATION = 2 * 60000;
const EVENT_TIMEOUT = 30000;
const EVENT_DATA_SIZE = 1024;

let deviceId;

async function runPingPongTest(ctx, inEvent, outEvent) {
  const t1 = Date.now();
  let count = 0;
  do {
    // Publish an event
    const outNum = ++count;
    const data = (outNum.toString() + ' ' + 'a'.repeat(EVENT_DATA_SIZE)).slice(0, EVENT_DATA_SIZE);
    await ctx.apiClient.instance.publishEvent({
      name: `${deviceId}/${outEvent}`,
      data,
      auth: ctx.apiClient.token
    });
    // Receive back an event with the same number
    let inNum;
    do {
      const data = await ctx.receiveEvent(inEvent, { timeout: EVENT_TIMEOUT });
      if (!data || data.length != EVENT_DATA_SIZE) {
        throw new Error('Unexpected event size');
      }
      const pos = data.indexOf(' ');
      if (pos <= 0) {
        throw new Error('Unexpected event format');
      }
      inNum = Number(data.slice(0, pos));
    } while (inNum !== outNum);
  } while (Date.now() - t1 < TEST_DURATION);
  return count;
}

before(function() {
  const dev = this.particle.devices[0];
  deviceId = dev.id;
});

test('01_connect_and_subscribe', async function() {
});

test('02_ping_pong_old_api', async function() {
  this.timeout(TEST_DURATION + EVENT_TIMEOUT);
  const count = await runPingPongTest(this.particle, 'devout1', 'devin1');
  expect(count).to.be.at.least(10);
  console.log('Sent/received events:', count);
});

test('03_ping_pong_new_api', async function() {
  this.timeout(TEST_DURATION + EVENT_TIMEOUT);
  const count = await runPingPongTest(this.particle, 'devout2', 'devin2');
  expect(count).to.be.at.least(10);
  console.log('Sent/received events:', count);
});
