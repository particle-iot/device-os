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

module.exports = {
	unsetDeviceVariables
};
