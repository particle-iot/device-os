'use strict';

const { execFile, execFileSync } = require('child_process');
const crypto = require('crypto');
const fs = require('fs');
const os = require('os');
const path = require('path');

const CLAIM_TIMEOUT = 60 * 60 * 1000;
const CLAIM_REFRESH_INTERVAL = 10000;
const AGENT_ANSWER_TIMEOUT = 60000;
const RADIO_PROBLEM_TIMEOUT = 5 * 60 * 1000;
const ACQUIRE_TIMEOUT = CLAIM_TIMEOUT + 5 * 60 * 1000;
const START_TIMEOUT = 30000;
const STOP_TIMEOUT = 5000;
const COMMAND_TIMEOUT = 30000;
const POLL_INTERVAL = 1000;
const SSH_CONNECT_TIMEOUT = 10;
const RULE_COMMENT = 'device-os-test-wifi-ap';
const SSID_PREFIX = 'device-os-test-';
const TMP_DIR_PREFIX = 'wifi-ap-';
const REMOTE_TMP_DIR = '/tmp';
const SUBNET_PREFIXES = ['192.168.212', '192.168.213', '10.212.0'];
const SBIN_DIRS = ['/usr/local/sbin', '/usr/sbin', '/sbin'];
const REQUIRED_TOOLS = ['hostapd', 'hostapd_cli', 'dnsmasq', 'iw', 'ip', 'iptables', 'sysctl', 'kill', 'rm'];
const OPTIONAL_TOOLS = ['nmcli', 'firewall-cmd'];
const ACTION_FRAME_CONTROL = Buffer.from([0xd0, 0x00]);
const MAC_RE = /^([0-9a-f]{2}:){5}[0-9a-f]{2}$/i;

class WifiApUnavailableError extends Error {
    constructor(message) {
        super(message);
        this.name = 'WifiApUnavailableError';
    }
}

function delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function isExecutable(file) {
    try {
        fs.accessSync(file, fs.constants.X_OK);
        return true;
    } catch (e) {
        return false;
    }
}

function exec(file, args, { timeout = COMMAND_TIMEOUT, input } = {}) {
    return new Promise(resolve => {
        const child = execFile(file, args, { timeout }, (err, stdout, stderr) => {
            const code = err ? (typeof err.code === 'number' ? err.code : -1) : 0;
            resolve({ code, stdout: String(stdout), stderr: String(stderr) });
        });
        child.stdin.on('error', () => {});
        child.stdin.end(input);
    });
}

function execSync(file, args, { timeout = COMMAND_TIMEOUT } = {}) {
    try {
        const stdout = execFileSync(file, args, { timeout, stdio: ['ignore', 'pipe', 'pipe'] });
        return { code: 0, stdout: String(stdout), stderr: '' };
    } catch (e) {
        return { code: typeof e.status === 'number' ? e.status : -1, stdout: String(e.stdout || ''), stderr: String(e.stderr || e.message) };
    }
}

function withSudo(file, args, sudo) {
    return sudo ? ['sudo', ['-n', file, ...args]] : [file, args];
}

function sleepSync(ms) {
    Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, ms);
}

async function check(host, file, args, opts) {
    const r = await host.run(file, args, opts);
    if (r.code !== 0) {
        throw new Error(`${path.basename(file)} ${args.join(' ')} failed: ${(r.stderr || r.stdout).trim()}`);
    }
    return r.stdout;
}

function macBytes(mac) {
    return Buffer.from(mac.replace(/:/g, ''), 'hex');
}

function readText(file) {
    try {
        return fs.readFileSync(file, 'utf8').trim();
    } catch (e) {
        return '';
    }
}

