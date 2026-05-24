'use strict';

// Generic docker subprocess helpers shared by HIL test fixtures.
//
// This module is intentionally test-agnostic: it knows nothing about
// tethering, any specific image, or the env vars / volumes a particular
// scenario needs. Callers (e.g. .../wiring/tethering_*/...spec.js) build
// their own docker-run args and invoke runContainer() / spawnDocker().

const { spawn } = require('child_process');

/**
 * Spawn `docker <args>` and capture stdout+stderr.
 *
 * @param {string[]} args Args to pass to the docker CLI.
 * @param {object} [opts]
 * @param {boolean} [opts.stream=false] Mirror child output to process.stdout live.
 * @param {number} [opts.timeoutMs=0] SIGTERM (then SIGKILL after 5s) if exceeded; 0 disables.
 * @returns {Promise<{ code: number|null, signal: string|null, output: string, killed: boolean }>}
 */
function spawnDocker(args, { stream = false, timeoutMs = 0 } = {}) {
    return new Promise((resolve) => {
        const p = spawn('docker', args, { stdio: ['ignore', 'pipe', 'pipe'] });
        let output = '';
        const onData = (chunk) => {
            const s = chunk.toString();
            output += s;
            if (stream) {
                process.stdout.write(s);
            }
        };
        p.stdout.on('data', onData);
        p.stderr.on('data', onData);

        let killed = false;
        let timer = null;
        if (timeoutMs > 0) {
            timer = setTimeout(() => {
                killed = true;
                p.kill('SIGTERM');
                setTimeout(() => p.kill('SIGKILL'), 5000);
            }, timeoutMs);
        }

        p.on('close', (code, signal) => {
            if (timer) clearTimeout(timer);
            resolve({ code, signal, output, killed });
        });
    });
}

/**
 * Ensure an image is locally available; pull it if not.
 *
 * @param {string} imageRef Image tag or digest.
 * @returns {Promise<string>} The same imageRef on success.
 */
async function ensureImage(imageRef) {
    const inspect = await spawnDocker(['image', 'inspect', imageRef]);
    if (inspect.code === 0) {
        return imageRef;
    }
    console.log(`Pulling docker image ${imageRef} ...`);
    const pull = await spawnDocker(['pull', imageRef], { stream: true });
    if (pull.code !== 0) {
        const tail = pull.output.split('\n').slice(-50).join('\n');
        throw new Error(`docker pull ${imageRef} failed (exit ${pull.code}):\n${tail}`);
    }
    return imageRef;
}

/**
 * Best-effort remove of a container by name. Ignores errors, the container
 * may simply not exist.
 *
 * @param {string} name Container name.
 */
async function removeContainer(name) {
    await spawnDocker(['rm', '-f', name]);
}

/**
 * Run a container with `--rm`. Cleans up any lingering container with the
 * same name before starting.
 *
 * The caller supplies whatever flags their scenario needs (--privileged, -e,
 * -v, --network, --tmpfs, etc) via `args`. This function only adds
 * `run --rm --name <name>`, then the image, then any `cmd`.
 *
 * @param {object} opts
 * @param {string} opts.image Image ref to run.
 * @param {string} opts.name Container name.
 * @param {string[]} [opts.args=[]] Flags between `--name` and the image.
 * @param {string[]} [opts.cmd=[]] Command/args appended after the image.
 * @param {boolean} [opts.stream=false] Mirror container output live.
 * @param {number} [opts.timeoutMs=0] SIGTERM the docker run after this many ms.
 * @returns {Promise<{ code, signal, output, killed }>}
 */
async function runContainer({ image, name, args = [], cmd = [], stream = false, timeoutMs = 0 }) {
    await removeContainer(name);
    const runArgs = ['run', '--rm', '--name', name, ...args, image, ...cmd];
    return spawnDocker(runArgs, { stream, timeoutMs });
}

module.exports = {
    spawnDocker,
    ensureImage,
    removeContainer,
    runContainer
};
