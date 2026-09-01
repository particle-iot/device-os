'use strict';
/* eslint no-undef: 'off' */

const { UsbCdcPort } = require('../../test/usb_cdc');

suite('USBSerial');
platform('gen3', 'gen4');
systemThread('enabled');
timeout(2 * 60 * 1000);

const DEVICE_TO_HOST_DATA = Buffer.from('device-to-host');
const HOST_TO_DEVICE_DATA = Buffer.from('host-to-device');
const NONBLOCKING_STRESS_TEST = 'USBSERIAL_13_NonBlockingWriteStressHandlesBackpressure';
const NONBLOCKING_CHARACTER_STRESS_TEST = 'USBSERIAL_16_NonBlockingCharacterWriteStressHandlesBackpressure';
const CLOSE_PORT_AFTER_TESTS = new Set([
    'USBSERIAL_04_isConnectedInitially',
    'USBSERIAL_06_isConnectedDetectsReopenedPort'
]);
const OPEN_PORT_AFTER_TESTS = new Set([
    'USBSERIAL_05_ClosedPortWritesFailWithoutBlocking',
    'USBSERIAL_07_EndBeginWhilePortIsClosed'
]);
const RX_STRESS_CHUNK_SIZE = 64;
const RX_STRESS_ITERATIONS = 256;
const SKIP_TEST_DATA_SIZE = 64;
const SKIP_TEST_PATTERN_OFFSET = 3;
const PAUSED_INPUT_TESTS = new Set([
    NONBLOCKING_STRESS_TEST,
    NONBLOCKING_CHARACTER_STRESS_TEST
]);

let cdc = null;
let device = null;
let rxBufferSize = 0;
let rxWriteError = null;
let rxStressWrite = null;

function rxData(iteration, size = rxBufferSize) {
    const chars = Buffer.from('0123456789ABCDEF');
    const data = Buffer.alloc(size);
    for (let i = 0; i < data.length; ++i) {
        data[i] = chars[(i + iteration) % chars.length];
    }
    return data;
}

async function discardDeviceOutput() {
    // The device-side write has completed before its same-named JS test runs.
    // Give the final USB packet time to reach the host before discarding it.
    await new Promise((resolve) => setTimeout(resolve, 100));
    cdc.discardInput();
}

before(async function() {
    device = this.particle.devices[0];
    if (!device) {
        throw new Error('No device assigned to this test');
    }
    cdc = new UsbCdcPort(device.id);
});

after(async function() {
    if (cdc) {
        await cdc.close();
    }
});

afterEach(async function() {
    // If the device-side stress assertions fail, its JS body is not run. Do
    // not leave input paused for the following blocking test in that case.
    // The HAL also treats a prolonged stalled TX endpoint as a lost host and
    // clears its logical CDC connection. Reopening the tty makes the host send
    // the line-state request again before the following blocking test starts.
    if (cdc && PAUSED_INPUT_TESTS.has(this.currentTest.title)) {
        cdc.resumeInput();
        await cdc.reconnect();
        cdc.discardInput();
    }
    // The device test runs before its same-named JS test. Keep the next test's
    // required port state even when the current device or JS assertion fails.
    if (cdc && CLOSE_PORT_AFTER_TESTS.has(this.currentTest.title)) {
        await cdc.disconnect();
    } else if (cdc && OPEN_PORT_AFTER_TESTS.has(this.currentTest.title) && !cdc.isOpen()) {
        await cdc.reconnect();
    }
});

test('USBSERIAL_00_RingBufferHelperIsSane', async function() {
    // The runner initializes the device suite immediately before this JS test,
    // which may reset the device and invalidate an already-open tty.
    expect(cdc.isOpen()).to.be.false;
    const devicePath = await cdc.open();
    console.log(`Opened USB CDC port ${devicePath} for ${device.id}`);
    cdc.discardInput();
});

test('USBSERIAL_01_SerialDoesNotDeadlockWhenInterruptsAreMasked', async function() {
    expect(cdc.isOpen()).to.be.true;
    await discardDeviceOutput();
});

test('USBSERIAL_02_ReadWrite', async function() {
    const data = await cdc.read(DEVICE_TO_HOST_DATA.length);
    expect(data.equals(DEVICE_TO_HOST_DATA)).to.be.true;
    await cdc.write(HOST_TO_DEVICE_DATA);
});

