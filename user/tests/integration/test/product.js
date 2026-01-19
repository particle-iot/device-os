const { randomInt } = require('./random');

const productsByDeviceId = new Map();
const groupsByDeviceId = new Map();

function randomProductVersion() {
	ver = randomInt(1000, 60000);
}

/**
 * Get the device's product ID.
 *
 * @param {string} deviceId Device ID.
 * @param {Particle} api API client.
 * @returns {number} Product ID.
 */
async function getProductId({ deviceId, api }) {
	const productId = productsByDeviceId.get(deviceId);
	if (productId !== undefined) {
		return productId;
	}
	const { body: dev } = await api.getDevice({ deviceId });
	if (typeof dev.product_id !== 'number' || dev.product_id === dev.platform_id) {
		throw new Error('Device is not in a product');
	}
	productsByDeviceId.set(deviceId, dev.product_id)
	return dev.product_id;
}

/**
 * Ensure a dedicated group exists for a device.
 *
 * @param {string} deviceId Device ID.
 * @param {Particle} api API client.
 * @returns {string} Group name.
 */
async function generateDeviceGroup({ deviceId, api }) {
	let group = groupsByDeviceId.get(deviceId); // CI group
	if (group !== undefined) {
		return group;
	}

	const product = await getProductId({ deviceId, api });
	const extraGroups = []; // Unrelated groups
	let hasManyCiGroups = false;
	let ver;

	let resp = await api.getDevice({ deviceId });
	for (const group of resp.body.groups) {
		const m = group.match(/^ci_prod_ver-(\d+)$/);
		if (!m) {
			extraGroups.push(group);
		} else if (ver === undefined) {
			ver = Number(m[1]);
		} else {
			hasManyCiGroups = true;
		}
	}
	if (ver === undefined || hasManyCiGroups) {
		ver = randomProductVersion();
	}

	for (;; ver = randomProductVersion()) {
		// Encode a number usable as a product firmware version in the group name (see generateProductVersion)
		group = `ci_prod_ver-${ver}`;
		resp = await api.listDevices({ product, groups: [group] });
		if (resp.body.devices.length === 1 && resp.body.devices[0].id === deviceId) {
			break;
		}
		if (resp.body.devices.length !== 0) {
			continue;
		}
		try {
			// Create a new group
			await api.post({ uri: `/v1/products/${product}/groups`, data: {
				name: group,
				product_id: product
			}});
		} catch (err) {
			const msg = err.body?.error;
			if (typeof msg === 'string' && msg.contains('already exists')) {
				continue;
			}
			throw err;
		}
		// Add the device to the group
		await api.put({ uri: api.deviceUri({ deviceId, product }), data: {
			groups: [group, ...extraGroups]
		}});
		resp = await api.listDevices({ product, groups: [group] });
		if (resp.body.devices.length !== 1 || resp.body.devices[0].id !== deviceId) {
			// Remove the device from the group
			await api.put({ uri: api.deviceUri({ deviceId, product }), data: {
				groups: extraGroups
			}});
			continue;
		}
		break;
	}
	console.log('Generated device group:', group);
	groupsByDeviceId.set(deviceId, group);
	return group;
}

/**
 * Generate a unique product firmware version for use with a device.
 *
 * @param {string} deviceId Device ID.
 * @param {Particle} api API client.
 * @returns {number} Firmware version.
 */
async function generateProductVersion({ deviceId, api }) {
	const group = await generateDeviceGroup({ deviceId, api });
	const m = group.match(/^ci_prod_ver-(\d+)$/);
	if (!m) {
		throw new Error(`Unexpected device group name: ${group}`);
	}
	const ver = Number(m[1]);
	console.log('Generated product version:', ver);
	return ver;
}

/**
 * Clear the cached product info.
 *
 * @param {string} deviceId Device ID.
 */
function clearProductCache({ deviceId }) {
	productsByDeviceId.delete(deviceId);
	groupsByDeviceId.delete(deviceId);
}

module.exports = {
	getProductId,
	generateDeviceGroup,
	generateProductVersion,
	clearProductCache
};
