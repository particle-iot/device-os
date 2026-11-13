'use strict';

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { HalModuleParser } = require('binary-version-reader');

const SHA_256_OFFSET_FROM_END = 4 + 2 + 32;

function findBins(dir) {
	const bins = [];
	for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
		const p = path.join(dir, entry.name);
		if (entry.isDirectory()) {
			bins.push(...findBins(p));
		} else if (entry.isFile() && entry.name.endsWith('.bin')) {
			bins.push(p);
		}
	}
	return bins;
}

async function verifyBin(file) {
	const info = await new HalModuleParser().parseFile(file);
	const data = info.fileBuffer;
	const errors = [];

	if (data.length <= SHA_256_OFFSET_FROM_END) {
		return [`too small to be a module binary (${data.length} bytes)`];
	}

	if (!info.crc.ok) {
		errors.push(`CRC stored ${info.crc.storedCrc} expected ${info.crc.actualCrc}`);
	}

	const sha = crypto.createHash('sha256').update(data.subarray(0, data.length - SHA_256_OFFSET_FROM_END)).digest('hex');
	if (info.suffixInfo.fwUniqueId !== sha) {
		errors.push(`SHA-256 stored ${info.suffixInfo.fwUniqueId} expected ${sha}`);
	}

	return errors;
}

async function main() {
	const dir = process.argv[2];
	if (!dir) {
		console.error(':::: Usage: verify-module-bins.js <dir>');
		return 1;
	}

	const bins = findBins(dir);
	if (bins.length === 0) {
		console.error(`:::: Error: No binaries found in ${dir}`);
		return 1;
	}

	let failed = 0;
	for (const file of bins) {
		let errors;
		try {
			errors = await verifyBin(file);
		} catch (err) {
			errors = [`failed to parse: ${err.message || err}`];
		}

		if (errors.length > 0) {
			failed++;
			console.error(`:::: Error: ${file} is not a valid module binary`);
			for (const e of errors) {
				console.error(`::::   ${e}`);
			}
		} else {
			console.log(`:::: OK: ${file}`);
		}
	}

	return failed > 0 ? 1 : 0;
}

main().then(code => {
	process.exitCode = code;
}, err => {
	console.error(err);
	process.exitCode = 1;
});
