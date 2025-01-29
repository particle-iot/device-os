suite('BLE scanner broadcaster');
platform('gen3', 'p2');
fixture('ble_scanner', 'ble_broadcaster');
systemThread('enabled');
// This tag should be filtered out by default
tag('fixture');

const BASE_EVENT_NAME = 'ble-test/';

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
        if (fixture.name === 'ble_scanner' && !centralDevice) {
            centralDevice = d;
        }
        if (fixture.name === 'ble_broadcaster' && !peripheralDevice) {
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

test('BLE_0000_Check_Feature_Disable_Listening_Mode', async function () {
});

test('BLE_000_Broadcaster_Cloud_Connect', async function() {
    console.log(`Waiting for peer info from Broadcaster (Peripheral) device ${peripheralDevice.id}`);
    const data = await this.particle.receiveEvent(BASE_EVENT_NAME + peripheralDevice.id);
    console.log(data);
    peripheralDevice.peerInfo = data;
    await distributePeerInfo();
});

test('BLE_000_Scanner_Cloud_Connect', async function() {
    console.log(`Waiting for peer info from Scanner (Central) device ${centralDevice.id}`);
    const data = await this.particle.receiveEvent(BASE_EVENT_NAME + centralDevice.id);
    console.log(data);
    centralDevice.peerInfo = data;
    await distributePeerInfo();
});
