suite('Cloud variables')

platform('gen2', 'gen3');
systemThread('enabled');

let api = null;
let auth = null;
let deviceId = null;

before(function() {
	api = this.particle.apiClient.instance;
	auth = this.particle.apiClient.token;
	deviceId = this.particle.devices[0].id;
});

test('register_variables', async function() {
	const resp = await api.getDevice({ deviceId, auth });
	const vars = resp.body.variables;
	expect(vars).to.include({
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
	});
});

test('check_variable_values', async function() {
	const vals = {
		'var_b': true,
		'var_i': -2147483648,
		'var_d': 1.7976931348623157e+308,
		'var_c': 'LlGqWwi6MhknxMW5Cbr62qBzYjrdXYxJiPKsnNOJ9dZli4WPOu22sr3Xc9r7yoni8Mkmb1KXrCyQI5J1V4JmWbhbrd5wJEOgUrsn',
		//'var_s': '1n4m9ISRdZf7VlMP4RxrYiq0iVTwPKe1tD3maxwOCu3WnYJmngZmQM1wSChiP6twUyqozIs73lA7XGZBwgbGjGifa2cvAJ1Dodyw',
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
		//'fn_s': '1n4m9ISRdZf7VlMP4RxrYiq0iVTwPKe1tD3maxwOCu3WnYJmngZmQM1wSChiP6twUyqozIs73lA7XGZBwgbGjGifa2cvAJ1Dodyw',
		'std_fn_i': -2147483648,
		'std_fn_d': 1.7976931348623157e+308,
		'std_fn_c': 'LlGqWwi6MhknxMW5Cbr62qBzYjrdXYxJiPKsnNOJ9dZli4WPOu22sr3Xc9r7yoni8Mkmb1KXrCyQI5J1V4JmWbhbrd5wJEOgUrsn',
		'std_fn_s': '1n4m9ISRdZf7VlMP4RxrYiq0iVTwPKe1tD3maxwOCu3WnYJmngZmQM1wSChiP6twUyqozIs73lA7XGZBwgbGjGifa2cvAJ1Dodyw'
	};
	const vars = {};
	for (let name in vals) {
		const resp = await api.getVariable({ deviceId, name, auth });
		vars[name] = resp.body.result;
	}
	expect(vars).to.include(vals);
});

test('check_current_thread', async function() {
});
