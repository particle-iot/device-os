'use strict';
/* eslint no-undef: 'off' */

const { Tether, loadRigConfig, rigEntryForDevice, adapterSerial } =
    require('../../test/tethering');

suite('Tethering Serial1');
platform('b5som', 'msom');
systemThread('enabled');
tag('fixture');

const RAT = {
    UNKNOWN: 0,
    NONE: 0,
    WIFI: 1,
    GSM: 2,
    EDGE: 3,
    UMTS: 4,
    CDMA: 5,
    LTE: 6,
    LTE_CAT_M1: 8,
    LTE_CAT_NB1: 9,
};

const THRESHOLDS = {
    [RAT.LTE]: 100000,
    [RAT.LTE_CAT_M1]: 30000,
    [RAT.LTE_CAT_NB1]: 20000,
    [RAT.UMTS]: 50000,
    [RAT.EDGE]: 20000,
    [RAT.GSM]: 20000,
    default: 50000,
};

// Raw tether-link floors, per platform and direction, derived from the line
// rate: 921600 baud 8N1 -> ~737 kbps of payload. Down (device->host) runs near
// line rate on both platforms; up (host->device) pays for device-side RX
// scheduling/backpressure, so its floor is lower.
const SERIAL1_LINE_BPS = 921600 * 8 / 10;
const LINK_BUDGET_BPS = {
    b5som: { down: Math.round(SERIAL1_LINE_BPS * 0.75), up: Math.round(SERIAL1_LINE_BPS * 0.40) },
    msom: { down: Math.round(SERIAL1_LINE_BPS * 0.75), up: Math.round(SERIAL1_LINE_BPS * 0.40) },
    default: { down: Math.round(SERIAL1_LINE_BPS * 0.50), up: Math.round(SERIAL1_LINE_BPS * 0.30) },
};

const DOWNLOAD_RUNS = 10;

let device = null;
let tether = null;
let minBps = 0;
let modemInfo = null;
let sustainDownBps = 0;
let sustainUpBps = 0;
// Per-test results accumulated for the final report/sanity checks (test 21)
const finalStats = {};

before(function() {
    if (!process.env.TETHERING_DOCKER_IMAGE) {
        throw new Error('TETHERING_DOCKER_IMAGE is not set');
    }
    device = this.particle.devices[0];
    if (!device) {
        throw new Error('No device assigned to this test');
    }
});

after(async function() {
    if (tether) {
        await tether.cleanup();
    }
});

test('01_TETHERING_SERIAL1_setup_and_bind', async function() {
    expect(device.mailBox).to.not.be.empty;
    modemInfo = JSON.parse(device.mailBox.shift().d);
    console.log('Modem info:', JSON.stringify(modemInfo));
    minBps = THRESHOLDS[modemInfo.rat] || THRESHOLDS.default;

    // TETHERING_ADAPTER_SERIAL alone is enough (local runs); the rig file is
    // only consulted as a fallback, so don't require RIG_FILE when the env
    // var is set.
    let serial = process.env.TETHERING_ADAPTER_SERIAL || '';
    if (!serial) {
        const rigEntry = rigEntryForDevice(loadRigConfig(), device.id);
        serial = adapterSerial(rigEntry);
    }
    if (!serial) {
        throw new Error(
            `Missing adapter serial for device ${device.id}. ` +
            `Set TETHERING_ADAPTER_SERIAL env var or ` +
            `tethering.adapterSerial in the rig file.`
        );
    }

    tether = new Tether('serial1', device.id, serial);
    await tether.start();
});

test('02_TETHERING_SERIAL1_resolve_port', async function() {
    tether.port = await tether.resolveTty(tether.adapterSerial);
    console.log(`Resolved port: ${tether.port}`);
});

test('03_TETHERING_SERIAL1_configure_port', async function() {
    await tether.setupPort({ baud: 921600, flowControl: 'rts-cts' });
});

test('04_TETHERING_SERIAL1_wait_for_modem', async function() {
    await tether.waitForModem();
    console.log('Particle modem detected');
});

test('05_TETHERING_SERIAL1_wait_for_ppp0', async function() {
    await tether.waitForPpp0();
    console.log('ppp0 is UP');
});

