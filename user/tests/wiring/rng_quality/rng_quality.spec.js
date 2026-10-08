suite('RNG quality');

platform('gen3', 'gen4');
timeout(60 * 60 * 1000);

let device = null;
let coldBootStream = null;

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

function mcvMinEntropy(maxCount, n) {
    const pHat = maxCount / n;
    const pUpper = Math.min(1, pHat + 2.576 * Math.sqrt(pHat * (1 - pHat) / (n - 1)));
    return { pHat, pUpper, h: -Math.log2(pUpper) };
}

function markovMinEntropy(s) {
    const n = s.samples;
    const p0 = (n - s.ones) / n;
    const p1 = s.ones / n;
    const d0 = s.t00 + s.t01;
    const d1 = s.t10 + s.t11;
    if (!d0 || !d1 || !p0 || !p1) {
        return null;
    }
    const lg = x => (x > 0 ? Math.log2(x) : -Infinity);
    const l00 = lg(s.t00 / d0);
    const l01 = lg(s.t01 / d0);
    const l10 = lg(s.t10 / d1);
    const l11 = lg(s.t11 / d1);
    const L = 128;
    const logPMax = Math.max(
        lg(p0) + (L - 1) * l00,
        lg(p0) + (L / 2) * l01 + (L / 2 - 1) * l10,
        lg(p0) + l01 + (L - 2) * l11,
        lg(p1) + l10 + (L - 2) * l00,
        lg(p1) + (L / 2) * l10 + (L / 2 - 1) * l01,
        lg(p1) + (L - 1) * l11);
    return Math.min(-logPMax / L, 1);
}

function binomialCutoff(p, n, alpha) {
    if (!(p > 0 && p < 1)) {
        return null;
    }
    const lf = new Float64Array(n + 1);
    for (let i = 1; i <= n; ++i) {
        lf[i] = lf[i - 1] + Math.log(i);
    }
    const lp = Math.log(p);
    const lq = Math.log(1 - p);
    let tail = 0;
    for (let k = n; k >= 0; --k) {
        tail += Math.exp(lf[n] - lf[k] - lf[n - k] + k * lp + (n - k) * lq);
        if (tail > alpha) {
            return k + 1;
        }
    }
    return 0;
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
    const usbDev = await device.getUsbDevice();
    coldBootStream = await pullRngBlock(usbDev, 32);
});

test('RNG_04_initial_generation_2', async function() {
    expect(Buffer.isBuffer(coldBootStream)).to.be.true;
    const usbDev = await device.getUsbDevice();
    const secondStream = await pullRngBlock(usbDev, 32);
    let diffWords = 0;
    for (let i = 0; i < 32; ++i) {
        if (secondStream.readUInt32LE(i * 4) !== coldBootStream.readUInt32LE(i * 4)) {
            ++diffWords;
        }
    }
    console.log(`cold boot streams: ${diffWords}/32 words differ`);
    expect(diffWords).to.be.at.least(1);
});

test('RNG_05_reseed_stress', async function() {
});

test('RNG_06_seed_corruption_data_zeros', async function() {
});

test('RNG_07_seed_corruption_data_zeros_replay', async function() {
});

test('RNG_08_seed_corruption_data_zeros_replay', async function() {
});

test('RNG_09_seed_corruption_struct_garbage', async function() {
});

test('RNG_10_seed_corruption_struct_garbage_replay', async function() {
});

test('RNG_11_entropy_source_qualification', async function() {
    const entry = device.mailBox.pop();
    if (!entry) {
        return;
    }
    const s = JSON.parse(entry.d);
    const n = s.samples;

    const zeros = n - s.ones;
    const bitMcv = mcvMinEntropy(Math.max(s.ones, zeros), n);
    const codeMcv = mcvMinEntropy(s.code_top_count, n);
    const markov = markovMinEntropy(s);

    const mean = s.code_sum / n;
    const sd = Math.sqrt(Math.max(0, s.code_sum_squares / n - mean * mean));

    const h = bitMcv.h;
    const rctNeeded = h > 0 ? 1 + Math.ceil(20 / h) : null;
    const aptNeeded = binomialCutoff(bitMcv.pUpper, s.apt_window, Math.pow(2, -20));

    console.log(`entropy source qualification (${n} raw ADC samples, ${s.errors} failed reads)`);
    console.log(`  sample rate: ${(s.elapsed_us / n).toFixed(1)} us/sample ` +
        `(64-sample warm boot refresh = ${(64 * s.elapsed_us / n / 1000).toFixed(1)} ms)`);
    console.log(`  raw code: min=${s.code_min} max=${s.code_max} mean=${mean.toFixed(3)} ` +
        `sd=${sd.toFixed(3)} distinct=${s.code_distinct} ` +
        `top=${s.code_top} (${(100 * s.code_top_count / n).toFixed(2)}%)`);
    console.log(`  lsb: ones=${s.ones} zeros=${zeros} p_max=${bitMcv.pHat.toFixed(6)} ` +
        `p_upper=${bitMcv.pUpper.toFixed(6)}`);
    console.log(`  lsb transitions: 00=${s.t00} 01=${s.t01} 10=${s.t10} 11=${s.t11}`);
    console.log(`  min-entropy per sample: mcv=${h.toFixed(4)} bits` +
        (markov === null ? '' : ` markov=${markov.toFixed(4)} bits`) +
        ` (shipped cutoffs assume 0.25)`);
    console.log(`  min-entropy per 12-bit code: mcv=${codeMcv.h.toFixed(4)} bits`);
    console.log(`  observed rct run=${s.max_run} (cutoff ${s.rct_cutoff}), ` +
        `apt max=${s.apt_max}/${s.apt_window} (cutoff ${s.apt_cutoff})`);
    console.log(`  cutoffs implied by the measured min-entropy: rct=${rctNeeded} apt=${aptNeeded}`);

    if (s.max_run >= s.rct_cutoff || s.apt_max >= s.apt_cutoff) {
        console.log('  WARNING: this device trips the shipped health tests');
    }
    if (rctNeeded !== null && (rctNeeded > s.rct_cutoff || aptNeeded > s.apt_cutoff)) {
        console.log('  WARNING: shipped cutoffs are tighter than the measured min-entropy supports');
    }

    expect(s.code_distinct).to.be.at.least(2);
    expect(n).to.be.at.least(s.requested * 0.9);
});
