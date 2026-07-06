'use strict';

// Tethering HIL test helper. Shared by wiring/tethering_usb and
// wiring/tethering_serial1 spec files. The actual test step sequence lives
// in each spec.js so it's easy to read top-to-bottom.
//
// Required environment (Concourse task sets these):
//   RIG_FILE              - path to the rig .jsonc file (JSONC, comments allowed)
//   TETHERING_DOCKER_IMAGE - published image tag, e.g. particle/tethering-hil:2.0.0
//
// Optional:
//   TETHERING_TEST_URL       - override the test download URL (default below)
//   TETHERING_ADAPTER_SERIAL - override the USB-to-UART adapter serial
//                              (for Serial1 mode; otherwise read from rig file)

const fs = require('fs');
const { parse: parseJsonc } = require('jsonc-parser');
const docker = require('../test/docker');

const DEFAULT_URL = 'http://technobly.com/100/phrack.29.phk';
const CONTAINER_NAME_PREFIX = 'tethering-hil';
const PARTICLE_MODEM_RE = 'Particle|B5-SoM|M-SoM|Tracker|Boron|Electron';

// iperf3 public server list for full-path tests. Must match the C++ list in
// user/tests/wiring/tethering_usb/iperf3_servers.h (keep them in sync).
const IPERF3_PUBLIC_SERVERS = [
    { host: 'speedtest.nocix.net', port: 5205 },
    { host: 'lax.speedtest.is.cc', port: 5203 },
    { host: 'speedtest.nocix.net', port: 5204 },
    { host: 'spd-uswb.hostkey.com', port: 5201 },
    { host: 'speedtest.nocix.net', port: 5203 },
    { host: 'nyc.speedtest.is.cc', port: 5202 },
    { host: 'speedtest.nocix.net', port: 5202 },
    { host: 'nyc.speedtest.is.cc', port: 5203 },
    { host: 'speedtest.xmission.com', port: 5209 },
    { host: 'lax.speedtest.is.cc', port: 5202 },
    { host: 'speedtest.nocix.net', port: 5201 },
    { host: 'speedtest.xmission.com', port: 5201 },
    { host: 'speedtest.xmission.com', port: 5202 },
];

// Clean up any tethering containers for this process on exit (including Ctrl-C)
process.on('SIGINT', () => cleanupAllContainers());
process.on('SIGTERM', () => cleanupAllContainers());
process.on('exit', () => cleanupAllContainers());