function shellQuote(arg) {
    return `'${String(arg).replace(/'/g, `'\\''`)}'`;
}

class LocalHost {
    get name() {
        return 'localhost';
    }

    get tmpDir() {
        return os.tmpdir();
    }

    run(file, args, { sudo = false, ...opts } = {}) {
        return exec(...withSudo(file, args, sudo), opts);
    }

    runSync(file, args, { sudo = false, ...opts } = {}) {
        return execSync(...withSudo(file, args, sudo), opts);
    }

    async findTools(names) {
        const dirs = (process.env.PATH || '').split(path.delimiter).concat(SBIN_DIRS);
        const tools = {};
        for (const name of names) {
            tools[name] = dirs.map(dir => path.join(dir, name)).find(isExecutable) || null;
        }
        return tools;
    }

    async isRoot() {
        return process.getuid() === 0;
    }

    async processes() {
        return fs.readdirSync('/proc').filter(d => /^\d+$/.test(d)).map(pid => ({
            pid,
            args: readText(`/proc/${pid}/cmdline`).split('\0')
        }));
    }

    async mkdtemp() {
        return fs.mkdtempSync(path.join(this.tmpDir, TMP_DIR_PREFIX));
    }

    async writeFile(file, data) {
        fs.writeFileSync(file, data);
    }

    readText(file) {
        return readText(file);
    }

    exists(file) {
        return fs.existsSync(file);
    }
}

class SshHost {
    constructor(destination) {
        this._destination = destination;
    }

    get name() {
        return this._destination;
    }

    get tmpDir() {
        return REMOTE_TMP_DIR;
    }

    run(file, args, { sudo = false, ...opts } = {}) {
        return exec('ssh', this._sshArgs(file, args, sudo), opts);
    }

    runSync(file, args, { sudo = false, ...opts } = {}) {
        return execSync('ssh', this._sshArgs(file, args, sudo), opts);
    }

    async findTools(names) {
        const script = 'for t; do p=; for d in $(echo "$PATH" | tr : " ") ' + SBIN_DIRS.join(' ') +
                '; do if [ -x "$d/$t" ]; then p="$d/$t"; break; fi; done; echo "$t $p"; done';
        const r = await this.run('sh', ['-c', script, 'sh', ...names]);
        if (r.code !== 0) {
            throw new Error(`ssh ${this._destination} failed: ${(r.stderr || r.stdout).trim()}`);
        }
        const tools = {};
        for (const line of r.stdout.split('\n')) {
            const [name, file] = line.trim().split(' ');
            if (name) {
                tools[name] = file || null;
            }
        }
        return tools;
    }

    async isRoot() {
        return (await check(this, 'id', ['-u'])).trim() === '0';
    }

    async processes() {
        const procs = [];
        for (const line of (await check(this, 'ps', ['-eo', 'pid=,args='])).split('\n')) {
            const m = /^\s*(\d+)\s+(.*)$/.exec(line);
            if (m) {
                procs.push({ pid: m[1], args: m[2].split(/\s+/) });
            }
        }
        return procs;
    }

    async mkdtemp() {
        return (await check(this, 'mktemp', ['-d', `${this.tmpDir}/${TMP_DIR_PREFIX}XXXXXXXX`])).trim();
    }

    async writeFile(file, data) {
        const r = await this.run('sh', ['-c', 'cat > "$1"', 'sh', file], { input: data });
        if (r.code !== 0) {
            throw new Error(`Can't write ${file} on ${this._destination}: ${(r.stderr || r.stdout).trim()}`);
        }
    }

    readText(file) {
        const r = this.runSync('cat', [file]);
        return r.code === 0 ? r.stdout.trim() : '';
    }

    exists(file) {
        return this.runSync('test', ['-e', file]).code === 0;
    }

    _sshArgs(file, args, sudo) {
        const [cmd, cmdArgs] = withSudo(file, args, sudo);
        return [
            '-o', 'BatchMode=yes',
            '-o', `ConnectTimeout=${SSH_CONNECT_TIMEOUT}`,
            '-o', 'ServerAliveInterval=10',
            '-o', 'ServerAliveCountMax=3',
            this._destination,
            [cmd, ...cmdArgs].map(shellQuote).join(' ')
        ];
    }
}

async function uplinkInterface(host, ip) {
    const r = await host.run(ip, ['-j', 'route', 'get', '1.1.1.1']);
    if (r.code !== 0) {
        return null;
    }
    try {
        const routes = JSON.parse(r.stdout);
        return routes.length ? routes[0].dev : null;
    } catch (e) {
        return null;
    }
}

async function wifiInterfaces(host, iw) {
    const ifaces = [];
    let phy = null;
    for (const line of (await check(host, iw, ['dev'])).split('\n')) {
        const t = line.trim();
        let m = /^phy#(\d+)$/.exec(t);
        if (m) {
            phy = m[1];
            continue;
        }
        m = /^Interface (\S+)$/.exec(t);
        if (m && phy !== null) {
            ifaces.push({ phy, ifname: m[1] });
        }
    }
    return ifaces;
}

function supportsApMode(phyInfo) {
    const m = /Supported interface modes:\n((?:\s+\* .*\n)+)/.exec(phyInfo);
    return !!m && m[1].split('\n').some(l => l.trim() === '* AP');
}

