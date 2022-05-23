suite('Cloud variables')

platform('gen3');

let api = null;
let auth = null;
let device = null;
let deviceId = null;
let limits = null;

before(function() {
	api = this.particle.apiClient.instance;
	auth = this.particle.apiClient.token;
	device = this.particle.devices[0];
	deviceId = device.id;
});

async function delayMs(ms) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

async function getDeviceVariablesWithRerties({ deviceId, auth, expectedVarNum = 0, retries = 10, delay = 1000 } = {}) {
	let lastError;
	for (let i = 0; i < retries; i++) {
		try {
			const resp = await api.getDevice({ deviceId, auth });
			const vars = resp.body.variables;
			if (expectedVarNum > 0 && Object.keys(vars).length !== expectedVarNum) {
				throw new Error('Number of variables returned from device does not match expected');
			}
			return resp;
		} catch (e) {
			lastError = e;
		}
		await delayMs(i * delay);
	}
	if (lastError) {
		throw lastError;
	}
	throw new Error('Error fetching variables from device');
}


async function getVariableWithRetries({ deviceId, name, auth, retries = 10, delay = 1000 } = {}) {
	let lastError;
	for (let i = 0; i < retries; i++) {
		try {
			const resp = await api.getVariable({ deviceId, name, auth });
			return resp;
		} catch (e) {
			lastError = e;
		}
		await delayMs(i * delay);
	}
	if (lastError) {
		throw lastError;
	}
	throw new Error('Error fetching variable from device');
}

test('01_register_variables', async function() {
	const expectedVars = {
		'var_b': 'bool',
		'var_i': 'int32',
		'var_d': 'double',
		'var_c': 'string',
		'var_s': 'string',
		'fn_b': 'bool',
		'fn_i8': 'int32',
		'fn_u8': 'int32',
		'fn_i16': 'int32',
		'fn_u16': 'int32',
		'fn_i32': 'int32',
		'fn_u32': 'int32',
		'fn_f': 'double',
		'fn_d': 'double',
		'fn_c': 'string',
		'fn_s': 'string',
		'std_fn_i': 'int32',
		'std_fn_d': 'double',
		'std_fn_c': 'string',
		'std_fn_s': 'string'
	};
	const resp = await getDeviceVariablesWithRerties({ deviceId, auth, expectedVarNum: Object.keys(expectedVars).length });
	const vars = resp.body.variables;
	expect(vars).to.include(expectedVars);
});

test('02_check_variable_values', async function() {
	const vals = {
		'var_b': true,
		'var_i': -2147483648,
		'var_d': 1.7976931348623157e+308,
		'var_c': 'LlGqWwi6MhknxMW5Cbr62qBzYjrdXYxJiPKsnNOJ9dZli4WPOu22sr3Xc9r7yoni8Mkmb1KXrCyQI5J1V4JmWbhbrd5wJEOgUrsn',
		'var_s': '1n4m9ISRdZf7VlMP4RxrYiq0iVTwPKe1tD3maxwOCu3WnYJmngZmQM1wSChiP6twUyqozIs73lA7XGZBwgbGjGifa2cvAJ1Dodyw',
		'fn_b': true,
		'fn_i8': -128,
		'fn_u8': 255,
		'fn_i16': -32768,
		'fn_u16': 65535,
		'fn_i32': -2147483648,
		'fn_u32': -1, // Unsigned 32-bit integers are not supported
		'fn_f': 3.4028234663852886e+38,
		'fn_d': 1.7976931348623157e+308,
		'fn_c': 'LlGqWwi6MhknxMW5Cbr62qBzYjrdXYxJiPKsnNOJ9dZli4WPOu22sr3Xc9r7yoni8Mkmb1KXrCyQI5J1V4JmWbhbrd5wJEOgUrsn',
		'fn_s': '1n4m9ISRdZf7VlMP4RxrYiq0iVTwPKe1tD3maxwOCu3WnYJmngZmQM1wSChiP6twUyqozIs73lA7XGZBwgbGjGifa2cvAJ1Dodyw',
		'std_fn_i': -2147483648,
		'std_fn_d': 1.7976931348623157e+308,
		'std_fn_c': 'LlGqWwi6MhknxMW5Cbr62qBzYjrdXYxJiPKsnNOJ9dZli4WPOu22sr3Xc9r7yoni8Mkmb1KXrCyQI5J1V4JmWbhbrd5wJEOgUrsn',
		'std_fn_s': '1n4m9ISRdZf7VlMP4RxrYiq0iVTwPKe1tD3maxwOCu3WnYJmngZmQM1wSChiP6twUyqozIs73lA7XGZBwgbGjGifa2cvAJ1Dodyw'
	};
	const vars = {};
	for (let name in vals) {
		const resp = await getVariableWithRetries({ deviceId, name, auth });
		vars[name] = resp.body.result;
	}
	expect(vars).to.include(vals);
});

