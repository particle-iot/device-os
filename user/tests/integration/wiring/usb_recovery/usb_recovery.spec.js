suite('USB recovery');
platform('gen4');
systemThread('enabled');
timeout(3 * 60 * 1000);

const { UsbCdcPort } = require('../../test/usb_cdc');
const path = require('path');

const EP0_POLL_TIMEOUT_MS = 250;

let device = null;
let cdc = null;

function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

function nodeUsbDevice(usbDev) {
    return usbDev.usbDevice.internalObject;
}

// The runner's node-usb (particle-usb's), for its low-level Transfer
const nodeUsb = require(require.resolve('usb', {
    paths: [path.dirname(require.resolve('particle-usb', { paths: [path.dirname(require.main.filename)] }))]
})).usb;

// GET_STATUS, cancelled if it hasn't completed after cancelMs. Not with a transfer timeout: on Linux node-usb's libusb
// handles timeouts only when its event thread wakes up (up to 60 s later), so a NAKed request just completes once the
// NAK ends. A cancel goes to the kernel at once.
async function getStatus(usbDev, cancelMs) {
    const dev = nodeUsbDevice(usbDev);
    return await new Promise((resolve, reject) => {
        // Setup packet: bmRequestType, bRequest, wValue, wIndex, wLength
        const buf = Buffer.alloc(nodeUsb.LIBUSB_CONTROL_SETUP_SIZE + 2);
        buf.writeUInt8(0x80, 0);
        buf.writeUInt8(0x00 /* GET_STATUS */, 1);
        buf.writeUInt16LE(0, 2);
        buf.writeUInt16LE(0, 4);
        buf.writeUInt16LE(2, 6);
        let timer = null;
        const transfer = new nodeUsb.Transfer(dev, 0, nodeUsb.LIBUSB_TRANSFER_TYPE_CONTROL, 0 /* no timeout */,
                (err, data, actual) => {
            clearTimeout(timer);
            if (err) {
                reject(err);
            } else {
                resolve(data.slice(nodeUsb.LIBUSB_CONTROL_SETUP_SIZE, nodeUsb.LIBUSB_CONTROL_SETUP_SIZE + actual));
            }
        });
        transfer.submit(buf);
        timer = setTimeout(() => {
            try {
                transfer.cancel();
            } catch (_) {
                // Completed meanwhile
            }
        }, cancelMs);
    });
}

async function getDescriptor(usbDev) {
    const dev = nodeUsbDevice(usbDev);
    return await new Promise((resolve, reject) => {
        // bmRequestType, bRequest, wValue, wIndex, data_or_length
        dev.controlTransfer(0x80, 0x06 /* GET_DESCRIPTOR */,  (0x01 << 8) | 0, 0, 18, (err, data) => (err ? reject(err) : resolve(data)));
    });
}

const LIBUSB_TRANSFER_TIMED_OUT = 2;
const LIBUSB_TRANSFER_CANCELLED = 3;

// Polls GET_STATUS control request
async function pollUntilTimeout(usbDev, timeoutMs, maxMs) {
    const deadline = Date.now() + maxMs;
    while (Date.now() < deadline) {
        try {
            await getStatus(usbDev, timeoutMs);
        } catch (err) {
            if (err.errno === LIBUSB_TRANSFER_CANCELLED || err.errno === LIBUSB_TRANSFER_TIMED_OUT) {
                return true;
            }
            throw err; // Anything else (a stall, a detach) is a real failure
        }
        await sleep(10);
    }
    return false; // Reached maxMs before USB timeout happened
}

async function systemEcho(usbDev, data, timeoutMs) {
    const CTRL_REQUEST_ECHO = 1; // system/inc/system_control.h
    return await usbDev.sendControlRequest(CTRL_REQUEST_ECHO, data, { timeout: timeoutMs });
}

// Write to the CDC port, giving up after timeoutMs. Resolves with 'ok' or 'timeout' and rejects on a write error. A
// write that times out stays pending (the device isn't taking the data), so its error is caught here: otherwise it
// would reject unhandled after the race is over.
async function cdcWriteWithTimeout(cdc, data, timeoutMs) {
    const write = cdc.write(data).then(() => 'ok', (err) => err);
    let timer = null;
    const timeout = new Promise((resolve) => {
        timer = setTimeout(() => resolve('timeout'), timeoutMs);
    });
    try {
        const r = await Promise.race([write, timeout]);
        if (r instanceof Error) {
            throw r;
        }
        return r;
    } finally {
        clearTimeout(timer);
    }
}

