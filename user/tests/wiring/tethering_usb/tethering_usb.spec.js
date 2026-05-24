'use strict';
/* eslint no-undef: 'off' */

// Tethering-specific glue. The helpers below (constants, rig loader,
// runTetheringTest) are duplicated in ../tethering_serial1/tethering_serial1.spec.js
// because the two scenarios share most of the host-side setup but pass a
// different `mode` and live in sibling test directories. The only thing
// hoisted out is generic docker plumbing (integration/test/docker.js).
// If you change one copy, keep the other in sync.
//
// Required environment (Concourse task sets these):
//   TETHERING_RIG_FILE     — JSON: { devices: [{ id, tethering: { debuggerSerial, budgetSec } }] }
//   TETHERING_DOCKER_IMAGE — published image tag, e.g. particle/tethering-hil:1.0.0
//   TETHERING_SCRIPTS_DIR  — absolute path containing tethering-init.sh and
//                            tethering-tests.sh; bind-mounted as /scripts
//
// Optional:
//   TETHERING_TEST_URL — override the test download URL (default below)

const fs = require('fs');
const path = require('path');
const docker = require('../../test/docker');

const DEFAULT_URL = 'http://technobly.com/100/phrack.29.phk';
const CONTAINER_NAME_PREFIX = 'tethering-hil';
// Outer cap on the docker-run subprocess. The container's own
// tethering-tests.sh enforces tighter per-step timeouts.
const RUN_OVERHEAD_SEC = 300;

function loadRigConfig() {
    const rigFile = process.env.TETHERING_RIG_FILE;
    if (!rigFile) {
        throw new Error('TETHERING_RIG_FILE is not set; the Concourse task must export it');
    }
    return JSON.parse(fs.readFileSync(rigFile, 'utf8'));
}

function rigEntryForDevice(rig, deviceId) {
    const entry = (rig.devices || []).find((d) => d.id === deviceId);
    if (!entry) {
        throw new Error(`No rig entry for device ${deviceId} in ${process.env.TETHERING_RIG_FILE}`);
    }
    return entry;
}

// `ctx` is the Mocha test context (i.e. `this` inside a test function).
// `mode` is 'usb' or 'serial1'.
async function runTetheringTest(ctx, mode) {
    const imageRef = process.env.TETHERING_DOCKER_IMAGE;
    if (!imageRef) {
        throw new Error('TETHERING_DOCKER_IMAGE is not set; the Concourse task must export it');
    }
    const scriptsDir = process.env.TETHERING_SCRIPTS_DIR;
    if (!scriptsDir) {
        throw new Error('TETHERING_SCRIPTS_DIR is not set; the Concourse task must export it');
    }
    if (!fs.existsSync(path.join(scriptsDir, 'tethering-init.sh'))) {
        throw new Error(`TETHERING_SCRIPTS_DIR does not contain tethering-init.sh: ${scriptsDir}`);
    }

    const devices = ctx.particle && ctx.particle.devices;
    if (!devices || devices.length === 0) {
        throw new Error('No devices assigned to this test (expected the device-side counterpart to run first)');
    }
    if (devices.length > 1) {
        throw new Error(`Expected exactly one DUT for tethering test, got ${devices.length}`);
    }
    const dutId = devices[0].id;

    const rigEntry = rigEntryForDevice(loadRigConfig(), dutId);
    const tethering = rigEntry.tethering || {};
    const debuggerSerial = tethering.debuggerSerial || '';
    const budgetSec = tethering.budgetSec;
    if (typeof budgetSec !== 'number') {
        throw new Error(`Missing tethering.budgetSec for device ${dutId} in rig config`);
    }
    if (mode === 'serial1' && !debuggerSerial) {
        throw new Error(`Missing tethering.debuggerSerial for device ${dutId} (required for Serial1 mode)`);
    }

    const url = process.env.TETHERING_TEST_URL || DEFAULT_URL;
    await docker.ensureImage(imageRef);

    const containerName = `${CONTAINER_NAME_PREFIX}-${mode}-${dutId.slice(-8)}-${process.pid}`;
    const containerArgs = [
        '--privileged',
        '--network', 'none',
        '--dns', '8.8.8.8', '--dns', '8.8.4.4',
        '-e', `TETHER_MODE=${mode}`,
        '-e', `DUT_DEVICE_ID=${dutId}`,
        '-e', `DEBUGGER_USB_SERIAL=${debuggerSerial}`,
        '-e', `TEST_BUDGET_SEC=${budgetSec}`,
        '-e', `TEST_URL=${url}`,
        '-v', `${scriptsDir}:/scripts:ro`,
        '-v', '/dev/bus/usb:/dev/bus/usb',
        '--tmpfs', '/run', '--tmpfs', '/run/lock'
    ];
    const timeoutMs = (budgetSec + RUN_OVERHEAD_SEC) * 1000;

    console.log(`Running tethering ${mode} test for ${dutId} (budget ${budgetSec}s, image ${imageRef}, container ${containerName})`);
    const result = await docker.runContainer({
        image: imageRef,
        name: containerName,
        args: containerArgs,
        stream: true,
        timeoutMs
    });

    if (result.killed) {
        throw new Error(
            `Tethering ${mode} test TIMED OUT after ${timeoutMs / 1000}s for ${dutId}\n` +
            'Tail of container output:\n' +
            result.output.split('\n').slice(-50).join('\n')
        );
    }
    if (result.code !== 0) {
        const tail = result.output.split('\n').slice(-50).join('\n');
        throw new Error(
            `Tethering ${mode} test FAILED (exit ${result.code}) for ${dutId}\n` +
            `Tail of container output:\n${tail}`
        );
    }
}

suite('Tethering USB');
platform('b5som', 'msom');
systemThread('enabled');
tag('fixture');

test('01_TETHERING_USB_setup_and_bind', async function() {
    // Device-side handoff: the on-device test brings up cellular and binds
    // tethering to USB CDC. By the time we get here, the device is ready.
});

test('02_TETHERING_USB_test_download_speeds', async function() {
    this.timeout(0); // honor the fixture's own timeout (budget + overhead)
    await runTetheringTest(this, 'usb');
});
