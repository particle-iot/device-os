const { get, post, patch } = require('./api');
const { getProductId } = require('./product');
const _ = require('lodash');

async function unsetDeviceVariables(api, deviceId) {
    const productId = await getProductId({ deviceId, api });

    // Unset all device variables
	resp = await get(api, `/v1/products/${productId}/env/${deviceId}`);
	const ownDeviceVars = resp.last_snapshot?.own || {};
	if (!_.isEmpty(ownDeviceVars)) {
		await patch(api, `/v1/products/${productId}/env/${deviceId}`, {
			ops: Object.entries(ownDeviceVars).map(([key]) => ({ op: 'Unset', key }))
		});
		await post(api, `/v1/env/${deviceId}/rollout`, {
			when: 'Connect'
		});
	}
}

async function unsetProductVariables(api, deviceId) {
	const productId = await getProductId({ deviceId, api });

	// Unset all product variables. Leftovers here get synced down on the next connect and reboot
	// the device, which reads as an unexpected reset in whatever test is running at the time
	const resp = await get(api, `/v1/products/${productId}/env`);
	const ownProductVars = resp.last_snapshot?.own || {};
	if (!_.isEmpty(ownProductVars)) {
		await patch(api, `/v1/products/${productId}/env`, {
			ops: Object.entries(ownProductVars).map(([key]) => ({ op: 'Unset', key }))
		});
		await post(api, `/v1/products/${productId}/env/rollout`, {
			when: 'Connect'
		});
	}
}

module.exports = {
	unsetDeviceVariables,
	unsetProductVariables
};
