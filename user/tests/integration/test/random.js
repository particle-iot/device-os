/**
 * Generate a random integer in the range [`min`, `max`].
 *
 * @param {number} min
 * @param {number} max
 * @returns {number}
 */
function randomInt(min, max) {
	return Math.floor(Math.random() * (max - min + 1)) + min;
}

/**
 * Generate a random string with a length in the range [`minLen`, `maxLen`].
 *
 * @param {number} minLen
 * @param {number} [maxLen=minLen]
 * @returns {string}
 */
function randomString(minLen, maxLen = minLen) {
	const len = randomInt(minLen, maxLen);
	let str = '';
	for (let i = 0; i < len; ++i) {
		str += String.fromCodePoint(randomInt(0x61 /* a */, 0x7a /* z */));
	}
	return str;
}

module.exports = {
	randomInt,
	randomString
};
