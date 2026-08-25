'use strict';
/* eslint no-undef: 'off' */

const { UsbCdcPort } = require('../../test/usb_cdc');
const { platformForName } = require('../../test/platform_util');

suite('USB');
platform('gen3', 'gen4');
systemThread('enabled');
timeout(5 * 60 * 1000);

// Control request types (system_control.h)
const CTRL_REQUEST_ECHO = 1;
const CTRL_REQUEST_APP_CUSTOM = 10;
const CTRL_REQUEST_DEVICE_ID = 20;
const CTRL_REQUEST_SYSTEM_VERSION = 30;
const CTRL_REQUEST_DIAGNOSTIC_INFO = 100;

// USB control request channel protocol (usb_control_request_channel.cpp)
const PARTICLE_BREQUEST = 0x50;
const SERVICE_INIT = 1;
const SERVICE_CHECK = 2;
const SERVICE_RESET = 5;
const SERVICE_MIN_WLENGTH = 64;
const STATUS_OK = 0;
const STATUS_PENDING = 2;
const STATUS_NOT_FOUND = 5;

// Microsoft OS descriptors (usbd_wcid.h)
const WCID_VENDOR_CODE = 0xee;
const MSFT_STRING_INDEX = 0xee;
const WCID_GUID = '{20b6cfa4-6dc7-468a-a8db-faa7c23ddea5}';

const HOST_RESET_ITERATIONS = 5;
const FINAL_CDC_DATA = Buffer.from('usb-final');

let device = null;
let cdc = null;
let platformInfo = null;
// The vendor (control) interface is hardcoded to interface 2 on nRF52840 and
// registered first (interface 0) on rtl872x, followed by the CDC comm/data
// interface pair
let isRtl = false;

function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

function echoPayload(size, seed = 0) {
    const chars = '0123456789abcdefghijklmnopqrstuvwxyz';
    let data = '';
    for (let i = 0; i < size; ++i) {
        data += chars[(i + seed) % chars.length];
    }
    return data;
}

// Decodes a null-terminated/padded UTF-16LE descriptor field
function readUtf16Z(buf, offset, length) {
    return buf.subarray(offset, offset + length).toString('utf16le').replace(/\0+$/, '');
}

// Decodes a null-padded ASCII descriptor field
function readAsciiZ(buf, offset, length) {
    return buf.subarray(offset, offset + length).toString('ascii').replace(/\0+$/, '');
}

function rawUsbDevice(usbDev) {
    return usbDev.usbDevice.internalObject;
}

function controlTransfer(usbDev, bmRequestType, bRequest, wValue, wIndex, dataOrLength) {
    return new Promise((resolve, reject) => {
        rawUsbDevice(usbDev).controlTransfer(bmRequestType, bRequest, wValue, wIndex, dataOrLength,
            (err, data) => (err ? reject(err) : resolve(data)));
    });
}

function getStringDescriptor(usbDev, index) {
    return new Promise((resolve, reject) => {
        rawUsbDevice(usbDev).getStringDescriptor(index, (err, value) => (err ? reject(err) : resolve(value)));
    });
}

// Parses the service reply layout defined in usb_control_request_channel.cpp
function parseServiceReply(data) {
    expect(data.length).to.be.at.least(6);
    const rep = {};
    rep.flags = data.readUInt32LE(0);
    expect(rep.flags & 0x01).to.equal(0x01); // STATUS is mandatory
    rep.status = data.readUInt16LE(4);
    let offs = 6;
    if (rep.flags & 0x02) { // ID
        rep.id = data.readUInt16LE(offs);
        offs += 2;
    }
    if (rep.flags & 0x04) { // SIZE
        rep.size = data.readUInt32LE(offs);
        offs += 4;
    }
    if (rep.flags & 0x08) { // RESULT
        rep.result = data.readInt32LE(offs);
        offs += 4;
    }
    return rep;
}

async function systemEcho(usbDev, payload, timeout = 30000) {
    const rep = await usbDev.sendControlRequest(CTRL_REQUEST_ECHO, payload, { timeout });
    expect(rep.result).to.equal(0);
    expect(rep.data).to.equal(payload);
}

// Reopens the runner's USB handle until the control channel answers an echo.
// Covers both the nRF52840 case (the handle survives a bus reset) and the
// rtl872x case (the device detaches and re-enumerates 1-2 s after a reset)
async function recoverControlChannel(timeoutMs = 120000) {
    const deadline = Date.now() + timeoutMs;
    let lastError = null;
    for (;;) {
        try {
            const usbDev = await device.getUsbDevice();
            await systemEcho(usbDev, 'recovery-probe', 5000);
            return usbDev;
        } catch (err) {
            lastError = err;
            try {
                await device.close();
            } catch (_) {
                // Retry with a fresh handle
            }
        }
        if (Date.now() >= deadline) {
            throw new Error(`USB control channel did not recover: ${lastError ? lastError.message : 'timeout'}`);
        }
        await sleep(500);
    }
}