async function findFreeRadio(host, iw, uplink, Unavailable) {
    const wanted = process.env.DEVICE_OS_TEST_RUNNER_WIFI_AP_INTERFACE;
    const ifaces = (await wifiInterfaces(host, iw)).filter(i => !wanted || i.ifname === wanted);
    for (const { phy, ifname } of ifaces) {
        if (ifname === uplink) {
            continue;
        }
        if (!/^\s*type managed$/m.test(await check(host, iw, ['dev', ifname, 'info']))) {
            continue;
        }
        if (!(await check(host, iw, ['dev', ifname, 'link'])).startsWith('Not connected')) {
            continue;
        }
        if (!supportsApMode(await check(host, iw, [`phy#${phy}`, 'info']))) {
            continue;
        }
        return ifname;
    }
    throw new Unavailable(wanted ? `${wanted} on ${host.name} is in use, carries the uplink or has no AP mode` :
            `No free wireless interface with AP mode on ${host.name}`);
}

class RadioClaim {
    constructor(dir) {
        fs.mkdirSync(dir, { recursive: true });
        this._file = path.join(dir, 'claim');
        this._statusFile = path.join(dir, 'status');
        this._id = `${Date.now()}-${process.pid}`;
        this._write();
        this._timer = setInterval(() => this._refresh(), CLAIM_REFRESH_INTERVAL);
        this._timer.unref();
    }

    async wait() {
        const start = Date.now();
        let status = '';
        let logged = '';
        let answered = false;
        let problemSince = 0;
        for (;;) {
            const now = Date.now();
            status = readText(this._statusFile);
            const [statusId, state, ...rest] = status.split(' ');
            if (statusId === this._id) {
                answered = true;
                if (state === 'ok') {
                    return rest[rest.length - 1];
                }
                if (status !== logged) {
                    console.log(`wifi-phy-agent: ${state} ${rest.join(' ')}`);
                    logged = status;
                }
                if (state === 'pending' && rest.join(' ') === 'busy') {
                    problemSince = 0;
                } else if (!problemSince) {
                    problemSince = now;
                } else if (now - problemSince > RADIO_PROBLEM_TIMEOUT) {
                    throw new Error(`wifi-phy-agent: ${state} ${rest.join(' ')}`);
                }
            } else if (!answered && now - start > AGENT_ANSWER_TIMEOUT) {
                throw new Error('wifi-phy-agent is not answering, is it running on this worker?');
            }
            if (now - start > CLAIM_TIMEOUT) {
                throw new Error(`wifi-phy-agent didn't hand over the radio, last status: ${status || 'none'}`);
            }
            await delay(POLL_INTERVAL);
        }
    }

    release() {
        clearInterval(this._timer);
        fs.rmSync(this._file, { force: true });
    }

    _refresh() {
        try {
            const now = new Date();
            fs.utimesSync(this._file, now, now);
        } catch (e) {
            this._write();
        }
    }

    _write() {
        try {
            fs.writeFileSync(this._file, `${this._id}\n`);
        } catch (e) {
            console.log(`Can't write the radio claim: ${e.message}`);
        }
    }
}

async function purgeStale(host, tools, sudo) {
    const tmpDir = path.join(host.tmpDir, TMP_DIR_PREFIX);
    for (const { pid, args } of await host.processes()) {
        const exe = path.basename(args[0] || '');
        if ((exe === 'hostapd' || exe === 'dnsmasq') && args.some(a => a.includes(tmpDir))) {
            await host.run(tools.kill, [pid], { sudo });
        }
    }
    for (const [table, chain] of [['nat', 'POSTROUTING'], ['filter', 'FORWARD']]) {
        const r = await host.run(tools.iptables, ['-t', table, '-S', chain], { sudo });
        for (const line of r.stdout.split('\n')) {
            if (line.startsWith(`-A ${chain} `) && line.includes(`--comment ${RULE_COMMENT}`)) {
                await host.run(tools.iptables, ['-t', table, '-D', ...line.split(' ').slice(1)], { sudo });
            }
        }
    }
}

function hostapdConfig({ ifname, ctrlDir, ssid, passphrase, channel, country }) {
    return [
        `interface=${ifname}`,
        'driver=nl80211',
        `ctrl_interface=${ctrlDir}`,
        'ctrl_interface_group=0',
        `ssid=${ssid}`,
        ...(country ? [`country_code=${country}`, 'ieee80211d=1'] : []),
        'hw_mode=g',
        `channel=${channel}`,
        'ieee80211n=1',
        'wmm_enabled=1',
        'wpa=2',
        'wpa_key_mgmt=WPA-PSK',
        'rsn_pairwise=CCMP',
        `wpa_passphrase=${passphrase}`,
        'bss_transition=1',
        'rrm_beacon_report=1',
        'rrm_neighbor_report=1',
        ''
    ].join('\n');
}