test('06_TETHERING_SERIAL1_verify_reachability', async function() {
    // Retry until ppp0 is reachable and peer IP resolves.
    // The PPP link may need a moment to stabilize after ppp0 comes up.
    let ok = false;
    for (let i = 0; i < 10 && !ok; i++) {
        try {
            await tether.ping();
            await tether.resolvePeerIp();
            ok = true;
        } catch (err) {
            console.log(`    verify_reachability attempt ${i + 1}/10 failed: ${err.message.split('\n')[0]}`);
            await new Promise(r => setTimeout(r, 2000));
        }
    }
    if (!ok) {
        throw new Error('ppp0 not reachable after 10 attempts');
    }
    console.log('ppp0 is reachable, tether IP:', tether.tetherIp);
});

test('07_TETHERING_SERIAL1_start_iperf_server', async function() {
    // Device-side starts iperf server
});

test('08_TETHERING_SERIAL1_iperf3_udp_sustain_down', async function() {
    // Serial1 @ 921600 baud -> ~737000 bps with 8N1 framing. Start there.
    if (!tether.tetherIp) {
        throw new Error('tetherIp not resolved - test 06 must succeed first');
    }
    await tether.ping({ host: tether.tetherIp });
    const result = await tether.iperf3UdpSustain({
        direction: 'down', startBitrate: 1000000, time: 10, verifyTime: 30
    });
    sustainDownBps = result.bitrate;
    console.log(
        `UDP sustain down (device->host): ` +
        `${Math.round(result.bitrate / 1000)} kbps, ` +
        `loss: ${result.loss}% ` +
        `(${result.iterations} iterations, ${result.attempts} attempts)`
    );
    this.particle.data = {
        type: 'iperf3_udp_sustain', direction: 'down',
        bitrate: result.bitrate, loss: result.loss,
        iterations: result.iterations, attempts: result.attempts
    };
    finalStats.sustainDown = { bps: Math.round(result.bitrate), loss: result.loss };
    const min = (LINK_BUDGET_BPS[device.platform?.name] || LINK_BUDGET_BPS.default).down;
    expect(result.bitrate).to.be.at.least(min,
        `Sustained down speed ${result.bitrate} bps is below minimum ${min} bps`);
    await tether.ping({ host: tether.tetherIp });
});

test('09_TETHERING_SERIAL1_iperf3_udp_sustain_down_small', async function() {
    // Small packet size (64 bytes) - real-time/telemetry profile
    await tether.ping({ host: tether.tetherIp });
    const result = await tether.iperf3UdpSustain({
        direction: 'down', startBitrate: 1000000, time: 10, verifyTime: 30, blksize: 64
    });
    console.log(
        `UDP sustain down 64B (device->host): ` +
        `${Math.round(result.bitrate / 1000)} kbps, ` +
        `loss: ${result.loss}% ` +
        `(${result.iterations} iterations, ${result.attempts} attempts)`
    );
    this.particle.data = {
        type: 'iperf3_udp_sustain', direction: 'down', blksize: 64,
        bitrate: result.bitrate, loss: result.loss,
        iterations: result.iterations, attempts: result.attempts
    };
    finalStats.sustainDownSmall = { bps: Math.round(result.bitrate), loss: result.loss };
    await tether.ping({ host: tether.tetherIp });
});

test('10_TETHERING_SERIAL1_iperf3_udp_sustain_up', async function() {
    await tether.ping({ host: tether.tetherIp });
    const result = await tether.iperf3UdpSustain({
        direction: 'up', startBitrate: 1000000, time: 10, verifyTime: 30
    });
    sustainUpBps = result.bitrate;
    console.log(
        `UDP sustain up (host->device): ` +
        `${Math.round(result.bitrate / 1000)} kbps, ` +
        `loss: ${result.loss}% ` +
        `(${result.iterations} iterations, ${result.attempts} attempts)`
    );
    this.particle.data = {
        type: 'iperf3_udp_sustain', direction: 'up',
        bitrate: result.bitrate, loss: result.loss,
        iterations: result.iterations, attempts: result.attempts
    };
    finalStats.sustainUp = { bps: Math.round(result.bitrate), loss: result.loss };
    const min = (LINK_BUDGET_BPS[device.platform?.name] || LINK_BUDGET_BPS.default).up;
    expect(result.bitrate, `Sustained up speed ${result.bitrate} bps is below minimum ${min} bps`).to.be.at.least(min);
    expect(result.loss, `Sustained up loss ${result.loss}% exceeds 5%`).to.be.at.most(5);
    await tether.ping({ host: tether.tetherIp });
});

