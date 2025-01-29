suite('SLEEP 2.0');

platform('gen3', 'p2');
systemThread('enabled');
timeout(15 * 60 * 1000);

fixture('sleep20_tester','sleep20_device');

// This tag should be filtered out by default
tag('fixture');

const BASE_EVENT_NAME = 'sleep20-test/';

let api = null;
let auth = null;
let centralDevice = null;
let peripheralDevice = null;

before(function() {
    api = this.particle.apiClient.instance;
    auth = this.particle.apiClient.token;
    for (let d of this.particle.devices) {
        const fixture = this.particle.fixtures.get(d.id);
        if (!fixture || !fixture.name) {
            continue;
        }
        if (fixture.name === 'sleep20_tester' && !centralDevice) {
            centralDevice = d;
        }
        if (fixture.name === 'sleep20_device' && !peripheralDevice) {
            peripheralDevice = d;
        }
    }
});

async function distributePeerInfo() {
    if (centralDevice.peerInfo && peripheralDevice.peerInfo) {
        console.log('Exchanging peer info');
        await this.particle.publishEvent({ name: BASE_EVENT_NAME + centralDevice.id, data: peripheralDevice.peerInfo, auth: auth });
        await this.particle.publishEvent({ name: BASE_EVENT_NAME + peripheralDevice.id, data: centralDevice.peerInfo, auth: auth });
    }
}

test('000_System_Sleep_Peripheral_Cloud_Connect', async function() {
    console.log(`Waiting for peer info from Sleep-20 Peripheral device ${peripheralDevice.id}`);
    const data = await this.particle.receiveEvent(BASE_EVENT_NAME + peripheralDevice.id);
    console.log(data);
    peripheralDevice.peerInfo = data;
    await distributePeerInfo();
});

test('000_System_Sleep_Central_Cloud_Connect', async function() {
    console.log(`Waiting for peer info from Sleep-20 Central device ${centralDevice.id}`);
    const data = await this.particle.receiveEvent(BASE_EVENT_NAME + centralDevice.id);
    console.log(data);
    centralDevice.peerInfo = data;
    await distributePeerInfo();
});