class WifiAp {
    static async acquire() {
        const claimDir = process.env.DEVICE_OS_TEST_RUNNER_WIFI_AP_CLAIM_DIR;
        const remote = !claimDir && process.env.DEVICE_OS_TEST_RUNNER_WIFI_AP_HOST;
        const strict = !!(claimDir || remote);
        const Unavailable = strict ? Error : WifiApUnavailableError;
        const host = remote ? new SshHost(remote) : new LocalHost();
        const tools = await host.findTools([...REQUIRED_TOOLS, ...OPTIONAL_TOOLS]);
        const missing = REQUIRED_TOOLS.filter(name => !tools[name]);
        if (missing.length) {
            throw new Unavailable(`Missing tools on ${host.name}: ${missing.join(', ')}`);
        }
        const uplink = await uplinkInterface(host, tools.ip);
        if (!uplink) {
            throw new Unavailable(`No default route on ${host.name} to NAT the AP to`);
        }
        if (claimDir) {
            const claim = new RadioClaim(claimDir);
            try {
                const ifname = await claim.wait();
                return new WifiAp({ host, tools, ifname, uplink, sudo: false, claim, strict });
            } catch (e) {
                claim.release();
                throw e;
            }
        }
        const sudo = !(await host.isRoot());
        if (sudo && (await host.run('sudo', ['-n', 'true'])).code !== 0) {
            throw new Unavailable(`Needs root or passwordless sudo on ${host.name}`);
        }
        await purgeStale(host, tools, sudo);
        const ifname = await findFreeRadio(host, tools.iw, uplink, Unavailable);
        return new WifiAp({ host, tools, ifname, uplink, sudo, claim: null, strict });
    }

    constructor({ host, tools, ifname, uplink, sudo, claim, strict }) {
        this._host = host;
        this._tools = tools;
        this._ifname = ifname;
        this._uplink = uplink;
        this._sudo = sudo;
        this._claim = claim;
        this._strict = strict;
        this._dir = null;
        this._prefix = null;
        this._rules = [];
        this._ipForward = null;
        this._nmcli = null;
        this._nmRadio = null;
        this._firewallCmd = null;
        this._onExit = () => this.release();
        process.once('exit', this._onExit);
        console.log(`Wi-Fi AP on ${host.name} ${ifname}, NAT through ${uplink}`);
    }

    async start({ ssid = SSID_PREFIX + crypto.randomBytes(4).toString('hex'), passphrase = crypto.randomBytes(16).toString('hex'),
            channel = 6 } = {}) {
        this._dir = await this._host.mkdtemp();
        this._prefix = await this._freeSubnet();
        await this._unmanage();
        await this._sh('ip', ['addr', 'flush', 'dev', this._ifname]);
        await this._host.writeFile(this._path('hostapd.conf'), hostapdConfig({
            ifname: this._ifname,
            ctrlDir: this._path('ctrl'),
            ssid,
            passphrase,
            channel,
            country: process.env.DEVICE_OS_TEST_RUNNER_WIFI_AP_COUNTRY
        }));
        try {
            await this._sh('hostapd', ['-B', '-P', this._path('hostapd.pid'), '-f', this._path('hostapd.log'),
                this._path('hostapd.conf')]);
            await this._waitEnabled();
        } catch (e) {
            throw new Error(`${e.message}\n${this._host.readText(this._path('hostapd.log'))}`);
        }
        await this._ensureAddress();
        await this._sh('dnsmasq', [
            '--conf-file=/dev/null',
            '--no-hosts',
            '--user=root',
            '--bind-interfaces',
            '--except-interface=lo',
            `--interface=${this._ifname}`,
            `--listen-address=${this._gateway}`,
            `--dhcp-range=${this._prefix}.100,${this._prefix}.199,255.255.255.0,1h`,
            `--dhcp-option=option:router,${this._gateway}`,
            `--dhcp-option=option:dns-server,${this._gateway}`,
            `--dhcp-leasefile=${this._path('dnsmasq.leases')}`,
            `--pid-file=${this._path('dnsmasq.pid')}`,
            `--log-facility=${this._path('dnsmasq.log')}`,
            '--log-dhcp'
        ]);
        await this._enableNat();
        return { ssid, passphrase };
    }