test('11_TETHERING_SERIAL1_iperf3_udp_sustain_up_small', async function() {
    // Small packet size (64 bytes) - real-time/telemetry profile
    await tether.ping({ host: tether.tetherIp });
    const result = await tether.iperf3UdpSustain({
        direction: 'up', startBitrate: 1000000, time: 10, verifyTime: 30, blksize: 64
    });
    console.log(
        `UDP sustain up 64B (host->device): ` +
        `${Math.round(result.bitrate / 1000)} kbps, ` +
        `loss: ${result.loss}% ` +
        `(${result.iterations} iterations, ${result.attempts} attempts)`
    );
    this.particle.data = {
        type: 'iperf3_udp_sustain', direction: 'up', blksize: 64,
        bitrate: result.bitrate, loss: result.loss,
        iterations: result.iterations, attempts: result.attempts
    };
    finalStats.sustainUpSmall = { bps: Math.round(result.bitrate), loss: result.loss };
    await tether.ping({ host: tether.tetherIp });
});

test('12_TETHERING_SERIAL1_iperf3_tcp_down', async function() {
    await tether.ping({ host: tether.tetherIp });
    const { json, attempts } = await tether.iperf3({ proto: 'tcp', direction: 'down', time: 30 });
    const sent = json.end.sum_sent;
    const recv = json.end.sum_received;
    console.log(`TCP down (device->host) (attempt ${attempts})`);
    console.log(`  host (receiver): ${Math.round(recv.bits_per_second / 1000)} kbps, ${recv.bytes} bytes`);
    console.log(`  device (sender): ${Math.round(sent.bits_per_second / 1000)} kbps, ${sent.bytes} bytes, retransmits: ${sent.retransmits}`);
    this.particle.data = {
        type: 'iperf3_tcp', direction: 'down',
        senderBps: sent.bits_per_second, receiverBps: recv.bits_per_second,
        retransmits: sent.retransmits, attempts
    };
    finalStats.tcpDown = { bps: Math.round(recv.bits_per_second), retransmits: sent.retransmits };
    await tether.ping({ host: tether.tetherIp });
});

test('13_TETHERING_SERIAL1_iperf3_tcp_up', async function() {
    await tether.ping({ host: tether.tetherIp });
    const { json, attempts } = await tether.iperf3({ proto: 'tcp', direction: 'up', time: 30 });
    const sent = json.end.sum_sent;
    const recv = json.end.sum_received;
    console.log(`TCP up (host->device) (attempt ${attempts})`);
    console.log(`  host (sender): ${Math.round(sent.bits_per_second / 1000)} kbps, ${sent.bytes} bytes, retransmits: ${sent.retransmits}`);
    console.log(`  device (receiver): ${Math.round(recv.bits_per_second / 1000)} kbps, ${recv.bytes} bytes`);
    this.particle.data = {
        type: 'iperf3_tcp', direction: 'up',
        senderBps: sent.bits_per_second, receiverBps: recv.bits_per_second,
        retransmits: sent.retransmits, attempts
    };
    finalStats.tcpUp = { bps: Math.round(recv.bits_per_second), retransmits: sent.retransmits };
    await tether.ping({ host: tether.tetherIp });
});

test('14_TETHERING_SERIAL1_stop_iperf_server', async function() {
    // Device-side stops iperf server
});

test('15_TETHERING_SERIAL1_download_speeds', async function() {
    await tether.ping({ host: tether.tetherIp });
    console.log(`[TEST_FILE_DOWNLOAD_SPEED_01] Downloading via ppp0 (min ${minBps} bps)`);
    const { avgBps, samples } = await tether.downloadSpeedTest(minBps, { runs: DOWNLOAD_RUNS });
    console.log(`Measured ${avgBps} bps (min ${minBps} bps)`);
    console.log(`Samples: ${JSON.stringify(samples)}`);
    this.particle.data = { type: 'download_speed', avgBps, minBps, samples };
    finalStats.downloadSpeeds = { bps: avgBps };
    expect(avgBps).to.be.at.least(minBps,
        `Average ${avgBps} bps is below minimum ${minBps} bps`);
    await tether.ping({ host: tether.tetherIp });
});