before(function() {
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

test('USB_RECOVERY_01_EP0_WATCHDOG_REATTACH', async function() {
    // The runner's polling of the device test is the host traffic the watchdog needs
});

test('USB_RECOVERY_02_GLOBAL_NAK_CLEARED', async function() {
    // Device does validation
});

test('USB_RECOVERY_03_CANCEL_ON_SETUP_TRIGGER', async function() {
    // Poll GET_STATUS until a request times out (the global IN NAK held its reply back)
    const usbDev = await device.getUsbDevice();
    const timedOut = await pollUntilTimeout(usbDev, EP0_POLL_TIMEOUT_MS, 30000);
    expect(timedOut, `no GET_STATUS was held back long enough to cancel (${EP0_POLL_TIMEOUT_MS} ms): the device's ` +
            `global IN NAK didn't take effect`).to.be.true;

    // When we time out, wait 2 seconds for devices global NAK timer to expire
    await sleep(2000);
    
    // Then try a different control request (GET_DESCRIPTOR) it should *not* return GET_STATUS data
    const data = await getDescriptor(usbDev);
    expect(data.length).to.equal(18);
    expect(data[0]).to.equal(0x12); // bLength: 18 bytes
    expect(data[1]).to.equal(0x01); // bDescriptorType: device
});

test('USB_RECOVERY_04_CANCEL_ON_SETUP_CHECK', async function() {
    // Device validates
});

test('USB_RECOVERY_05_CDC_TX_STORM_START', async function() {
    // Device starts CDC TX thread
});

test('USB_RECOVERY_06_CDC_TX_STORM_TRIGGER', async function() {
    // Only Linux strands the CDC IN transfer when the port isn't read
    if (process.platform !== 'linux') {
        return;
    }
    // The host holds the CDC port open and writes to it without reading the device's
    // output. Each write reopens the device's TX side after its CDC TX timeout closed it. No control requests here, so
    // the EP0 watchdog has nothing to act on. The device measures its own starvation
    const UNREAD_MS = 15000;
    const WRITE_INTERVAL_MS = 10;
    const WRITE_TIMEOUT_MS = 2000; // If the device hits this timeout, it is starved
    const RECOVERY_MS = 15000; // Longer than a watchdog repair + reattach + re-enumeration
    const line = Buffer.from('x'.repeat(63) + '\n');

    await cdc.open();
    await sleep(300);  // Let the host read for a moment
    cdc.pauseInput();  // Keep the port open but stop reading: Linux stops collecting CDC IN once its tty buffer fills
    const holdStart = Date.now();
    const until = holdStart + UNREAD_MS;
    let writes = 0;
    let stalledAfterMs = null;
    while (Date.now() < until) {
        const r = await cdcWriteWithTimeout(cdc, line, WRITE_TIMEOUT_MS);
        if (r === 'timeout') {
            stalledAfterMs = Date.now() - holdStart;
            break; // Starved: no need to hold any longer
        }
        writes++;
        await sleep(WRITE_INTERVAL_MS);
    }

    // Release the port. With a write still pending, don't wait on the close for long
    try {
        cdc.resumeInput();
        await Promise.race([cdc.disconnect(), sleep(5000)]);
    } catch (err) {
        console.log(`releasing the port: ${err.message}`);
    }

    expect(stalledAfterMs, `the device stopped accepting CDC data after ${writes} writes (${stalledAfterMs} ms into ` +
            `the hold): its threads were starved while the host held the port unread`).to.be.null;

    // The device must answer again once the host lets go
    const deadline = Date.now() + RECOVERY_MS;
    let answered = false;
    let lastErr = null;
    for (let i = 0; !answered && Date.now() < deadline; i++) {
        try {
            const data = `after-${i}`;
            const reply = await systemEcho(await device.getUsbDevice(), data, 2000);
            answered = reply.result === 0 && reply.data.toString() === data;
            if (!answered) {
                lastErr = new Error(`echo returned result ${reply.result}, data "${reply.data}"`);
            }
        } catch (err) {
            lastErr = err;
        }
        if (!answered) {
            await sleep(500);
        }
    }
    expect(answered, `the device didn't answer an echo within ${RECOVERY_MS} ms of releasing the port` +
            `${lastErr ? ` (last error: ${lastErr.message})` : ''}`).to.be.true;
});

test('USB_RECOVERY_07_CDC_TX_STORM_CHECK', async function() {
    // Device validates: the device's heartbeat kept running while the port was unread
});
