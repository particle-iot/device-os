/**
 * Send a `GET` request to the API.
 *
 * @param {Particle} api API client.
 * @param {string} URI path suffix.
 * @returns {Promise<object>} Response body.
 */
async function get(api, path) {
	const { body } = await api.get({ uri: path });
	return body;
}

/**
 * Send a `POST` request to the API.
 *
 * @param {Particle} api API client.
 * @param {string} URI path suffix.
 * @param {object} Request body.
 * @returns {Promise<object>} Response body.
 */
async function post(api, path, data) {
	const { body } = await api.post({ uri: path, data });
	return body;
}

/**
 * Send a `PATCH` request to the API.
 *
 * @param {Particle} api API client.
 * @param {string} URI path suffix.
 * @param {object} Request body.
 * @returns {Promise<object>} Response body.
 */
async function patch(api, path, data) {
	// TODO: patch() is not exposed by particle-api-js
	const { body } = await api.agent.request({
		method: 'patch',
		uri: path,
		auth: api._defaultAuth,
		data
	});
	return body;
}

/**
 * Get an URI path for a product API endpoint.
 *
 * @param {string} path Path suffix.
 * @param {string|number} product Product ID or slug.
 * @param {string} [device] Device ID or name.
 * @returns {string}
 */
function productPath(path, product, device) {
	let prefix = `/v1/products/${product}`;
	if (device !== undefined) {
		prefix += `/devices/${device}`;
	}
	return prefix + path;
}

/**
 * Get an URI path for an org or user sandbox API endpoint.
 *
 * @param {string} path Path suffix.
 * @param {string} [org] Org ID or slug.
 * @param {string} [device] Device ID or name.
 * @returns {string}
 */
function orgPath(path, org, device) {
	let prefix = '/v1';
	if (org !== undefined) {
		prefix += `/orgs/${org}`;
	}
	if (device !== undefined) {
		prefix += `/devices/${device}`;
	}
	return prefix + path;
}

module.exports = {
	get,
	post,
	patch,
	productPath,
	orgPath
};