test('16_TETHERING_SERIAL1_upload_speeds', async function() {
    await tether.ping({ host: tether.tetherIp });
    const bps = minBps;
    console.log(`[TEST_FILE_UPLOAD_SPEED_01] Uploading via ppp0 (min ${bps} bps)`);
    const { avgBps, samples } = await tether.uploadSpeedTest(bps, { runs: DOWNLOAD_RUNS, sizeKb: 100 });
    console.log(`Measured ${avgBps} bps (min ${bps} bps)`);
    console.log(`Samples: ${JSON.stringify(samples)}`);
    this.particle.data = { type: 'upload_speed', avgBps, bps, samples };
    finalStats.uploadSpeeds = { bps: avgBps };
    expect(avgBps).to.be.at.least(bps,
        `Average ${avgBps} bps is below minimum ${bps} bps`);
    await tether.ping({ host: tether.tetherIp });
});

test('17_TETHERING_SERIAL1_iperf3_udp_full_path_down', async function() {
    await tether.ping({ host: tether.tetherIp });

    // Device-side result first
    expect(device.mailBox).to.not.be.empty;
    const deviceRaw = device.mailBox.shift().d;
    console.log(`UDP full-path down (device direct)`);
    const deviceJson = JSON.parse(deviceRaw);
    const dEnd = deviceJson.end || {};
    // Prefer sum_received: it's what the device itself actually received (and
    // what the on-device retry gate is based on). end.sum can be zeroed when
    // the control connection dies at the results exchange even though the data
    // phase completed fine.
    const ds = dEnd.sum_received || dEnd.sum || { bits_per_second: 0, bytes: 0, lost_percent: 0, jitter_ms: 0 };
    console.log(`  ${Math.round(ds.bits_per_second / 1000)} kbps, ${ds.bytes} bytes, loss: ${ds.lost_percent || 0}%, jitter: ${ds.jitter_ms || 0}ms`);

    // Container-side: through tether -> device -> cellular -> public server
    const bitrate = sustainDownBps > 0 ? sustainDownBps : 1000000;
    const { json, attempts } = await tether.iperf3UdpFullPath({ direction: 'down', bitrate, time: 20 });
    // For a down test report what the container actually RECEIVED (end.sum's
    // bits_per_second is the sender's blast rate, which only reflects the
    // server's -b target). This matches the device-direct line above, which
    // reports the device's received rate.
    const s = json.end.sum_received || json.end.sum;
    const sLoss = (json.end.sum && json.end.sum.lost_percent) || s.lost_percent || 0;
    const sJitter = (json.end.sum && json.end.sum.jitter_ms) || s.jitter_ms || 0;
    console.log(`UDP full-path down (container via tether) (attempt ${attempts})`);
    console.log(`  ${Math.round(s.bits_per_second / 1000)} kbps, ${s.bytes} bytes, loss: ${sLoss}%, jitter: ${sJitter}ms`);

    this.particle.data = {
        type: 'iperf3_udp_full_path', direction: 'down',
        containerBps: s.bits_per_second, containerLoss: sLoss, containerJitter: sJitter,
        deviceBps: ds.bits_per_second, deviceLoss: ds.lost_percent, deviceJitter: ds.jitter_ms,
        attempts
    };
    finalStats.fullPathUdpDown = { deviceBps: Math.round(ds.bits_per_second), containerBps: Math.round(s.bits_per_second), containerLoss: sLoss };
    await tether.ping({ host: tether.tetherIp });
});