before(function() {
    device = this.particle.devices[0];
    if (!device) {
        throw new Error('No device assigned to this test');
    }
    platformInfo = platformForName(device.platform.name);
    isRtl = platformInfo.baseMcu === 'rtl872x';
    cdc = new UsbCdcPort(device.id);
});

after(async function() {
    if (cdc) {
        await cdc.close();
    }
});

test('USB_00_SystemEchoShort', async function() {
    const usbDev = await device.getUsbDevice();
    // Payloads up to USB_REQUEST_MAX_POOLED_BUFFER_SIZE (64) are allocated
    // synchronously from the system pool in ISR context
    for (const size of [1, 63, 64]) {
        await systemEcho(usbDev, echoPayload(size, size));
    }
});

test('USB_01_SystemEchoLarge', async function() {
    const usbDev = await device.getUsbDevice();
    // Payloads above 64 bytes are allocated asynchronously on the device;
    // above 4096 bytes the host splits the payload into multiple SEND/RECV
    // control transfers
    for (const size of [65, 512, 4096, 4097, 10000]) {
        await systemEcho(usbDev, echoPayload(size, size));
    }
});

test('USB_02_SystemEchoConcurrent', async function() {
    const usbDev = await device.getUsbDevice();
    // The device serves up to USB_REQUEST_MAX_ACTIVE_COUNT (4) requests
    const payloads = [0, 1, 2, 3].map((i) => echoPayload(512 + i * 256, i));
    const reps = await Promise.all(payloads.map((p) => usbDev.sendControlRequest(CTRL_REQUEST_ECHO, p, { timeout: 30000 })));
    for (let i = 0; i < reps.length; ++i) {
        expect(reps[i].result).to.equal(0);
        expect(reps[i].data).to.equal(payloads[i]);
    }
});

test('USB_03_AppCustomEcho', async function() {
    const usbDev = await device.getUsbDevice();
    // Unlike CTRL_REQUEST_ECHO, this round-trips through the application's
    // custom request handler (the test runner's request handler on the device)
    for (const size of [16, 6000]) {
        const payload = echoPayload(size, size);
        const rep = await usbDev.sendControlRequest(CTRL_REQUEST_APP_CUSTOM, JSON.stringify({ c: 'e', d: payload }),
            { timeout: 30000 });
        expect(rep.result).to.equal(0);
        expect(JSON.parse(rep.data)).to.equal(payload);
    }
});

test('USB_04_AppCustomEchoDeferred', async function() {
    const usbDev = await device.getUsbDevice();
    // Each request is completed by the device application loop after the
    // requested delay, so the host observes it as PENDING across multiple
    // CHECK polls. The first-issued request has the longest delay, making the
    // completion order the reverse of the issue order.
    const delays = [2500, 1600, 900, 300];
    const start = Date.now();
    const completionOrder = [];
    const reps = await Promise.all(delays.map(async (delay, i) => {
        const payload = echoPayload(128, i);
        const rep = await usbDev.sendControlRequest(CTRL_REQUEST_APP_CUSTOM,
            JSON.stringify({ c: 'e', d: payload, t: delay }), { timeout: 30000 });
        expect(Date.now() - start).to.be.at.least(delay);
        completionOrder.push(i);
        return { rep, payload };
    }));
    for (const { rep, payload } of reps) {
        expect(rep.result).to.equal(0);
        expect(JSON.parse(rep.data)).to.equal(payload);
    }
    expect(completionOrder).to.deep.equal([3, 2, 1, 0]);
});

test('USB_05_DiagnosticInfo', async function() {
    const usbDev = await device.getUsbDevice();
    const rep = await usbDev.sendControlRequest(CTRL_REQUEST_DIAGNOSTIC_INFO, null, { timeout: 30000 });
    expect(rep.result).to.equal(0);
    // The reply is JSON-formatted diagnostic data
    const info = JSON.parse(rep.data.toString());
    expect(info).to.be.an('object').that.is.not.empty;
});

test('USB_06_RawVendorRequests', async function() {
    expect(device.mailBox).to.have.lengthOf(1);
    const version = device.mailBox[0].d;
    expect(version).to.be.a('string').that.is.not.empty;
    const usbDev = await device.getUsbDevice();
    // Synchronous vendor requests served directly from the EP0 setup handler
    const id = await controlTransfer(usbDev, 0xc0, PARTICLE_BREQUEST, 0, CTRL_REQUEST_DEVICE_ID, SERVICE_MIN_WLENGTH);
    expect(id.toString().toLowerCase()).to.equal(device.id.toLowerCase());
    const ver = await controlTransfer(usbDev, 0xc0, PARTICLE_BREQUEST, 0, CTRL_REQUEST_SYSTEM_VERSION, SERVICE_MIN_WLENGTH);
    expect(ver.toString()).to.equal(version);
});

