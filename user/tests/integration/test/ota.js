const { setTimeout: delay } = require('node:timers/promises');

async function flashDevice(testCtx, fwData, {
	deviceId,
	filename = 'firmware.bin',
	timeout = 5 * 60 * 1000,
	retries = 3,
	backoff = 3000
} = {}) {
	const { instance: api, token: auth } = testCtx.particle.apiClient;

	if (deviceId === undefined) {
		deviceId = testCtx.particle.devices[0].id;
	}

	// Flush any previously received status events
	for (;;) {
		try {
			await testCtx.particle.receiveEvent('spark/flash/status', { timeout: 1 });
		} catch (err) {
			break;
		}
	}

	let attempts = 0;

	for (;;) {
		try {
			let timeoutAt = performance.now() + timeout;
			await api.flashDevice({ deviceId, files: { [filename]: fwData }, auth });

			for (;;) {
				const eventTimeout = timeoutAt - performance.now();
				if (eventTimeout <= 0) {
					throw new Error('Timeout while flashing device OTA');
				}

				const data = await testCtx.particle.receiveEvent('spark/flash/status', { timeout: eventTimeout });
				console.log('spark/flash/status:', data);
				if (data.startsWith('started')) {
					continue;
				}
				if (data.startsWith('success')) {
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
	flashDevice
};