test('18_TETHERING_SERIAL1_iperf3_udp_full_path_up', async function() {
    await tether.ping({ host: tether.tetherIp });

    // Device-side result first
    expect(device.mailBox).to.not.be.empty;
    const deviceRaw = device.mailBox.shift().d;
    console.log(`UDP full-path up (device direct)`);
    const deviceJson = JSON.parse(deviceRaw);
    const dEnd = deviceJson.end || {};
    // sum_sent is the OFFERED load (what the sender pushed into the path);
    // sum_received is what the server actually got, reported back in the
    // results exchange - the real delivered throughput. A sender that outruns
    // the bottleneck (e.g. the container blasting 1 Mbps into ppp0 while the
    // modem UART carries ~340 kbps) shows a big offered number with the excess
    // as loss, so sum_sent alone is misleading.
    const ds = dEnd.sum_sent || { bits_per_second: 0, bytes: 0 };
    const dr = dEnd.sum_received || { bits_per_second: 0, bytes: 0 };
    console.log(`  sender (offered): ${Math.round(ds.bits_per_second / 1000)} kbps, ${ds.bytes} bytes`);
    console.log(`  receiver (delivered): ${Math.round(dr.bits_per_second / 1000)} kbps, ${dr.bytes} bytes, loss: ${dr.lost_percent || 0}%, jitter: ${dr.jitter_ms || 0}ms`);

    // Container-side
    const bitrate = sustainUpBps > 0 ? sustainUpBps : 1000000;
    const { json, attempts } = await tether.iperf3UdpFullPath({ direction: 'up', bitrate, time: 20 });
    const s = json.end.sum_sent || json.end.sum;
    const r = json.end.sum_received || { bits_per_second: 0, bytes: 0 };
    console.log(`UDP full-path up (container via tether) (attempt ${attempts})`);
    console.log(`  sender (offered): ${Math.round(s.bits_per_second / 1000)} kbps, ${s.bytes} bytes`);
    console.log(`  receiver (delivered): ${Math.round(r.bits_per_second / 1000)} kbps, ${r.bytes} bytes, loss: ${r.lost_percent || 0}%, jitter: ${r.jitter_ms || 0}ms`);

    this.particle.data = {
        type: 'iperf3_udp_full_path', direction: 'up',
        containerSenderBps: s.bits_per_second,
        containerReceiverBps: r.bits_per_second,
        containerLoss: r.lost_percent || 0, containerJitter: r.jitter_ms || 0,
        deviceSenderBps: ds.bits_per_second,
        deviceReceiverBps: dr.bits_per_second,
        deviceLoss: dr.lost_percent || 0, deviceJitter: dr.jitter_ms || 0,
        attempts
    };
    finalStats.fullPathUdpUp = { deviceBps: Math.round(dr.bits_per_second), containerBps: Math.round(r.bits_per_second), containerLoss: r.lost_percent || 0 };
    await tether.ping({ host: tether.tetherIp });
});

test('19_TETHERING_SERIAL1_iperf3_tcp_full_path_down', async function() {
    await tether.ping({ host: tether.tetherIp });

    // Device-side result first
    expect(device.mailBox).to.not.be.empty;
    const deviceRaw = device.mailBox.shift().d;
    console.log(`TCP full-path down (device direct)`);
    const deviceJson = JSON.parse(deviceRaw);
    const dEnd = deviceJson.end || {};
    const ds = dEnd.sum_sent || { bits_per_second: 0, retransmits: 0 };
    const dr = dEnd.sum_received || { bits_per_second: 0 };
    console.log(`  sender: ${Math.round(ds.bits_per_second / 1000)} kbps, receiver: ${Math.round(dr.bits_per_second / 1000)} kbps, retransmits: ${ds.retransmits}`);

    // Container-side
    const { json, attempts } = await tether.iperf3TcpFullPath({ direction: 'down', time: 20 });
    const sent = json.end.sum_sent;
    const recv = json.end.sum_received;
    console.log(`TCP full-path down (container via tether) (attempt ${attempts})`);
    console.log(`  sender: ${Math.round(sent.bits_per_second / 1000)} kbps, receiver: ${Math.round(recv.bits_per_second / 1000)} kbps, retransmits: ${sent.retransmits}`);

    this.particle.data = {
        type: 'iperf3_tcp_full_path', direction: 'down',
        containerSenderBps: sent.bits_per_second, containerReceiverBps: recv.bits_per_second,
        containerRetransmits: sent.retransmits,
        deviceSenderBps: ds.bits_per_second, deviceReceiverBps: dr.bits_per_second,
        deviceRetransmits: ds.retransmits, attempts
    };
    finalStats.fullPathTcpDown = { deviceBps: Math.round(dr.bits_per_second), containerBps: Math.round(recv.bits_per_second) };
    await tether.ping({ host: tether.tetherIp });
});

