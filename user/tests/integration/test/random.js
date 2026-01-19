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

module.exports = {
	randomInt
};