test('USB_07_ServiceRequestCancellation', async function() {
    const usbDev = await device.getUsbDevice();
    // Start an ECHO request but never send its payload
    let rep = parseServiceReply(await controlTransfer(usbDev, 0xc0, SERVICE_INIT, 128 /* payload size */,
        CTRL_REQUEST_ECHO, SERVICE_MIN_WLENGTH));
    expect(rep.status).to.be.oneOf([STATUS_OK, STATUS_PENDING]);
    expect(rep.id).to.be.a('number');
    const id = rep.id;
    try {
        // Wait for the asynchronous buffer allocation to complete
        const deadline = Date.now() + 5000;
        while (rep.status === STATUS_PENDING) {
            expect(Date.now()).to.be.below(deadline);
            await sleep(100);
            rep = parseServiceReply(await controlTransfer(usbDev, 0xc0, SERVICE_CHECK, 0, id, SERVICE_MIN_WLENGTH));
        }
        expect(rep.status).to.equal(STATUS_OK);
    } finally {
        // Always cancel so a failure does not leak one of the 4 request slots
        rep = parseServiceReply(await controlTransfer(usbDev, 0xc0, SERVICE_RESET, 0, id, SERVICE_MIN_WLENGTH));
    }
    expect(rep.status).to.equal(STATUS_OK);
    // The device no longer knows about the cancelled request
    rep = parseServiceReply(await controlTransfer(usbDev, 0xc0, SERVICE_CHECK, 0, id, SERVICE_MIN_WLENGTH));
    expect(rep.status).to.equal(STATUS_NOT_FOUND);
});

test('USB_08_DeviceDescriptor', async function() {
    const usbDev = await device.getUsbDevice();
    const desc = rawUsbDevice(usbDev).deviceDescriptor;
    expect(desc.idVendor).to.equal(Number.parseInt(platformInfo.usb.vendorId, 16));
    expect(desc.idProduct).to.equal(Number.parseInt(platformInfo.usb.productId, 16));
    const serial = await getStringDescriptor(usbDev, desc.iSerialNumber);
    expect(serial.toLowerCase()).to.equal(device.id.toLowerCase());
});

test('USB_09_InterfaceLayout', async function() {
    const usbDev = await device.getUsbDevice();
    const conf = rawUsbDevice(usbDev).configDescriptor;
    const byNumber = new Map();
    for (const alts of conf.interfaces) {
        for (const iface of alts) {
            if (iface.bAlternateSetting === 0) {
                byNumber.set(iface.bInterfaceNumber, iface);
            }
        }
    }
    const vendorIface = byNumber.get(isRtl ? 0 : 2);
    const cdcCommIface = byNumber.get(isRtl ? 1 : 0);
    const cdcDataIface = byNumber.get(isRtl ? 2 : 1);
    expect(vendorIface, 'vendor control interface').to.exist;
    expect(vendorIface.bInterfaceClass).to.equal(0xff);
    expect(vendorIface.bInterfaceSubClass).to.equal(0xff);
    expect(vendorIface.bInterfaceProtocol).to.equal(0xff);
    expect(cdcCommIface, 'CDC communications interface').to.exist;
    expect(cdcCommIface.bInterfaceClass).to.equal(0x02); // Communications
    expect(cdcDataIface, 'CDC data interface').to.exist;
    expect(cdcDataIface.bInterfaceClass).to.equal(0x0a); // CDC Data
});

test('USB_10_MsftOsStringDescriptor', async function() {
    const usbDev = await device.getUsbDevice();
    // The MS OS string descriptor is a regular string descriptor at index 0xee:
    // bLength, bDescriptorType, qwSignature ("MSFT100"), bMS_VendorCode, padding
    const data = await controlTransfer(usbDev, 0x80, 0x06 /* GET_DESCRIPTOR */, 0x0300 | MSFT_STRING_INDEX, 0, 255);
    expect(data.readUInt8(0)).to.equal(18); // bLength
    expect(data.readUInt8(1)).to.equal(0x03); // bDescriptorType: string
    expect(readUtf16Z(data, 2, 14)).to.equal('MSFT100'); // qwSignature
    expect(data.readUInt8(16)).to.equal(WCID_VENDOR_CODE); // bMS_VendorCode
});

