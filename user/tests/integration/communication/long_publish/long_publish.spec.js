suite('Cloud events (long running)');

platform('gen3', 'gen4');

const EVENT_COUNT = 200;
const EVENT_INTERVAL = 250;
const EVENT_TIMEOUT = 30000;
const EVENT_SIZE = 1024;

const TEST_TIMEOUT = 10 * 60 * 1000;

let deviceId;

async function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function runPingPongTest(ctx, inEvent, outEvent) {
  let lastPubTime = 0;
  for (let outNum = 1; outNum <= EVENT_COUNT; ++outNum) {
    let dt = Date.now() - lastPubTime;
    if (dt < EVENT_INTERVAL) {
      await delay(EVENT_INTERVAL - dt);
    }
    // Publish an event
    const data = (outNum.toString() + ' ').padEnd(EVENT_SIZE, 'a');
    await ctx.publishEvent({
      name: `${deviceId}/${outEvent}`,
      data,
      retries: 3
    });
    lastPubTime = Date.now();
    // Receive back an event with the same number
    let inNum;
    do {
      const data = await ctx.receiveEvent(inEvent, { timeout: EVENT_TIMEOUT });
      if (!data || data.length != EVENT_SIZE) {
        throw new Error('Unexpected event size');
      }
      const pos = data.indexOf(' ');
      if (pos <= 0) {
        throw new Error('Unexpected event format');
      }
      inNum = Number(data.slice(0, pos));
    } while (inNum !== outNum);
  }
}

before(function() {
  const dev = this.particle.devices[0];
  deviceId = dev.id;
});

test('01_connect_and_subscribe', async function() {
});

test('02_ping_pong_old_api', async function() {
  this.timeout(TEST_TIMEOUT);
  const t1 = Date.now();
  await runPingPongTest(this.particle, 'devout1', 'devin1');
  const t2 = Date.now();
  console.log(`Events sent/received: ${EVENT_COUNT}
Time elapsed: ${Math.round((t2 - t1) * 10 / 1000) / 10}s`);
});

test('03_ping_pong_new_api', async function() {
  this.timeout(TEST_TIMEOUT);
  const t1 = Date.now();
  await runPingPongTest(this.particle, 'devout2', 'devin2');
  const t2 = Date.now();
  console.log(`Events sent/received: ${EVENT_COUNT}
Time elapsed: ${Math.round((t2 - t1) * 10 / 1000) / 10}s`);
});