test('03_publish_variable_limits', async function() {
	const data = await this.particle.receiveEvent('limits');
	limits = JSON.parse(data);
	expect(limits.max_num).to.equal(100);
	expect(limits.max_name_len).to.equal(64);
	expect(limits.max_val_len).to.be.above(794); // Maximum supported size of a variable value in pre-3.0.0 Device OS
	console.log('Particle.maxVariableValueSize() returned', limits.max_val_len);
});

test('04_verify_max_variable_value_size', async function() {
	// Original 1500-character string used in the application code
	const str = 'XvFclXWVOG6n99rUYpsLzrp8VyPWdpKfm4z4SdX2GwxLwoJOSPpHL5jF6ajMaJhJdWUuDPSfmqoDmb5DQZRWZFM2f6tSsqmDzVPojUr5qZJQKEgb8WPndRRnD6y9AA5RPfkoqNZKfTgmDCWSGDHygLaFvYOUsM6ggZD8pBLnyrfs5c1fMrM6qZsRglUfaEit4hrDKfsdHoD2SUmdckgU6vqmYHpeVEwW6xitwwFRtyHSvCUb4XbZIWBHJypHEHS17wUpDbTPHcaowsod9Ogp1UjD2ybAUaNd1ul0yPvPigNAqdBsOQ8viVEnOyADAnf0TPQjaXEQ5LWgLJNIheO2qmniPFL9WSnQFPZSY7lwjANoK07ys62nRGoAgwS1sNL0LOvweWwklUxhVDw7foEWBDSXoLaaHieQ7sUvcxAH05S0LMd4m3QbFbkxwFnZPjqvdS98dtAIcvAZqGwbHtnGIInWT5LArXrsyAmiGouezRbgMwS6IFn6ObkGyvEmGqyIGmTdhGlDUSMVMzRXXKoXDn36yqKimGwLhiBKEc4oq7TpwfQ8P17DjO3rVC8hA9cf0UFNHSIhrK4bHtOKSoXEIDv4O4p86xG9oJ84yuUxz4psJHolfwFFlZ6m5csmSOk6urU3kpxh9FyuBnwGrICGIfTxMNfOU0EiV1ajMudqz9G2L2IBgxsqjKaOmeGjja4tgg9cW1UMFnEK9QaXs88kdUmXiJRnIHuZlCg1rOUvQgxmoUlPR7lZ9R6ZfWOivmX2gs7kxiSxK84JmVirVjqgE1gHASoSXjUj3YhJ5h0c6yR5QN6QrHc4zPN2jnI1Tukt8mS7WXbRmGPz31dZSUC9LYVqifY9bw77QYiqenXFbtX4vEeOKFxCvXbzZv3QKKCReobPu0eTM0iLNcrVXUocZXjOnfU7e42UrV8HBGrkB0ozu0mgmVcDlW0M5wp8gcx4ekXLlmvfYH0WO3YamV1ioraHwXJ0MmRSjaFuau7CqOZyUPfhspnM7Yo8yz8J58oVs7oxTzdkgINbr0zBclRyNY6Box9p1MMOtR5t5oNiRYs7g8WxIN4KCKY5CWnlUNUByCwNHhnEGRIIi5guZNt22FHsBPtoztLDwJ7YUY26GTJUypdXm3QOho3vw4IP68w651rcJU7SWX9aEw7pkTS7FqHYT0vsyt2H2Jzx5QQsBcbVei2RL9lgnNRB2UvxNSOyiifjeIECvapmMLiTdTYgq2ZVBDjoTJyZ5DPRdCsJlpKzlNvoomiXnIPyVfhMWhGk5IKieNvkYtTqQZEVhwysndg3MkQLHqlSpU061PPrEoPUtJSvX4c5JtBnISKDT3sFpIHnUayITBUjzKUpJABiPr8E2zBJP1WFJd5yWEBf1JsRBFmrnP7qq6b6zNraRw1NrBvxva04kxIcW8wiTuOkrvlGwChxy5vG4AtVVDga2TSDotdzu5W2mLW3QI9r05zNY0MWVPwpZVmbGZjcYBqE2As7Gl67';
	const resp = await getVariableWithRetries({ deviceId, name: 'var_s', auth });
	expect(resp.body.result).to.equal(str.slice(0, limits.max_val_len));
});

test('05_empty_string_variable', async function() {
	const resp = await getVariableWithRetries({ deviceId, name: 'var_s', auth });
	expect(resp.body.result).to.equal('');
});

test('06_check_current_thread', async function() {
	// Reset the device before the next test
	await device.reset();
});

test('07_register_many_variables', async function() {
	const varCount = limits.max_num;
	const resp = await getDeviceVariablesWithRerties({ deviceId, auth, expectedVarNum: varCount });
	const vars = resp.body.variables;
	const expectedVars = {};
	for (let i = 1; i <= varCount; ++i) {
		let name = 'var_' + i.toString().padStart(3, '0') + '_';
		name += 'x'.repeat(Math.max(limits.max_name_len - name.length, 0));
		expectedVars[name] = 'int32';
	}
	expect(vars).to.deep.equal(expectedVars);
});