function cleanupAllContainers() {
    // Only remove containers created by THIS process. The container name
    // ends with the PID (see the Tether constructor), so filtering by the
    // PID-specific suffix avoids killing containers belonging to other
    // concurrent test processes on the same host (parallel runs of
    // tethering_usb + tethering_serial1, or b5som + msom simultaneously).
    try {
        const { execSync } = require('child_process');
        const suffix = `-${process.pid}`;
        const out = execSync(
            `docker ps -a --filter "name=${CONTAINER_NAME_PREFIX}" -q`,
            { stdio: 'pipe' }
        ).toString().trim();
        if (!out) return;
        out.split('\n').filter(Boolean).forEach((id) => {
            // Inspect the name; only kill if it ends with this PID's suffix.
            try {
                const name = execSync(`docker inspect --format '{{.Name}}' ${id}`, { stdio: 'pipe' }).toString().trim().replace(/^\//, '');
                if (name.endsWith(suffix)) {
                    execSync(`docker rm -f ${id}`, { stdio: 'pipe' });
                }
            } catch (_) { /* best-effort */ }
        });
    } catch (_) { /* best-effort */ }
}

function loadRigConfig() {
    const rigFile = process.env.RIG_FILE;
    if (!rigFile) {
        throw new Error('RIG_FILE is not set; the Concourse task must export it');
    }
    const raw = fs.readFileSync(rigFile, 'utf8');
    const errors = [];
    const rig = parseJsonc(raw, errors, { allowTrailingComma: true });
    if (errors.length) {
        throw new Error(`Failed to parse rig file ${rigFile}: ${JSON.stringify(errors)}`);
    }
    return rig;
}

function rigEntryForDevice(rig, deviceId) {
    const entry = (rig.devices || []).find((d) => d.id === deviceId);
    if (!entry) {
        throw new Error(`No rig entry for device ${deviceId} in ${process.env.RIG_FILE}`);
    }
    return entry;
}

function adapterSerial(rigEntry) {
    return process.env.TETHERING_ADAPTER_SERIAL || (rigEntry.tethering || {}).adapterSerial || '';
}

function defaultUrl() {
    return process.env.TETHERING_TEST_URL || DEFAULT_URL;
}

function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

async function poll(fn, timeoutMs, intervalMs = 1000, desc = 'condition') {
    const deadline = Date.now() + timeoutMs;
    for (;;) {
        if (await fn()) return;
        if (Date.now() >= deadline) {
            throw new Error(`Timed out waiting for ${desc} (${timeoutMs / 1000}s)`);
        }
        await sleep(intervalMs);
    }
}

class Tether {
    constructor(mode, deviceId, adapterSerial) {
        this.mode = mode;
        this.deviceId = deviceId;
        this.adapterSerial = adapterSerial || '';
        this.url = defaultUrl();
        this.name = `${CONTAINER_NAME_PREFIX}-${mode}-${deviceId.slice(-8)}-${process.pid}`;
        this.imageRef = process.env.TETHERING_DOCKER_IMAGE;
        this.tetherIp = '';
        this.port = '';
    }

    get containerArgs() {
        return [
            '--privileged',
            '--network', 'none',
            '--dns', '8.8.8.8', '--dns', '8.8.4.4',
            '-e', `TETHER_MODE=${this.mode}`,
            '-e', `DUT_DEVICE_ID=${this.deviceId}`,
            '-e', `ADAPTER_USB_SERIAL=${this.adapterSerial}`,
            '-e', `TEST_URL=${this.url}`,
            '-v', '/dev/bus/usb:/dev/bus/usb',
            '--tmpfs', '/run', '--tmpfs', '/run/lock'
        ];
    }

    /** Exec a command (string -> bash -c, array -> direct). Throws on non-zero exit. */
    async exec(cmd, opts) {
        const r = await docker.execContainer({ name: this.name, cmd, ...opts });
        if (process.env.TETHERING_DEBUG) {
            const cmdStr = typeof cmd === 'string' ? cmd : cmd.join(' ');
            console.log(`[DEBUG] exec: ${cmdStr}`);
            console.log(`[DEBUG] exit: ${r.code}, output: ${r.output}`);
        }
        if (r.code !== 0) {
            if (process.env.TETHERING_DEBUG) {
                const cmdStr = typeof cmd === 'string' ? cmd : cmd.join(' ');
                console.log(`[DEBUG] exec FAILED: ${cmdStr}\n[DEBUG] full output:\n${r.output}`);
            }
            throw new Error(
                `Command failed (exit ${r.code}) in ${this.name}:\n` +
                `  ${typeof cmd === 'string' ? cmd : cmd.join(' ')}\n` +
                `Output:\n${r.output}`
            );
        }
        return r.output.trim();
    }

    /** Exec a command, return {code, output} without throwing. */
    async tryExec(cmd, opts) {
        const r = await docker.execContainer({ name: this.name, cmd, ...opts });
        if (process.env.TETHERING_DEBUG) {
            const cmdStr = typeof cmd === 'string' ? cmd : cmd.join(' ');
            console.log(`[DEBUG] tryExec: ${cmdStr}`);
            console.log(`[DEBUG] exit: ${r.code}, output: ${r.output}`);
        }
        return r;
    }

    /** Start the detached container, ensure /dev/ppp, wait for MM + NM. */
    async start() {
        if (!this.imageRef) throw new Error('TETHERING_DOCKER_IMAGE is not set');
        await docker.ensureImage(this.imageRef);
        await docker.startContainer({ image: this.imageRef, name: this.name, args: this.containerArgs });
        await this.ensurePppDevice();
        await poll(
            async () => {
                const r = await this.tryExec('mmcli -L >/dev/null 2>&1 && nmcli general status >/dev/null 2>&1');
                return r.code === 0;
            },
            30000, 2000, 'MM/NM D-Bus readiness'
        );
    }

    /** Resolve /dev/ttyACM* by USB serial. */
    async resolveTty(targetSerial) {
        for (let attempt = 1; attempt <= 5; attempt++) {
            const r = await this.tryExec(['resolve-tty.sh', targetSerial]);
            if (r.code === 0 && r.output.trim()) {
                return r.output.trim();
            }
            if (attempt < 5) {
                await sleep(2000);
                await this.exec('udevadm trigger || true; udevadm settle --timeout=5 || true');
            }
        }
        const r = await this.tryExec(['resolve-tty.sh', targetSerial]);
        throw new Error(
            `Could not resolve /dev/ttyACM* for USB serial '${targetSerial}' ` +
            `after 5 attempts\nLast resolve-tty.sh output:\n${r.output}`
        );
    }

    /**
     * Configure port: udev rules, ppp options, NM gsm connection.
     * @param {object} opts
     * @param {number} [opts.baud] Tether serial port baudrate. Omit for USB CDC.
     * @param {string} [opts.flowControl='none'] Flow control: 'none' or 'rts-cts'.
     */
    async setupPort(opts = {}) {
        const b = opts.baud ? String(opts.baud) : '';
        const flowControl = opts.flowControl || 'none';
        const targetSerial = this.adapterSerial || this.deviceId;
        await this.exec([
            'bash', '-c',
            `cat > /etc/udev/rules.d/78-mm-tethering.rules <<'EOF'\n` +
            `ACTION=="add|change|move|bind", SUBSYSTEMS=="usb", ` +
            `ATTRS{serial}=="${targetSerial}", ` +
            `ENV{ID_MM_DEVICE_PROCESS}="1"\n` +
            `ACTION=="add|change|move|bind", KERNEL=="${this.port}", ` +
            `ENV{ID_MM_DEVICE_IGNORE}="0", ` +
            `ENV{ID_MM_CANDIDATE}="1", ` +
            `ENV{ID_MM_PORT_TYPE_AT_PRIMARY}="1", ` +
            `ENV{ID_MM_TTY_FLOW_CONTROL}="${flowControl}", ` +
            (b ? `ENV{ID_MM_TTY_BAUDRATE}="${b}", ` : '') +
            `ENV{ID_MM_DEVICE_PROCESS}="1"\n` +
            `EOF`
        ]);
        await this.exec('udevadm control --reload-rules');
        await this.exec('udevadm trigger --action=change --subsystem-match=tty');
        await this.exec('udevadm trigger --action=change --subsystem-match=usb');
        await this.exec('udevadm settle --timeout=5');
        await this.exec('systemctl restart ModemManager.service');
        await this.exec('mkdir -p /etc/ppp');
        await this.exec(`if [ -f /etc/ppp/options ]; then mv /etc/ppp/options /etc/ppp/options.bak; fi`);
        await this.exec(`printf 'local\\n' > /etc/ppp/options.${this.port}`);
        await this.exec(
            `if ! nmcli -g NAME connection show | grep -qx tethering; then ` +
            `nmcli connection add type gsm con-name tethering ` +
            `gsm.apn tethering connection.interface ${this.port} ` +
            `ppp.crtscts true autoconnect yes; fi`
        );
    }

    /** Wait for ModemManager to detect a Particle modem. */
    async waitForModem(timeoutMs = 60000) {
        await poll(
            async () => {
                const out = await this.exec('mmcli -L 2>/dev/null || true');
                if (new RegExp(PARTICLE_MODEM_RE).test(out)) {
                    return true;
                }
                if (out.includes('/Modem/')) {
                    await this.exec('udevadm trigger --action=change --subsystem-match=tty || true');
                }
                return false;
            },
            timeoutMs, 5000, 'Particle modem detection'
        );
    }

    /** Wait for ppp0 interface to be UP. */
    async waitForPpp0(timeoutMs = 120000) {
        await poll(
            async () => {
                const r = await this.tryExec('ip addr show ppp0 2>/dev/null');
                return r.code === 0 && r.output.includes('LOWER_UP');
            },
            timeoutMs, 1000, 'ppp0 interface UP'
        );
    }

    /** Ensure /dev/ppp exists (DinD workaround). */
    async ensurePppDevice() {
        await this.exec([
            'bash', '-c',
            'if [ ! -e /dev/ppp ]; then ' +
            'ppp_major="$(awk \'$2=="ppp"{print $1}\' /proc/devices)"; ' +
            ': "${ppp_major:=108}"; ' +
            'mknod /dev/ppp c "$ppp_major" 0 2>/dev/null || true; ' +
            'fi'
        ]);
    }

    /**
     * Ping a target via ppp0. Throws on failure.
     * @param {object} opts
     * @param {string} [opts.host='8.8.8.8'] Target host.
     * @param {number} [opts.count=1] Number of pings.
     * @param {number} [opts.timeout=2] Timeout in seconds per ping.
     */
    async ping(opts = {}) {
        const host = opts.host || '8.8.8.8';
        const count = opts.count || 1;
        const timeout = opts.timeout || 2;
        await this.exec(`ping -I ppp0 -c ${count} -W ${timeout} -n ${host}`);
    }

    /** Get the PPP peer address (device side) from the container's ppp0. */
    async resolvePeerIp() {
        const out = await this.exec('ip -4 -o addr show ppp0');
        const match = out.match(/peer\s+(\d+\.\d+\.\d+\.\d+)/);
        if (!match) {
            throw new Error(`Could not parse ppp0 peer address: ${out}`);
        }
        this.tetherIp = match[1];
        return this.tetherIp;
    }

    /**
     * Run download speed tests via curl over ppp0.
     * @param {number} minBps Minimum acceptable average speed (bits/sec).
     * @param {object} opts
     * @param {number} [opts.runs=10] Number of download samples.
     * @returns {Promise<{avgBps: number, samples: number[]}>}
     */
    async downloadSpeedTest(minBps, opts = {}) {
        const runs = opts.runs || 10;
        const maxTime = 120;
        const samples = [];
        let totalBytes = 0;
        let totalTimeSec = 0;

        for (let i = 1; i <= runs; i++) {
            let out;
            try {
                out = await this.exec(
                    `curl --interface ppp0 -sS -o /dev/null --max-time ${maxTime} ` +
                    `-w '%{speed_download} %{size_download} %{time_total}' '${this.url}'`
                );
            } catch (err) {
                console.log(`    ${i} failed: ${err.message.split('\n')[0]}`);
                continue;
            }
            const [speedDownload, sizeDownload, timeTotal] = out.split(/\s+/);
            const bytes = parseInt(sizeDownload, 10);
            const sec = parseFloat(timeTotal);
            const bps = Math.round((bytes * 8) / sec);
            totalBytes += bytes;
            totalTimeSec += sec;
            samples.push(bps);
            console.log(`    ${i} bps: ${bps}, size: ${bytes}, time: ${sec}s`);
        }

        if (samples.length === 0) {
            throw new Error('All download attempts failed');
        }

        const avgBps = Math.round((totalBytes * 8) / totalTimeSec);
        return { avgBps, samples };
    }

    /**
     * Run upload speed tests via curl over ppp0.
      * @param {number} minBps Minimum acceptable average speed (bits/sec).
      * @param {object} opts
      * @param {number} [opts.runs=10] Number of upload samples.
      * @param {number} [opts.sizeKb=100] Payload size in KB per upload.
      * @returns {Promise<{avgBps: number, samples: number[]}>}
      */
    async uploadSpeedTest(minBps, opts = {}) {
        const runs = opts.runs || 10;
        const sizeKb = opts.sizeKb || 100;
        const maxTime = 120;
        // Tele2's speedtest upload sink: accepts arbitrary POST bodies and
        // returns a tiny (11-byte) response, so the measured time is upload, not
        // response download. Deliberately NOT Cloudflare-fronted: sustained
        // uploads to Cloudflare anycast (speed.cloudflare.com, httpspot.dev) die
        // flow-by-flow on some roaming carriers (handshake + first segments ACK,
        // then the flow blackholes while ping/other flows to the same IP work).
        // Plain HTTP keeps TLS out of a raw throughput measurement. Override
        // with TETHERING_UPLOAD_URL.
        const url = process.env.TETHERING_UPLOAD_URL || 'http://speedtest.tele2.net/upload.php';

        const samples = [];
        let totalBytes = 0;
        let totalTimeSec = 0;

        for (let i = 1; i <= runs; i++) {
            let out;
            try {
                // Defense in depth against a wedged upload (a stalled flow can
                // survive curl's --max-time in rare cases, and a hung exec used
                // to block the whole test until killed by hand):
                //  - --speed-limit/--speed-time: abort if under 1 KB/s for 30s
                //  - timeout(1): SIGTERM at maxTime+10s, SIGKILL 5s later
                //  - exec timeoutMs: docker-side backstop above all of it
                out = await this.exec(
                    `dd if=/dev/urandom of=/tmp/upload.bin bs=1024 count=${sizeKb} 2>/dev/null && ` +
                    `timeout -k 5 ${maxTime + 10} ` +
                    `curl --interface ppp0 -sS -o /dev/null --max-time ${maxTime} ` +
                    `--speed-limit 1024 --speed-time 30 ` +
                    `-w '%{speed_upload} %{size_upload} %{time_total}' ` +
                    `-X POST -H 'Content-Type: application/octet-stream' ` +
                    `--data-binary @/tmp/upload.bin '${url}'; rc=$?; ` +
                    `rm -f /tmp/upload.bin; exit $rc`,
                    { timeoutMs: (maxTime + 30) * 1000 }
                );
            } catch (err) {
                console.log(`    ${i} failed: ${err.message.split('\n')[0]}`);
                continue;
            }
            const [speedUpload, sizeUpload, timeTotal] = out.split(/\s+/);
            const bytes = parseInt(sizeUpload, 10);
            const sec = parseFloat(timeTotal);
            const bps = Math.round((bytes * 8) / sec);
            totalBytes += bytes;
            totalTimeSec += sec;
            samples.push(bps);
            console.log(`    ${i} bps: ${bps}, size: ${bytes}, time: ${sec}s`);
        }

        if (samples.length === 0) {
            throw new Error('All upload attempts failed');
        }

        const avgBps = Math.round((totalBytes * 8) / totalTimeSec);
        return { avgBps, samples };
    }

    /** Capture NM connection timestamp + MM modem state for stability checking. */
    async captureLinkState() {
        const ts = await this.exec('nmcli -t -f connection.timestamp connection show tethering 2>/dev/null || echo 0');
        const modemState = await this.exec('mmcli -m 0 --state 2>/dev/null || echo unknown');
        return {
            timestamp: parseInt(ts.trim(), 10) || 0,
            modemState: modemState.trim()
        };
    }

    /** Verify link state hasn't changed since captureLinkState(). */
    async verifyLinkState(expected) {
        const current = await this.captureLinkState();
        if (current.timestamp !== expected.timestamp) {
            throw new Error(
                `PPP link was re-established during test (NM timestamp: ` +
                `${expected.timestamp} -> ${current.timestamp}) -- connection was unstable`
            );
        }
        if (current.modemState !== expected.modemState) {
            throw new Error(
                `Modem state changed during test ` +
                `(${expected.modemState} -> ${current.modemState})`
            );
        }
    }

    /**
     * Run an iperf3 test. Retries on failure up to opts.retries times (default 3).
     * Captures link state before/after - fails if link was re-established.
     * @param {object} opts
     * @param {string} [opts.host] Target host (default: this.tetherIp)
     * @param {'tcp'|'udp'} [opts.proto='tcp']
     * @param {'up'|'down'} [opts.direction='down'] up = container->device, down = device->container
     * @param {number} [opts.time=10] Duration in seconds.
     * @param {number|string} [opts.bitrate] For UDP, target bitrate.
     * @param {number} [opts.retries=3] Retry attempts on failure.
     * @returns {Promise<{json: object, attempts: number}>}
     */
    async iperf3(opts = {}) {
        const host = opts.host || this.tetherIp;
        const proto = opts.proto || 'tcp';
        const direction = opts.direction || 'down';
        const time = opts.time || 10;
        const port = opts.port || 5201;
        const retries = opts.retries != null ? opts.retries : 3;
        const getServerOutput = opts.getServerOutput !== false;
        const blksize = opts.blksize;

        const args = [
            'iperf3', '-J',
            '-c', host, '-p', String(port),
            '-t', String(time), '--bind-dev', 'ppp0',
            '--connect-timeout', '10000'
        ];
        if (getServerOutput) {
            args.push('--get-server-output');
        }
        if (proto === 'udp') {
            args.push('-u', '-b', String(opts.bitrate || '1M'));
        }
        if (blksize) {
            args.push('-l', String(blksize));
        }
        if (direction === 'down') {
            args.push('-R');
        }

        let lastErr;
        for (let attempt = 1; attempt <= retries; attempt++) {
            if (attempt > 1) {
                await sleep(2000);
            }
            // Kill any stale iperf3 processes from previous attempts
            await this.tryExec('pkill -9 iperf3 2>/dev/null');
            // Verify PPP link is alive before each attempt
            if (this.tetherIp) {
                const r = await this.tryExec(`ping -I ppp0 -c 1 -W 5 -n ${this.tetherIp} 2>/dev/null`);
                if (r.code !== 0) {
                    lastErr = new Error(`PPP link down: ping to ${this.tetherIp} failed`);
                    console.log(`    iperf3 attempt ${attempt}/${retries}: PPP link not alive, skipping`);
                    continue;
                }
            }
            const linkBefore = await this.captureLinkState();
            let out;
            try {
                out = await this.exec(args, { timeoutMs: (time + 60) * 1000 });
            } catch (err) {
                lastErr = err;
                console.log(`    iperf3 attempt ${attempt}/${retries} failed: ${err.message.split('\n')[0]}`);
                // Always log full output on failure for debugging
                const lines = err.message.split('\n');
                const outputStart = lines.findIndex(l => l.includes('Output:'));
                if (outputStart >= 0) {
                    console.log(`    iperf3 stderr/stdout:\n${lines.slice(outputStart + 1).join('\n')}`);
                }
                continue;
            }
            await this.verifyLinkState(linkBefore);

            let json;
            try {
                json = JSON.parse(out);
            } catch (err) {
                lastErr = new Error(`iperf3 returned non-JSON output: ${out.slice(0, 200)}`);
                console.log(`    iperf3 attempt ${attempt}/${retries} failed: ${lastErr.message.split('\n')[0]}`);
                // Always log full output on non-JSON for debugging
                console.log(`    iperf3 full output (${out.length} bytes):\n${out.slice(0, 1000)}`);
                continue;
            }

            if (attempt > 1) {
                console.log(`    iperf3 succeeded on attempt ${attempt}/${retries}`);
            }
            if (json.end) {
                console.log(`    [JSON] end:`, JSON.stringify({
                    sum: json.end.sum,
                    sum_sent: json.end.sum_sent,
                    sum_received: json.end.sum_received,
                    server_output: json.end.server_output_json
                        ? {
                              sum: json.end.server_output_json.end && json.end.server_output_json.end.sum,
                              sum_sent: json.end.server_output_json.end && json.end.server_output_json.end.sum_sent,
                              sum_received: json.end.server_output_json.end && json.end.server_output_json.end.sum_received
                          }
                        : undefined
                }));
            }
            return { json, attempts: attempt };
        }

        throw new Error(`iperf3 (${proto} ${direction}) failed after ${retries} attempts: ${lastErr.message}`);
    }

    /**
     * Find max sustainable UDP bitrate.
     * For 'down' (device is sender): device self-limits, so actual throughput
     * IS the max sustainable. No search needed.
     * For 'up' (host is sender): host can exceed link speed causing loss.
     * Binary search on -b to find where loss = 0%.
     * @param {object} opts
     * @param {'up'|'down'} [opts.direction='down']
     * @param {number} [opts.startBitrate=1000000] Starting bitrate for search.
     * @param {number} [opts.time=10] Duration per search probe.
     * @param {number} [opts.verifyTime=30] Duration of final verification.
     * @param {number} [opts.blksize] UDP datagram size in bytes (default: iperf3 default ~1460).
     * @returns {Promise<{bitrate: number, loss: number, attempts: number, iterations: number}>}
     */
    async iperf3UdpSustain(opts = {}) {
        const direction = opts.direction || 'down';
        const searchTime = opts.time || 10;
        const verifyTime = opts.verifyTime || 30;
        const blksize = opts.blksize;
        const lossThreshold = 1.0; // Max acceptable loss % for verify
        let iterations = 0;
        let totalAttempts = 0;

        const getStats = (json) => {
            let bps, loss;
            if (direction === 'up') {
                // Device is receiver
                const srv = json.end.server_output_json && json.end.server_output_json.end;
                if (srv) {
                    bps = srv.sum.bits_per_second;
                    loss = srv.sum.lost_percent;
                } else {
                    bps = json.end.sum_received ? json.end.sum_received.bits_per_second : 0;
                    loss = json.end.sum_received ? json.end.sum_received.lost_percent : 100;
                }
            } else {
                // Client is receiver
                bps = json.end.sum.bits_per_second;
                loss = json.end.sum.lost_percent;
            }
            return { bps: Math.round(bps), loss };
        };

        const waitForLink = async () => {
            await poll(
                async () => {
                    const r = await this.tryExec('ip addr show ppp0 2>/dev/null');
                    return r.code === 0 && r.output.includes('LOWER_UP');
                },
                30000, 1000, 'ppp0 link recovery (LOWER_UP)'
            );
            let pingOk = false;
            for (let i = 0; i < 5; i++) {
                const r = await this.tryExec(`ping -I ppp0 -c 1 -W 2 -n ${this.tetherIp} 2>/dev/null`);
                if (r.code === 0) { pingOk = true; break; }
                if (i === 0) {
                    console.log(`    UDP sustain: peer unreachable, restarting NM connection...`);
                    await this.tryExec('nmcli connection down tethering 2>/dev/null');
                    await sleep(2000);
                    await this.tryExec('nmcli connection up tethering 2>/dev/null');
                    await poll(
                        async () => {
                            const r2 = await this.tryExec('ip addr show ppp0 2>/dev/null');
                            return r2.code === 0 && r2.output.includes('LOWER_UP');
                        },
                        30000, 1000, 'ppp0 reconnect after NM restart'
                    );
                }
                await sleep(2000);
            }
            if (!pingOk) {
                throw new Error('ppp0 link did not recover after NM restart');
            }
            // Wait for PPP TX backlog to drain after heavy traffic.
            // Ping latency spikes when the TX queue is congested; wait for it
            // to stabilize below 100ms before proceeding.
            for (let i = 0; i < 10; i++) {
                const r = await this.tryExec(`ping -I ppp0 -c 1 -W 5 -n ${this.tetherIp} 2>/dev/null`);
                if (r.code !== 0) {
                    await sleep(1000);
                    continue;
                }
                const m = r.output.match(/time=([\d.]+)\s*ms/);
                const latency = m ? parseFloat(m[1]) : 999;
                if (latency < 100) {
                    break;
                }
                await sleep(1000);
            }
        };

        const probe = async (bitrate) => {
            iterations++;
            await this.tryExec('pkill -9 iperf3 2>/dev/null');
            await sleep(5000); // Give PPP TX queue time to drain after heavy traffic
            // Wait for PPP TX backlog to drain before probing.
            // Use a large ping (1000 bytes) to verify the TX queue has capacity
            // for the control connection's TCP segments during results exchange.
            for (let i = 0; i < 10; i++) {
                const r = await this.tryExec(`ping -I ppp0 -c 1 -W 5 -s 1000 -n ${this.tetherIp} 2>/dev/null`);
                if (r.code !== 0) {
                    await sleep(1000);
                    continue;
                }
                const m = r.output.match(/time=([\d.]+)\s*ms/);
                const latency = m ? parseFloat(m[1]) : 999;
                if (latency < 100) break;
                await sleep(1000);
            }
            try {
                const { json, attempts } = await this.iperf3({
                    proto: 'udp', direction, time: searchTime, retries: 1,
                    bitrate: `${bitrate}`, getServerOutput: true, blksize
                });
                totalAttempts += attempts;
                return getStats(json);
            } catch (err) {
                console.log(`    UDP sustain: probe failed, waiting for link recovery...`);
                await waitForLink();
                return null;
            }
        };

        // First probe with high cap - measure actual throughput
        const highCap = 1000000000; // 1G - effectively unlimited
        let result = await probe(highCap);
        if (!result) {
            throw new Error('UDP sustain: initial probe failed');
        }
        console.log(`    UDP sustain: ${Math.round(result.bps / 1000)} kbps, loss: ${result.loss}%`);

        if (result.loss === 0) {
            // Sender self-limited (or link is fast enough). Actual throughput is the max.
            // For 'up', verify the host wasn't just slow - try doubling to confirm.
            // For 'down', the device self-limits so this is the answer.
            if (direction === 'down') {
                // Device is sender, self-limited. Verify at full duration.
                const { json: vjson, attempts: vatt } = await this.iperf3({
                    proto: 'udp', direction, time: verifyTime, retries: 3, bitrate: `${highCap}`,
                    getServerOutput: true, blksize
                });
                totalAttempts += vatt;
                iterations++;
                const vstats = getStats(vjson);
                console.log(`    UDP sustain verify: ${Math.round(vstats.bps / 1000)} kbps, loss: ${vstats.loss}%`);
                return { bitrate: vstats.bps, loss: vstats.loss, attempts: totalAttempts, iterations };
            }
            // 'up' direction: host can send faster. Binary search upward.
            let lo = result.bps;
            let hi = result.bps * 2;
            let bestBps = result.bps;
            let bestLoss = 0;
            // Search upward for 0% loss
            while (true) {
                result = await probe(hi);
                if (!result) break;
                console.log(`    UDP sustain: ${Math.round(result.bps / 1000)} kbps, loss: ${result.loss}%`);
                if (result.loss > 0) break;
                lo = hi;
                bestBps = result.bps;
                bestLoss = result.loss;
                hi = hi * 2;
            }
            // Binary search between lo (0% loss) and hi (>0% loss)
            while (Math.round(hi - lo) > 5000) {
                const mid = Math.round((hi + lo) / 2);
                result = await probe(mid);
                if (!result) { hi = mid; continue; }
                console.log(`    UDP sustain: ${Math.round(result.bps / 1000)} kbps @ -b ${mid}, loss: ${result.loss}%`);
                if (result.loss === 0) {
                    lo = mid;
                    if (result.bps > bestBps) {
                        bestBps = result.bps;
                        bestLoss = result.loss;
                    }
                } else {
                    hi = mid;
                }
            }
            // Verify
            await this.tryExec('pkill -9 iperf3 2>/dev/null');
            await sleep(2000);
            const vres = await this.iperf3Verify({
                direction, time: verifyTime, bitrate: lo, blksize,
                getStats, lossThreshold
            });
            totalAttempts += vres.attempts;
            iterations += vres.iterations;
            console.log(`    UDP sustain verify: ${Math.round(vres.bps / 1000)} kbps, loss: ${vres.loss}%`);
            return { bitrate: vres.bps, loss: vres.loss, attempts: totalAttempts, iterations };
        }

        // Loss > 0%. The received throughput from the probe tells us the link's
        // approximate capacity. Narrow the search around it instead of scanning
        // all the way down from highCap.
        const receivedBps = result.bps;
        let lo = Math.round(receivedBps * 0.9);
        let hi = Math.round(receivedBps * 1.1);
        let bestBps = 0;
        let bestLoss = 100;
        // Search for 0% loss only. The narrow window converges in ~4 iterations.
        while (Math.round(hi - lo) > 5000) {
            const mid = Math.round((hi + lo) / 2);
            result = await probe(mid);
            if (!result) { hi = mid; continue; }
            console.log(`    UDP sustain: ${Math.round(result.bps / 1000)} kbps @ -b ${mid}, loss: ${result.loss}%`);
            if (result.loss === 0) {
                lo = mid;
                if (result.bps > bestBps) {
                    bestBps = result.bps;
                    bestLoss = result.loss;
                }
            } else {
                hi = mid;
            }
        }
        if (bestBps === 0) {
            // Fallback: widen search if the narrow window didn't find anything
            lo = 1000;
            hi = Math.round(receivedBps * 1.1);
            while (Math.round(hi - lo) > 5000) {
                const mid = Math.round((hi + lo) / 2);
                result = await probe(mid);
                if (!result) { hi = mid; continue; }
                console.log(`    UDP sustain: ${Math.round(result.bps / 1000)} kbps @ -b ${mid}, loss: ${result.loss}%`);
                if (result.loss === 0) {
                    lo = mid;
                    if (result.bps > bestBps) {
                        bestBps = result.bps;
                        bestLoss = result.loss;
                    }
                } else {
                    hi = mid;
                }
            }
        }
        if (bestBps === 0) {
            throw new Error(`UDP sustain: could not find a loss-free bitrate`);
        }
        await this.tryExec('pkill -9 iperf3 2>/dev/null');
        await sleep(2000);
        const vres = await this.iperf3Verify({
            direction, time: verifyTime, bitrate: lo, blksize,
            getStats, lossThreshold
        });
        totalAttempts += vres.attempts;
        iterations += vres.iterations;
        console.log(`    UDP sustain verify: ${Math.round(vres.bps / 1000)} kbps, loss: ${vres.loss}%`);
        return { bitrate: vres.bps, loss: vres.loss, attempts: totalAttempts, iterations };
    }

    /**
     * Run a verification iperf3 test and retry at a lower bitrate if loss exceeds threshold.
     * @param {object} opts
     * @param {'up'|'down'} opts.direction
     * @param {number} opts.time Verify duration in seconds.
     * @param {number} opts.bitrate Target bitrate for the first attempt.
     * @param {number} [opts.blksize] UDP datagram size.
     * @param {function} opts.getStats Function to extract {bps, loss} from JSON.
     * @param {number} [opts.lossThreshold=1.0] Max acceptable loss percentage.
     * @returns {Promise<{bps: number, loss: number, attempts: number, iterations: number}>}
     */
    async iperf3Verify(opts = {}) {
        const direction = opts.direction;
        const time = opts.time || 30;
        const blksize = opts.blksize;
        const getStats = opts.getStats;
        const lossThreshold = opts.lossThreshold != null ? opts.lossThreshold : 1.0;
        let bitrate = opts.bitrate;
        let attempts = 0;
        let iterations = 0;
        const maxRetries = 3;
        for (let retry = 0; retry <= maxRetries; retry++) {
            await this.tryExec('pkill -9 iperf3 2>/dev/null');
            if (retry > 0) await sleep(2000);
            console.log(`    iperf3Verify: retry ${retry}/${maxRetries}, bitrate=${bitrate}`);
            try {
                const { json, attempts: att } = await this.iperf3({
                    proto: 'udp', direction, time, retries: 1, bitrate: `${bitrate}`, blksize
                });
                attempts += att;
                iterations++;
                const stats = getStats(json);
                if (stats.loss <= lossThreshold) {
                    return { bps: stats.bps, loss: stats.loss, attempts, iterations };
                }
                console.log(`    UDP sustain verify: loss ${stats.loss}% > ${lossThreshold}%, retrying at received bitrate`);
                bitrate = Math.round(stats.bps);
            } catch (err) {
                console.log(`    UDP sustain verify: failed: ${err.message.split('\n')[0]}`);
                const lines = err.message.split('\n');
                const outputStart = lines.findIndex(l => l.includes('Output:'));
                if (outputStart >= 0) {
                    console.log(`    verify stderr/stdout:\n${lines.slice(outputStart + 1).join('\n').slice(0, 500)}`);
                }
                // Reduce bitrate modestly for retry
                bitrate = Math.round(bitrate * 0.9);
            }
        }
        // Last resort: return the last result even if loss exceeds threshold
        try {
            const { json, attempts: att } = await this.iperf3({
                proto: 'udp', direction, time, retries: 3, bitrate: `${bitrate}`, blksize
            });
            attempts += att;
            iterations++;
            const stats = getStats(json);
            return { bps: stats.bps, loss: stats.loss, attempts, iterations };
        } catch (err) {
            // All attempts failed - return a failure result so the caller can
            // decide whether to fail the test or retry
            console.log(`    UDP sustain verify: last resort failed: ${err.message.split('\n')[0]}`);
            return { bps: 0, loss: 100, attempts, iterations };
        }
    }

    /**
     * Run a full-path iperf3 UDP test through the cellular link to a public server.
     * Traffic goes: container -> ppp0 -> device -> cellular -> internet -> server.
     * @param {object} opts
     * @param {'up'|'down'} [opts.direction='down']
     * @param {number|string} [opts.bitrate] Target bitrate.
     * @param {number} [opts.time=10] Duration in seconds.
     * @param {number} [opts.retries=3] Retries per iperf3 invocation.
     * @param {string} [opts.host] Public iperf3 server (default: from env or cycling US servers).
     * @returns {Promise<{json: object, attempts: number}>}
     */
    async iperf3UdpFullPath(opts = {}) {
        const servers = IPERF3_PUBLIC_SERVERS;
        let lastErr;
        for (const srv of servers) {
            try {
                return await this.iperf3({
                    proto: 'udp',
                    host: srv.host,
                    port: srv.port,
                    direction: opts.direction || 'down',
                    time: opts.time || 10,
                    retries: 1,
                    bitrate: opts.bitrate || '1M'
                });
            } catch (err) {
                lastErr = err;
                console.log(`    iperf3 full-path: server ${srv.host}:${srv.port} failed: ${err.message.split('\n')[0]}`);
            }
        }
        throw lastErr || new Error('iperf3 full-path: all servers failed');
    }

    /**
     * Run a TCP iperf3 test against a public server, cycling through the
     * server list from iperf3_servers.h.
     */
    async iperf3TcpFullPath(opts = {}) {
        const servers = IPERF3_PUBLIC_SERVERS;
        let lastErr;
        for (const srv of servers) {
            try {
                return await this.iperf3({
                    proto: 'tcp',
                    host: srv.host,
                    port: srv.port,
                    direction: opts.direction || 'down',
                    time: opts.time || 20,
                    retries: 1
                });
            } catch (err) {
                lastErr = err;
                console.log(`    iperf3 full-path: server ${srv.host}:${srv.port} failed: ${err.message.split('\n')[0]}`);
            }
        }
        throw lastErr || new Error('iperf3 full-path: all servers failed');
    }

    /** Fetch container logs for diagnostics. */
    async getLogs() {
        try {
            return await docker.containerLogs(this.name);
        } catch (_) {
            return '';
        }
    }

    /** Stop and remove container. */
    async cleanup() {
        await docker.stopContainer(this.name);
    }
}

module.exports = {
    Tether,
    loadRigConfig,
    rigEntryForDevice,
    adapterSerial,
    defaultUrl
};