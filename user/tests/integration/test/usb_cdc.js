'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');
const { SerialPort } = require('serialport');

const DEFAULT_SYS_ROOT = '/sys';
const LINUX_TTY_RE = /^tty(?:ACM|USB)\d+$/;
const OPEN_PORTS = new Set();

process.once('exit', () => {
    for (const port of OPEN_PORTS) {
        port.cleanupSync();
    }
});

function sleep(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

function readFile(file) {
    try {
        return fs.readFileSync(file, 'utf8').trim();
    } catch (_) {
        return '';
    }
}

function run(command, args, options = {}) {
    const result = spawnSync(command, args, { encoding: 'utf8', ...options });
    if (result.error) {
        throw result.error;
    }
    if (result.status !== 0) {
        const output = `${result.stdout || ''}${result.stderr || ''}`.trim();
        throw new Error(`${command} failed with exit code ${result.status}${output ? `: ${output}` : ''}`);
    }
    return result.stdout;
}

async function waitFor(fn, timeoutMs, description, intervalMs = 250) {
    const deadline = Date.now() + timeoutMs;
    let lastError = null;
    for (;;) {
        try {
            const value = fn();
            if (value) {
                return value;
            }
        } catch (err) {
            lastError = err;
        }
        if (Date.now() >= deadline) {
            const detail = lastError ? `: ${lastError.message}` : '';
            throw new Error(`Timed out waiting for ${description} (${timeoutMs / 1000}s)${detail}`);
        }
        await sleep(intervalMs);
    }
}

function findLinuxTtyByUsbSerial(serial, sysRoot = DEFAULT_SYS_ROOT) {
    const expected = serial.toLowerCase();
    const sysTtyPath = path.join(sysRoot, 'class', 'tty');
    const entries = fs.readdirSync(sysTtyPath);
    for (const entry of entries) {
        if (!LINUX_TTY_RE.test(entry)) {
            continue;
        }
        let devicePath;
        try {
            devicePath = fs.realpathSync(path.join(sysTtyPath, entry, 'device'));
        } catch (_) {
            continue;
        }
        while (devicePath.startsWith(`${sysRoot}/`)) {
            const candidate = readFile(path.join(devicePath, 'serial'));
            if (candidate && candidate.toLowerCase() === expected) {
                return entry;
            }
            const parent = path.dirname(devicePath);
            if (parent === devicePath) {
                break;
            }
            devicePath = parent;
        }
    }
    return '';
}

function findValueInTree(node, names) {
    if (!node || typeof node !== 'object') {
        return '';
    }
    for (const name of names) {
        if (typeof node[name] === 'string' && node[name]) {
            return node[name];
        }
    }
    const children = Array.isArray(node.IORegistryEntryChildren) ? node.IORegistryEntryChildren : [];
    for (const child of children) {
        const value = findValueInTree(child, names);
        if (value) {
            return value;
        }
    }
    return '';
}

function findDarwinPortByUsbSerial(serial) {
    const xml = run('ioreg', ['-a', '-p', 'IOUSB']);
    const json = run('plutil', ['-convert', 'json', '-o', '-', '-'], { input: xml });
    const parsed = JSON.parse(json);
    const roots = Array.isArray(parsed) ? parsed : [parsed];
    const expected = serial.toLowerCase();

    const visit = (node) => {
        if (!node || typeof node !== 'object') {
            return '';
        }
        const candidate = node['USB Serial Number'] || node.kUSBSerialNumberString || '';
        if (candidate && candidate.toLowerCase() === expected) {
            const port = findValueInTree(node, ['IOCalloutDevice', 'IODialinDevice']);
            if (port) {
                return port;
            }
        }
        const children = Array.isArray(node.IORegistryEntryChildren) ? node.IORegistryEntryChildren : [];
        for (const child of children) {
            const port = visit(child);
            if (port) {
                return port;
            }
        }
        return '';
    };

    for (const root of roots) {
        const port = visit(root);
        if (port) {
            return port;
        }
    }

    // Useful for a normal developer machine with one attached CDC device even
    // if a macOS release changes the IORegistry property layout.
    const ports = fs.readdirSync('/dev')
        .filter((name) => name.startsWith('cu.usbmodem'))
        .map((name) => path.join('/dev', name));
    return ports.length === 1 ? ports[0] : '';
}

class UsbCdcPort {
    constructor(deviceId) {
        this.deviceId = deviceId;
        this.devicePath = '';
        this.ttyName = '';
        this.devRoot = '';
        this.sysRoot = '';
        this.port = null;
        this.portError = null;
        this.receivedData = Buffer.alloc(0);
        this.mountedDevtmpfs = false;
        this.mountedSysfs = false;
    }

    async open(timeoutMs = 30000) {
        OPEN_PORTS.add(this);
        try {
            this.devicePath = await this.resolveDevicePath(timeoutMs);
            await this.openDevice();
            return this.devicePath;
        } catch (err) {
            this.cleanupSync();
            throw err;
        }
    }

    async resolveDevicePath(timeoutMs) {
        if (process.platform === 'darwin') {
            return waitFor(
                () => findDarwinPortByUsbSerial(this.deviceId),
                timeoutMs,
                `a CDC port for USB serial ${this.deviceId}`
            );
        }
        if (process.platform !== 'linux') {
            throw new Error(`USB CDC integration tests are not supported on ${process.platform}`);
        }

        let sysRoot = this.mountedSysfs ? this.sysRoot : DEFAULT_SYS_ROOT;
        try {
            this.ttyName = findLinuxTtyByUsbSerial(this.deviceId, sysRoot);
        } catch (_) {
            this.ttyName = '';
            // Try a private sysfs mount when running as root.
        }
        if (!this.ttyName && !this.mountedSysfs && process.getuid && process.getuid() === 0) {
            const suffix = this.deviceId.replace(/[^a-zA-Z0-9]/g, '').slice(-8);
            this.sysRoot = path.join('/run', `device-os-test-sys-${process.pid}-${suffix}`);
            fs.mkdirSync(this.sysRoot, { recursive: true });
            try {
                run('mount', ['-t', 'sysfs', '-o', 'ro,nosuid,nodev,noexec', 'sysfs', this.sysRoot]);
                this.mountedSysfs = true;
                sysRoot = this.sysRoot;
            } catch (_) {
                try {
                    fs.rmdirSync(this.sysRoot);
                } catch (_err) {
                    // Best-effort cleanup.
                }
                this.sysRoot = '';
            }
        }

        this.ttyName = this.ttyName || await waitFor(
            () => findLinuxTtyByUsbSerial(this.deviceId, sysRoot),
            timeoutMs,
            `a CDC tty for USB serial ${this.deviceId}`
        );

        const regularPath = path.join('/dev', this.ttyName);
        if (fs.existsSync(regularPath)) {
            return regularPath;
        }
        if (!process.getuid || process.getuid() !== 0) {
            throw new Error(`${regularPath} is missing and mounting devtmpfs requires root`);
        }
        if (!this.mountedDevtmpfs) {
            const suffix = this.deviceId.replace(/[^a-zA-Z0-9]/g, '').slice(-8);
            this.devRoot = path.join('/run', `device-os-test-dev-${process.pid}-${suffix}`);
            fs.mkdirSync(this.devRoot, { recursive: true });
            run('mount', ['-t', 'devtmpfs', '-o', 'mode=0755,nosuid', 'devtmpfs', this.devRoot]);
            this.mountedDevtmpfs = true;
        }
        return waitFor(
            () => {
                const candidate = path.join(this.devRoot, this.ttyName);
                return fs.existsSync(candidate) && candidate;
            },
            5000,
            `${this.ttyName} in the private devtmpfs mount`
        );
    }

    async openDevice() {
        this.portError = null;
        const port = new SerialPort({
            path: this.devicePath,
            baudRate: 9600,
            hupcl: true,
            autoOpen: false
        });
        this.port = port;
        port.on('data', (data) => {
            this.receivedData = Buffer.concat([this.receivedData, data]);
        });
        port.on('error', (err) => {
            this.portError = err;
        });
        await new Promise((resolve, reject) => {
            port.open((err) => err ? reject(err) : resolve());
        });
        await sleep(250);
        if (!this.isOpen()) {
            throw new Error(`CDC port closed during startup${this.portError ? `: ${this.portError.message}` : ''}`);
        }
    }

    async write(data) {
        if (!this.isOpen()) {
            throw new Error('USB CDC port is not open');
        }
        const buffer = Buffer.isBuffer(data) ? data : Buffer.from(data);
        const port = this.port;
        await new Promise((resolve, reject) => {
            port.write(buffer, (err) => err ? reject(err) : resolve());
        });
        await new Promise((resolve, reject) => {
            port.drain((err) => err ? reject(err) : resolve());
        });
    }

    async read(size, timeoutMs = 5000) {
        await waitFor(
            () => {
                if (this.portError) {
                    throw this.portError;
                }
                return this.receivedData.length >= size;
            },
            timeoutMs,
            `${size} bytes from ${this.devicePath}`,
            25
        );
        const data = this.receivedData.subarray(0, size);
        this.receivedData = this.receivedData.subarray(size);
        return data;
    }

    discardInput() {
        this.receivedData = Buffer.alloc(0);
    }

    pauseInput() {
        if (!this.isOpen()) {
            throw new Error('USB CDC port is not open');
        }
        // Keep the tty open (and DTR asserted), but stop draining its readable
        // stream. Once the host buffers fill, the device observes USB TX
        // backpressure.
        this.port.pause();
    }

    resumeInput() {
        if (this.isOpen()) {
            this.port.resume();
        }
    }

    isOpen() {
        return !!this.port && this.port.isOpen;
    }

    async disconnect() {
        const port = this.port;
        if (!port) {
            return;
        }
        if (port.isOpen) {
            await new Promise((resolve, reject) => {
                port.close((err) => err ? reject(err) : resolve());
            });
        }
        if (this.port === port) {
            this.port = null;
        }
    }

    async reconnect(timeoutMs = 30000) {
        const deadline = Date.now() + timeoutMs;
        let lastError = null;
        try {
            await this.disconnect();
        } catch (err) {
            // Continue reopening so one failed test does not poison the rest
            // of the suite, but retain the close error if all retries fail.
            lastError = err;
        }
        // Keep the tty closed long enough for HUPCL/DTR-low to reach the
        // device before opening a new handle and asserting DTR again.
        await sleep(250);
        while (Date.now() < deadline) {
            try {
                this.devicePath = await this.resolveDevicePath(Math.min(1000, deadline - Date.now()));
                await this.openDevice();
                return this.devicePath;
            } catch (err) {
                lastError = err;
                try {
                    await this.disconnect();
                } catch (_) {
                    // The next iteration gets a fresh port object.
                }
                await sleep(250);
            }
        }
        throw new Error(`Failed to reopen USB CDC port: ${lastError ? lastError.message : 'timeout'}`);
    }

    async close() {
        try {
            await this.disconnect();
        } finally {
            this.unmountFilesystems();
            OPEN_PORTS.delete(this);
        }
    }

    cleanupSync() {
        const port = this.port;
        this.port = null;
        if (port && port.isOpen) {
            try {
                port.close(() => {});
            } catch (_) {
                // Best-effort cleanup.
            }
        }
        this.unmountFilesystems();
        OPEN_PORTS.delete(this);
    }

    unmountFilesystems() {
        if (this.mountedDevtmpfs) {
            spawnSync('umount', [this.devRoot], { stdio: 'ignore' });
            this.mountedDevtmpfs = false;
        }
        if (this.devRoot) {
            try {
                fs.rmdirSync(this.devRoot);
            } catch (_) {
                // Best-effort cleanup. Concourse has its own mount namespace.
            }
            this.devRoot = '';
        }
        if (this.mountedSysfs) {
            spawnSync('umount', [this.sysRoot], { stdio: 'ignore' });
            this.mountedSysfs = false;
        }
        if (this.sysRoot) {
            try {
                fs.rmdirSync(this.sysRoot);
            } catch (_) {
                // Best-effort cleanup. Concourse has its own mount namespace.
            }
            this.sysRoot = '';
        }
    }
}

module.exports = {
    UsbCdcPort,
    findDarwinPortByUsbSerial,
    findLinuxTtyByUsbSerial
};