test('USBSERIAL_03_ReadWriteVerifiesHostData', async function() {
    expect(cdc.isOpen()).to.be.true;
});

test('USBSERIAL_04_isConnectedInitially', async function() {
    expect(cdc.isOpen()).to.be.true;
    await cdc.disconnect();
});

test('USBSERIAL_05_ClosedPortWritesFailWithoutBlocking', async function() {
    expect(cdc.isOpen()).to.be.false;
    await cdc.reconnect();
});

test('USBSERIAL_06_isConnectedDetectsReopenedPort', async function() {
    expect(cdc.isOpen()).to.be.true;
    await cdc.disconnect();
});

test('USBSERIAL_07_EndBeginWhilePortIsClosed', async function() {
    expect(cdc.isOpen()).to.be.false;
    await cdc.reconnect();
});

test('USBSERIAL_08_RxBufferSetup', async function() {
    expect(cdc.isOpen()).to.be.true;
    expect(device.mailBox).to.have.lengthOf(1);
    rxBufferSize = Number.parseInt(device.mailBox[0].d, 10);
    expect(rxBufferSize).to.be.above(0);
    await cdc.write(rxData(0));
});

test('USBSERIAL_09_RxBufferFillsCompletelyFirst', async function() {
    expect(cdc.isOpen()).to.be.true;
    await cdc.write(rxData(1));
});

test('USBSERIAL_10_RxBufferFillsCompletelySecond', async function() {
    expect(cdc.isOpen()).to.be.true;
    await cdc.write(rxData(2));
});

test('USBSERIAL_11_RxBufferFillsCompletelyThird', async function() {
    expect(cdc.isOpen()).to.be.true;
});

test('USBSERIAL_12_NonBlockingWriteStressSetup', async function() {
    expect(cdc.isOpen()).to.be.true;
    cdc.discardInput();
    cdc.pauseInput();
});

test(NONBLOCKING_STRESS_TEST, async function() {
    expect(cdc.isOpen()).to.be.true;
    cdc.resumeInput();
    await discardDeviceOutput();
});

test('USBSERIAL_14_BlockingWriteStressWritesEveryBuffer', async function() {
    expect(cdc.isOpen()).to.be.true;
    await discardDeviceOutput();
});

test('USBSERIAL_15_NonBlockingCharacterWriteStressSetup', async function() {
    expect(cdc.isOpen()).to.be.true;
    cdc.discardInput();
    cdc.pauseInput();
});

test(NONBLOCKING_CHARACTER_STRESS_TEST, async function() {
    expect(cdc.isOpen()).to.be.true;
    cdc.resumeInput();
    await discardDeviceOutput();
});

test('USBSERIAL_17_BlockingCharacterWriteStressWritesEveryByte', async function() {
    expect(cdc.isOpen()).to.be.true;
    await discardDeviceOutput();
});

test('USBSERIAL_18_DeviceReceiveStressSetup', async function() {
    expect(cdc.isOpen()).to.be.true;
    expect(rxBufferSize).to.be.above(0);
    rxWriteError = null;
    const chunks = [];
    for (let i = 0; i < RX_STRESS_ITERATIONS; ++i) {
        chunks.push(rxData(i, RX_STRESS_CHUNK_SIZE));
    }
    const stream = Buffer.concat(chunks);
    // Do not await drain here: the device consumes this stream in test 19,
    // which runs after this JS test completes. Let the runner's start-test
    // control request finish before beginning the CDC transfer on Gen 3.
    rxStressWrite = new Promise((resolve) => setTimeout(resolve, 250))
        .then(() => cdc.write(stream))
        .catch((err) => {
            rxWriteError = err;
        });
});

test('USBSERIAL_19_DeviceReceiveStress', async function() {
    expect(cdc.isOpen()).to.be.true;
    await rxStressWrite;
    expect(rxWriteError).to.be.null;
});

test('USBSERIAL_20_NullReceiveSetup', async function() {
    expect(cdc.isOpen()).to.be.true;
    await cdc.write(rxData(SKIP_TEST_PATTERN_OFFSET, SKIP_TEST_DATA_SIZE));
});

test('USBSERIAL_21_ReceiveWithNullBufferSkipsData', async function() {
    expect(cdc.isOpen()).to.be.true;
});