test('USB_11_WcidCompatIdDescriptor', async function() {
    const usbDev = await device.getUsbDevice();
    // Extended Compat ID OS descriptor: a 16-byte header followed by one
    // 24-byte function section (usbd_wcid.h)
    const data = await controlTransfer(usbDev, 0xc0, WCID_VENDOR_CODE, 0, 0x0004 /* Extended Compat ID */, 0x28);
    expect(data.length).to.equal(0x28);
    expect(data.readUInt32LE(0)).to.equal(0x28); // dwLength
    expect(data.readUInt16LE(4)).to.equal(0x0100); // bcdVersion
    expect(data.readUInt16LE(6)).to.equal(0x0004); // wIndex
    expect(data.readUInt8(8)).to.equal(1); // bCount, followed by 7 reserved bytes
    const func = data.subarray(16); // The single function section
    expect(func.readUInt8(0)).to.equal(isRtl ? 0 : 2); // bFirstInterfaceNumber (the vendor control interface)
    expect(readAsciiZ(func, 2, 8)).to.equal('WINUSB'); // compatibleID
    expect(readAsciiZ(func, 10, 8)).to.equal(''); // subCompatibleID

    // Extended Properties OS descriptor: a 10-byte header followed by one
    // custom property section holding the DeviceInterfaceGUIDs registry value.
    // It is requested with recipient=interface, which Linux usbfs may refuse
    // to route, so verify opportunistically
    let props = null;
    try {
        props = await controlTransfer(usbDev, 0xc1, WCID_VENDOR_CODE, isRtl ? 0 : 2, 0x0005 /* Extended Properties */, 0x92);
    } catch (err) {
        console.log(`Skipping extended properties descriptor check: ${err.message}`);
        return;
    }
    expect(props.length).to.equal(0x92);
    expect(props.readUInt32LE(0)).to.equal(0x92); // dwLength
    expect(props.readUInt16LE(4)).to.equal(0x0100); // bcdVersion
    expect(props.readUInt16LE(6)).to.equal(0x0005); // wIndex
    expect(props.readUInt16LE(8)).to.equal(1); // wCount
    let offs = 10; // The single custom property section starts here
    offs += 4; // dwSize
    expect(props.readUInt32LE(offs)).to.equal(7); // dwPropertyDataType: REG_MULTI_SZ
    offs += 4;
    const nameLength = props.readUInt16LE(offs); // wPropertyNameLength
    offs += 2;
    expect(readUtf16Z(props, offs, nameLength)).to.equal('DeviceInterfaceGUIDs'); // bPropertyName
    offs += nameLength;
    const dataLength = props.readUInt32LE(offs); // dwPropertyDataLength
    offs += 4;
    expect(offs + dataLength).to.be.at.most(props.length);
    // bPropertyData: a REG_MULTI_SZ holding a single GUID
    expect(readUtf16Z(props, offs, dataLength).toLowerCase()).to.equal(WCID_GUID);
});

test('USB_12_EnterListeningMode', async function() {
    const usbDev = await device.getUsbDevice();
    await usbDev.enterListeningMode();
    expect(await usbDev.getDeviceMode()).to.equal('LISTENING');
    // Control requests keep working in listening mode
    await systemEcho(usbDev, echoPayload(128, 7));
});

test('USB_13_DeviceObservesListeningMode', async function() {
    // The device-side test verified Network.listening() while this side kept
    // the device in listening mode; now leave it
    const usbDev = await device.getUsbDevice();
    await usbDev.leaveListeningMode();
    expect(await usbDev.getDeviceMode()).to.equal('NORMAL');
});

test('USB_14_DeviceObservesNormalMode', async function() {
    // Device-side assertions only
});

test('USB_15_HostBusResetRecovery', async function() {
    for (let i = 0; i < HOST_RESET_ITERATIONS; ++i) {
        const usbDev = await recoverControlChannel();
        try {
            await new Promise((resolve, reject) => {
                rawUsbDevice(usbDev).reset((err) => (err ? reject(err) : resolve()));
            });
        } catch (err) {
            // Expected on rtl872x: a bus reset makes the device detach and
            // re-attach its whole USB stack, invalidating the handle
            console.log(`Bus reset ${i}: ${err.message}`);
        }
        await sleep(isRtl ? 2000 : 500);
    }
    await recoverControlChannel();
});

test('USB_16_DeviceEndBeginStress', async function() {
    // The device ran several Serial.end()/begin() re-enumeration cycles before
    // this test body
    await recoverControlChannel();
    const devicePath = await cdc.open();
    console.log(`Opened USB CDC port ${devicePath} for ${device.id}`);
    cdc.discardInput();
});

test('USB_17_FinalCdcSanity', async function() {
    const data = await cdc.read(FINAL_CDC_DATA.length);
    expect(data.equals(FINAL_CDC_DATA)).to.be.true;
    const usbDev = await device.getUsbDevice();
    await systemEcho(usbDev, echoPayload(512, 15));
});
