suite('BLE central peripheral');
platform('gen3');
fixture('ble_central', 'ble_peripheral');
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
        if (fixture.name === 'ble_central' && !centralDevice) {
            centralDevice = d;
        }
        if (fixture.name === 'ble_peripheral' && !peripheralDevice) {
            peripheralDevice = d;
        }
    }
});

async function distributePeerInfo() {
    if (centralDevice.peerInfo && peripheralDevice.peerInfo) {
        console.log('Exchanging peer info');
        await api.publishEvent({ name: BASE_EVENT_NAME + centralDevice.id, data: peripheralDevice.peerInfo, auth: auth });
        await api.publishEvent({ name: BASE_EVENT_NAME + peripheralDevice.id, data: centralDevice.peerInfo, auth: auth });
    }
}

test('BLE_000_Peripheral_Cloud_Connect', async function() {
    console.log(`Waiting for peer info from Peripheral device ${peripheralDevice.id}`);
    const data = await this.particle.receiveEvent(BASE_EVENT_NAME + peripheralDevice.id);
    console.log(data);
    peripheralDevice.peerInfo = data;
    await distributePeerInfo();
});

test('BLE_000_Central_Cloud_Connect', async function() {
    console.log(`Waiting for peer info from Central device ${centralDevice.id}`);
    const data = await this.particle.receiveEvent(BASE_EVENT_NAME + centralDevice.id);
    console.log(data);
    centralDevice.peerInfo = data;
    await distributePeerInfo();
});
