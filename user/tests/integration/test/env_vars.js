const { createEnvVarsAssetModule } = require('binary-version-reader');
const tempy = require('tempy');

/**
 * Build an env vars asset that can be flashed to a device.
 *
 * Env vars are read at boot and cannot be set over the wire, so a test that needs them flashes
 * them as an asset.
 *
 * @param {object} vars Environment variables to set.
 * @param {string} name File name for the asset.
 * @returns {Promise<string>} Path to the asset on disk.
 */
async function writeEnvVarsAsset(vars, name) {
	const assetData = await createEnvVarsAssetModule(vars);
	return tempy.write(assetData, { name });
}

module.exports = {
	writeEnvVarsAsset
};