    async disable() {
        await this._command('DISABLE');
    }

    async enable() {
        await this._command('ENABLE');
        await this._waitEnabled();
        await this._ensureAddress();
    }

    async stations() {
        return (await this.cli('list_sta')).split('\n').map(l => l.trim()).filter(l => MAC_RE.test(l));
    }

    async waitStation(timeout) {
        const deadline = Date.now() + timeout;
        for (;;) {
            const stations = await this.stations();
            if (stations.length) {
                return stations[0];
            }
            if (Date.now() >= deadline) {
                throw new Error('No station connected to the AP');
            }
            await delay(POLL_INTERVAL);
        }
    }

    async bssid() {
        const m = /^bssid\[0\]=(\S+)$/m.exec(await this.cli('status'));
        if (!m) {
            throw new Error('Unknown AP BSSID');
        }
        return m[1];
    }

    async sendAction(sta, body) {
        const bssid = macBytes(await this.bssid());
        const header = Buffer.concat([ACTION_FRAME_CONTROL, Buffer.alloc(2), macBytes(sta), bssid, bssid, Buffer.alloc(2)]);
        const out = await this.cli('raw', 'MGMT_TX', Buffer.concat([header, body]).toString('hex'));
        if (out === 'UNKNOWN COMMAND') {
            const Unavailable = this._strict ? Error : WifiApUnavailableError;
            throw new Unavailable(`hostapd on ${this._host.name} is built without CONFIG_TESTING_OPTIONS, no MGMT_TX`);
        }
        if (out !== 'OK') {
            throw new Error(`hostapd_cli MGMT_TX: ${out}`);
        }
    }

    async cli(...args) {
        const r = await this._host.run(this._tools.hostapd_cli, ['-p', this._path('ctrl'), '-i', this._ifname, ...args], { sudo: this._sudo });
        if (r.code !== 0) {
            throw new Error(`hostapd_cli ${args[0]} failed: ${(r.stderr || r.stdout).trim()}`);
        }
        return r.stdout.trim();
    }

    release() {
        process.removeListener('exit', this._onExit);
        const errors = [];
        const sh = (file, args) => {
            const r = this._host.runSync(file, args, { sudo: this._sudo });
            if (r.code !== 0) {
                errors.push(`${path.basename(file)} ${args.join(' ')}: ${(r.stderr || r.stdout).trim()}`);
            }
        };
        if (this._dir) {
            this._stop('dnsmasq.pid', sh);
            this._stop('hostapd.pid', sh);
        }
        for (const { table, chain, args } of this._rules.reverse()) {
            sh(this._tools.iptables, ['-t', table, '-D', chain, ...args]);
        }
        this._rules = [];
        if (this._ipForward !== null) {
            sh(this._tools.sysctl, ['-qw', `net.ipv4.ip_forward=${this._ipForward}`]);
            this._ipForward = null;
        }
        if (this._firewallCmd) {
            sh(this._firewallCmd, ['--zone=trusted', `--remove-interface=${this._ifname}`]);
            this._firewallCmd = null;
        }
        sh(this._tools.ip, ['addr', 'flush', 'dev', this._ifname]);
        if (this._nmRadio) {
            sh(this._nmRadio, ['radio', 'wifi', 'off']);
            this._nmRadio = null;
        }
        if (this._nmcli) {
            sh(this._nmcli, ['device', 'set', this._ifname, 'managed', 'yes']);
            this._nmcli = null;
        }
        if (this._dir) {
            sh(this._tools.rm, ['-rf', this._dir]);
            this._dir = null;
        }
        if (this._claim) {
            this._claim.release();
            this._claim = null;
        }
        if (errors.length) {
            console.log(`Wi-Fi AP cleanup: ${errors.join('; ')}`);
        }
    }

    get _gateway() {
        return `${this._prefix}.1`;
    }

    _path(name) {
        return path.join(this._dir, name);
    }

    _sh(tool, args) {
        return check(this._host, this._tools[tool], args, { sudo: this._sudo });
    }

    async _command(...args) {
        const out = await this.cli('raw', ...args);
        if (out !== 'OK') {
            throw new Error(`hostapd_cli ${args[0]}: ${out}`);
        }
    }