test('20_TETHERING_SERIAL1_iperf3_tcp_full_path_up', async function() {
    await tether.ping({ host: tether.tetherIp });

    // Device-side result first
    expect(device.mailBox).to.not.be.empty;
    const deviceRaw = device.mailBox.shift().d;
    console.log(`TCP full-path up (device direct)`);
    const deviceJson = JSON.parse(deviceRaw);
    const dEnd = deviceJson.end || {};
    const ds = dEnd.sum_sent || { bits_per_second: 0, retransmits: 0 };
    const dr = dEnd.sum_received || { bits_per_second: 0 };
    console.log(`  sender: ${Math.round(ds.bits_per_second / 1000)} kbps, receiver: ${Math.round(dr.bits_per_second / 1000)} kbps, retransmits: ${ds.retransmits}`);

    // Container-side
    const { json, attempts } = await tether.iperf3TcpFullPath({ direction: 'up', time: 20 });
    const sent = json.end.sum_sent;
    const recv = json.end.sum_received;
    console.log(`TCP full-path up (container via tether) (attempt ${attempts})`);
    console.log(`  sender: ${Math.round(sent.bits_per_second / 1000)} kbps, receiver: ${Math.round(recv.bits_per_second / 1000)} kbps, retransmits: ${sent.retransmits}`);

    this.particle.data = {
        type: 'iperf3_tcp_full_path', direction: 'up',
        containerSenderBps: sent.bits_per_second, containerReceiverBps: recv.bits_per_second,
        containerRetransmits: sent.retransmits,
        deviceSenderBps: ds.bits_per_second, deviceReceiverBps: dr.bits_per_second,
        deviceRetransmits: ds.retransmits, attempts
    };
    finalStats.fullPathTcpUp = { deviceBps: Math.round(dr.bits_per_second), containerBps: Math.round(recv.bits_per_second) };
    await tether.ping({ host: tether.tetherIp });
});

test('21_TETHERING_SERIAL1_final_report', async function() {
    const linkBudget = LINK_BUDGET_BPS[device.platform?.name] || LINK_BUDGET_BPS.default;
    const stats = {
        type: 'tethering_final_report',
        mode: 'serial1',
        platform: device.platform ? device.platform.name : null,
        deviceId: device.id,
        modem: modemInfo,
        thresholds: { minBps, linkBudget },
        results: finalStats
    };
    console.log(`Final tethering stats: ${JSON.stringify(stats)}`);
    this.particle.data = stats;

    // Relaxed cross-test sanity checks. The individual tests enforce their own
    // absolute floors; these only catch gross inconsistencies between paths
    // that share a bottleneck.
    const r = finalStats;

    // Tether-local: TCP should reach a sane fraction of the UDP sustain rate in
    // the same direction (catches pathological TCP behavior on the tether link).
    if (r.sustainDown && r.tcpDown) {
        expect(r.tcpDown.bps).to.be.at.least(r.sustainDown.bps * 0.25,
            `tether TCP down ${Math.round(r.tcpDown.bps)} bps < 25% of UDP sustain down ${Math.round(r.sustainDown.bps)} bps`);
    }
    if (r.sustainUp && r.tcpUp) {
        expect(r.tcpUp.bps).to.be.at.least(r.sustainUp.bps * 0.25,
            `tether TCP up ${Math.round(r.tcpUp.bps)} bps < 25% of UDP sustain up ${Math.round(r.sustainUp.bps)} bps`);
    }

    // The tether link must not be the bottleneck of the full path: its local
    // capacity should comfortably exceed what the full cellular path delivered
    // through it.
    if (r.sustainDown && r.fullPathUdpDown && r.fullPathUdpDown.containerBps > 0) {
        expect(r.sustainDown.bps).to.be.at.least(r.fullPathUdpDown.containerBps * 0.8,
            `tether sustain down ${Math.round(r.sustainDown.bps)} bps is below the full-path delivered rate ${Math.round(r.fullPathUdpDown.containerBps)} bps`);
    }

    // Container-through-tether and device-direct cross the same cellular
    // bottleneck, so their delivered rates should be in the same ballpark:
    // tethered performance no worse than 40% of what the device itself gets.
    if (r.fullPathUdpDown && r.fullPathUdpDown.deviceBps > 0 && r.fullPathUdpDown.containerBps > 0) {
        expect(r.fullPathUdpDown.containerBps).to.be.at.least(r.fullPathUdpDown.deviceBps * 0.4,
            `tethered full-path down ${Math.round(r.fullPathUdpDown.containerBps)} bps < 40% of device-direct ${Math.round(r.fullPathUdpDown.deviceBps)} bps`);
    }
    if (r.fullPathUdpUp && r.fullPathUdpUp.deviceBps > 0 && r.fullPathUdpUp.containerBps > 0) {
        expect(r.fullPathUdpUp.containerBps).to.be.at.least(r.fullPathUdpUp.deviceBps * 0.4,
            `tethered full-path up ${Math.round(r.fullPathUdpUp.containerBps)} bps < 40% of device-direct ${Math.round(r.fullPathUdpUp.deviceBps)} bps`);
    }
});
