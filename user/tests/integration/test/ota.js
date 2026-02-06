const { setTimeout: delay } = require('node:timers/promises');

const DEFAULT_FLASH_TIMEOUT = 5 * 60 * 1000;

/**
 * Wait for an OTA udpate status event.
 *
 * @param {object} testCtx `this` context of the test running.
 * @param {object} [opts] Options.
 * @param {string} [opts.deviceId] Device ID.
 * @param {string} [opts.status] Expected status: `started`, `success`, `failed`.
 * @param {number} [opts.timeout] Timeout in milliseconds.
 * @returns {Promise<string>} Update status.
 */
async function waitFlashStatusEvent(testCtx, { deviceId, status, timeout = DEFAULT_FLASH_TIMEOUT } = {}) {
	if (deviceId === undefined) {
		deviceId = testCtx.particle.devices[0].id;
	}

	const timeoutAt = performance.now() + timeout;
	for (;;) {
		const statusTimeout = timeoutAt - performance.now();
		if (statusTimeout <= 0) {
			throw new Error('Timeout while waiting for flash status event');
		}
		// TODO: receiveEvent() should optionally take a device ID in case the test involves
		// multiple devices
		const data = await testCtx.particle.receiveEvent('spark/flash/status', { timeout: statusTimeout });
		console.log('spark/flash/status', data);
		const currentStatus = data.split(' ')[0]; // Ignore the filename
		if (status === undefined || currentStatus === status) {
			return currentStatus;
		}
	}
}

/**
 * Send an OTA update to a device.
 *
 * Waits for the completion of the update.
 *
 * @param {object} testCtx `this` context of the test running.
 * @param {Buffer} fwData Firmware module data.
 * @param {object} [opts] Options.
 * @param {string} [opts.deviceId] Device ID.
 * @param {string} [opts.filename] Filename.
 * @param {number} [opts.timeout] Timeout in milliseconds.
 * @param {number} [opts.retries] Number of retries.
 * @param {number} [opts.backoff] Initial backoff interval.
 */
async function flash(testCtx, fwData, {
	deviceId,
	filename = 'firmware.bin',
	timeout = DEFAULT_FLASH_TIMEOUT,
	retries = 3,
	backoff = 3000
} = {}) {
	if (deviceId === undefined) {
		deviceId = testCtx.particle.devices[0].id;
	}
	const { instance: api, token: auth } = testCtx.particle.apiClient;

	// Flush any previously received status events
	for (;;) {
		try {
			await waitFlashStatusEvent(testCtx, { timeout: 1 });
		} catch (err) {
			break;
		}
	}

	let attempts = 0;

	for (;;) {
		try {
			const timeoutAt = performance.now() + timeout;
			await api.flashDevice({ deviceId, files: { [filename]: fwData }, auth });

			for (;;) {
				const statusTimeout = timeoutAt - performance.now();
				const status = await waitFlashStatusEvent(testCtx, { deviceId, timeout: statusTimeout });
				if (status === 'started') {
					continue;
				}
				if (status === 'success') {
					break;
				}
				throw new Error('Failed to flash device OTA');
			}
			break;
		} catch (err) {
			if (attempts >= retries) {
				throw err;
			}
			console.log(err);
			console.log(`Retrying in ${backoff}ms`);
			await delay(backoff);
			backoff *= 2;
			++attempts;
		}
	}
}

module.exports = {
	waitFlashStatusEvent,
	flash
};
