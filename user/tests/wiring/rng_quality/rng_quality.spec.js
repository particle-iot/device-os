suite('RNG quality');

platform('gen3', 'gen4');
timeout(60 * 60 * 1000);

let device = null;

const WORDS_PER_BLOCK = 16383; // 65532 bytes, the payload limit is 65535 (uint16_t wLength)
const BLOCKS = 24;

function berlekampMassey(bits) {
    const n = bits.length;
    const C = new Uint8Array(n + 1);
    const B = new Uint8Array(n + 1);
    const T = new Uint8Array(n + 1);
    C[0] = B[0] = 1;
    let L = 0;
    let m = -1;
    for (let N = 0; N < n; ++N) {
        let d = 0;
        for (let i = 0; i <= L; ++i) {
            d ^= C[i] & bits[N - i];
        }
        if (d) {
            T.set(C);
            const shift = N - m;
            for (let i = 0; i + shift <= n; ++i) {
                C[i + shift] ^= B[i];
            }
            if (2 * L <= N) {
                L = N + 1 - L;
                m = N;
                B.set(T);
            }
        }
    }
    return L;
}

function chiSquareBytes(bytes) {
    const counts = new Array(256).fill(0);
    for (const b of bytes) {
        counts[b]++;
    }
    const expected = bytes.length / 256;
    let chi = 0;
    for (const c of counts) {
        chi += (c - expected) ** 2 / expected;
    }
    return chi;
}

function bitStats(bytes) {
    let ones = 0;
    let max = 0;
    let cur = 0;
    let last = -1;
    for (const byte of bytes) {
        for (let i = 7; i >= 0; --i) {
            const bit = (byte >>> i) & 1;
            ones += bit;
            if (bit === last) {
                ++cur;
            } else {
                cur = 1;
                last = bit;
            }
            max = Math.max(max, cur);
        }
    }
    return { ones, maxRun: max };
}

function shannonEntropyBytes(bytes) {
    const counts = new Array(256).fill(0);
    for (const b of bytes) {
        counts[b]++;
    }
    let h = 0;
    for (const c of counts) {
        if (c > 0) {
            const p = c / bytes.length;
            h -= p * Math.log2(p);
        }
    }
    return h;
}

async function pullRngBlock(usbDev, n) {
    const req = Buffer.from(JSON.stringify({ c: 'R', n }));
    const rep = await usbDev.sendControlRequest(10, req);
    expect(rep.result).to.equal(0);
    expect(Buffer.isBuffer(rep.data)).to.be.true;
    return rep.data;
}

before(function() {
    device = this.particle.devices[0];
});

test('RNG_01_not_stuck', async function() {
});

test('RNG_02_replay_across_soft_reset', async function() {
});

test('RNG_03_host_side_statistical_analysis', async function() {
    this.slow(10 * 60 * 1000);
    const usbDev = await device.getUsbDevice();
    const blocks = [];
    for (let i = 0; i < BLOCKS; ++i) {
        blocks.push(await pullRngBlock(usbDev, WORDS_PER_BLOCK));
    }
    const bytes = Buffer.concat(blocks);
    expect(bytes.length).to.equal(BLOCKS * WORDS_PER_BLOCK * 4);

    const words = new Array(bytes.length / 4);
    for (let i = 0; i < words.length; ++i) {
        words[i] = bytes.readUInt32LE(i * 4);
    }

    const n = bytes.length * 8;
    const stats = bitStats(bytes);
    const sigma = Math.sqrt(n) / 2;
    expect(stats.ones).to.be.within(n / 2 - 5 * sigma, n / 2 + 5 * sigma);

    const chi = chiSquareBytes(bytes);
    expect(chi).to.be.within(255 - 6 * Math.sqrt(510), 255 + 6 * Math.sqrt(510));

    expect(stats.maxRun).to.be.lessThan(40);

    const planeN = 512;
    const threshold = planeN / 2 - 32;
    for (let bit = 0; bit < 32; ++bit) {
        const plane = new Array(planeN);
        for (let i = 0; i < planeN; ++i) {
            plane[i] = (words[i] >>> bit) & 1;
        }
        expect(berlekampMassey(plane)).to.be.at.least(threshold);
    }

    console.log(`words: ${words.length}, ones: ${stats.ones}/${n}, chi2: ${chi.toFixed(1)}, ` +
        `maxrun: ${stats.maxRun}, entropy: ${shannonEntropyBytes(bytes).toFixed(4)} bits/byte`);
});

test('RNG_04_initial_generation_1', async function() {
});

test('RNG_04_initial_generation_2', async function() {
});
