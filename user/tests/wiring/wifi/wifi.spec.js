'use strict';
/* eslint no-undef: 'off' */

const { WifiAp, WifiApUnavailableError, ACQUIRE_TIMEOUT } = require('../../test/wifi_ap');

suite('Wi-Fi');
platform('wifi');
systemThread('enabled');
tag('wifi_ap');

const STATION_TIMEOUT = 60000;
const CLEANUP_TIMEOUT = 180000;

const WNM_CATEGORY = 10;
const WNM_BTM_REQUEST = 7;
const BTM_BSS_TERMINATION_INCLUDED = 0x08;
const BTM_ESS_DISASSOC_IMMINENT = 0x10;
const BSS_TERMINATION_DURATION_SUBELEM = 4;
const BAD_URL_LEN = 200;

const RADIO_MEASUREMENT_CATEGORY = 5;
const RADIO_MEASUREMENT_REQUEST = 0;
const MEASUREMENT_REQUEST_ELEM = 38;
const BEACON_MEASUREMENT = 5;
const BEACON_SSID_SUBELEM = 0;
const OPERATING_CLASS_2G = 81;
const BEACON_TABLE_MODE = 2;
const OVERSIZED_SSID_LEN = 193;

let device = null;
let ap = null;
let apCredentials = null;
let sta = null;
let restored = false;

async function runDeviceTest(name) {
    await device.startTest(name);
    return device.waitTest({ timeout: CLEANUP_TIMEOUT });
}

function btmRequest(mode, optional) {
    return Buffer.concat([Buffer.from([WNM_CATEGORY, WNM_BTM_REQUEST, 1, mode, 0, 0, 0xff]), optional]);
}

function beaconRequest(ssid) {
    const beacon = Buffer.concat([
        Buffer.from([OPERATING_CLASS_2G, 0, 0, 0, 100, 0, BEACON_TABLE_MODE]),
        Buffer.alloc(6, 0xff),
        Buffer.from([BEACON_SSID_SUBELEM, ssid.length]),
        ssid
    ]);
    return Buffer.concat([
        Buffer.from([RADIO_MEASUREMENT_CATEGORY, RADIO_MEASUREMENT_REQUEST, 1, 0, 0]),
        Buffer.from([MEASUREMENT_REQUEST_ELEM, 3 + beacon.length, 1, 0, BEACON_MEASUREMENT]),
        beacon
    ]);
}

async function sendAction(ctx, body) {
    try {
        await ap.sendAction(sta, body);
    } catch (e) {
        if (!(e instanceof WifiApUnavailableError)) {
            throw e;
        }
        console.log(`Skipping: ${e.message}`);
        ctx.skip();
    }
}

before(async function() {
    this.timeout(ACQUIRE_TIMEOUT);
    device = this.particle.devices[0];
    ap = null;
    apCredentials = null;
    sta = null;
    restored = false;
    try {
        ap = await WifiAp.acquire();
    } catch (e) {
        if (!(e instanceof WifiApUnavailableError)) {
            throw e;
        }
        console.log(`Skipping: ${e.message}`);
        this.skip();
    }
});

after(async function() {
    if (!ap) {
        return;
    }
    if (!restored) {
        try {
            await runDeviceTest('WIFI_97_reset');
            const r = await runDeviceTest('WIFI_98_cleanup');
            console.log(`Restoring Wi-Fi credentials: ${r.result.description}`);
        } catch (e) {
            console.log(`Restoring Wi-Fi credentials failed: ${e.message}`);
        }
    }
    ap.release();
    ap = null;
});

test('WIFI_00_prepare', async function() {
});

test('WIFI_01_start_ap', async function() {
    apCredentials = await ap.start();
    const usbDevice = await device.getUsbDevice();
    await usbDevice.setWifiCredentials({ ssid: apCredentials.ssid, security: 'WPA2_PSK', password: apCredentials.passphrase });
});

test('WIFI_02_connect_to_cloud_through_ap', async function() {
    sta = await ap.waitStation(STATION_TIMEOUT);
});

test('WIFI_10_send_btm_request_with_bss_termination', async function() {
    const duration = Buffer.concat([Buffer.from([BSS_TERMINATION_DURATION_SUBELEM, 10]), Buffer.alloc(8), Buffer.from([1, 0])]);
    await sendAction(this, btmRequest(BTM_BSS_TERMINATION_INCLUDED, duration));
});

test('WIFI_11_connected_after_btm_request_with_bss_termination', async function() {
});

test('WIFI_12_send_btm_request_with_bad_url_length', async function() {
    await sendAction(this, btmRequest(BTM_ESS_DISASSOC_IMMINENT, Buffer.from([BAD_URL_LEN, 0x61, 0x62, 0x63, 0x64])));
});

test('WIFI_13_connected_after_btm_request_with_bad_url_length', async function() {
});

test('WIFI_14_send_beacon_request', async function() {
    await sendAction(this, beaconRequest(Buffer.from(apCredentials.ssid)));
});

test('WIFI_15_connected_after_beacon_request', async function() {
});

test('WIFI_16_send_beacon_request_with_oversized_ssid', async function() {
    await sendAction(this, beaconRequest(Buffer.alloc(OVERSIZED_SSID_LEN, 0x61)));
});

test('WIFI_17_connected_after_beacon_request_with_oversized_ssid', async function() {
});

test('WIFI_20_disable_ap', async function() {
    await ap.disable();
});

test('WIFI_21_no_coverage_connection_attempts_back_off_without_radio_reset', async function() {
});

test('WIFI_22_no_coverage_new_credentials_reset_backoff', async function() {
});

test('WIFI_23_enable_ap', async function() {
    await ap.enable();
});

test('WIFI_24_reconnect_when_ap_returns', async function() {
});

test('WIFI_97_reset', async function() {
});

test('WIFI_98_cleanup', async function() {
    restored = true;
});

test('WIFI_99_reconnect_with_original_credentials', async function() {
});