    async _waitEnabled() {
        const deadline = Date.now() + START_TIMEOUT;
        for (;;) {
            const r = await this._host.run(this._tools.hostapd_cli, ['-p', this._path('ctrl'), '-i', this._ifname, 'status'], { sudo: this._sudo });
            if (r.code === 0 && /^state=ENABLED$/m.test(r.stdout)) {
                return;
            }
            if (Date.now() >= deadline) {
                throw new Error('hostapd did not enable the AP');
            }
            await delay(POLL_INTERVAL);
        }
    }

    async _ensureAddress() {
        const addrs = await this._sh('ip', ['-4', 'addr', 'show', 'dev', this._ifname]);
        if (!addrs.includes(`inet ${this._gateway}/`)) {
            await this._sh('ip', ['addr', 'add', `${this._gateway}/24`, 'dev', this._ifname]);
        }
        await this._sh('ip', ['link', 'set', this._ifname, 'up']);
    }

    async _freeSubnet() {
        const used = (await check(this._host, this._tools.ip, ['-4', 'route', 'show', 'table', 'all'])) +
                (await check(this._host, this._tools.ip, ['-4', 'addr']));
        const prefix = SUBNET_PREFIXES.find(p => !used.includes(`${p}.`));
        if (!prefix) {
            throw new Error('No free subnet for the AP');
        }
        return prefix;
    }

    async _unmanage() {
        const nmcli = this._tools.nmcli;
        if (!nmcli) {
            return;
        }
        const r = await this._host.run(nmcli, ['-t', '-f', 'DEVICE,STATE', 'device', 'status']);
        const line = r.code === 0 && r.stdout.split('\n').find(l => l.startsWith(`${this._ifname}:`));
        if (!line) {
            return;
        }
        if (!line.endsWith(':unmanaged')) {
            await check(this._host, nmcli, ['device', 'set', this._ifname, 'managed', 'no'], { sudo: this._sudo });
            this._nmcli = nmcli;
        }
        if ((await check(this._host, nmcli, ['radio', 'wifi'])).trim() === 'disabled') {
            await check(this._host, nmcli, ['radio', 'wifi', 'on'], { sudo: this._sudo });
            this._nmRadio = nmcli;
            await this._waitUnblocked();
        }
    }

    async _waitUnblocked() {
        const deadline = Date.now() + START_TIMEOUT;
        for (;;) {
            const r = await this._host.run('sh', ['-c', 'cat /sys/class/net/"$1"/phy80211/rfkill*/soft', 'sh', this._ifname]);
            if (r.code !== 0 || !r.stdout.split('\n').includes('1')) {
                return;
            }
            if (Date.now() >= deadline) {
                throw new Error(`${this._ifname} is still rfkill blocked`);
            }
            await delay(POLL_INTERVAL);
        }
    }

    async _enableNat() {
        const ipForward = this._host.readText('/proc/sys/net/ipv4/ip_forward');
        if (ipForward !== '1') {
            await this._sh('sysctl', ['-qw', 'net.ipv4.ip_forward=1']);
            this._ipForward = ipForward || '0';
        }
        const comment = ['-m', 'comment', '--comment', RULE_COMMENT];
        const rules = [
            ['nat', 'POSTROUTING', '-s', `${this._prefix}.0/24`, '-o', this._uplink, ...comment, '-j', 'MASQUERADE'],
            ['filter', 'FORWARD', '-i', this._ifname, '-o', this._uplink, ...comment, '-j', 'ACCEPT'],
            ['filter', 'FORWARD', '-i', this._uplink, '-o', this._ifname, '-m', 'conntrack', '--ctstate', 'RELATED,ESTABLISHED',
                ...comment, '-j', 'ACCEPT']
        ];
        for (const [table, chain, ...args] of rules) {
            await this._sh('iptables', ['-t', table, '-I', chain, ...args]);
            this._rules.push({ table, chain, args });
        }
        const firewallCmd = this._tools['firewall-cmd'];
        if (firewallCmd && (await this._host.run(firewallCmd, ['--state'], { sudo: this._sudo })).code === 0) {
            await check(this._host, firewallCmd, ['--zone=trusted', `--change-interface=${this._ifname}`], { sudo: this._sudo });
            this._firewallCmd = firewallCmd;
        }
    }

    _stop(pidFile, sh) {
        const pid = this._host.readText(this._path(pidFile));
        if (!pid) {
            return;
        }
        sh(this._tools.kill, [pid]);
        const deadline = Date.now() + STOP_TIMEOUT;
        while (this._host.exists(`/proc/${pid}`) && Date.now() < deadline) {
            sleepSync(100);
        }
    }
}

module.exports = {
    WifiAp,
    WifiApUnavailableError,
    ACQUIRE_TIMEOUT
};
