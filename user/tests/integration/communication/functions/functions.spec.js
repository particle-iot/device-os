suite('Cloud functions')

platform('gen2', 'gen3');

let api = null;
let auth = null;
let device = null;
let deviceId = null;

before(function() {
	api = this.particle.apiClient.instance;
	auth = this.particle.apiClient.token;
	device = this.particle.devices[0];
	deviceId = device.id;
});

test('register_functions', async function() {
	const resp = await api.getDevice({ deviceId, auth });
	const funcs = resp.body.functions;
	expect(funcs).to.include.members(['fn_1', 'fn_2']);
});

test('call_function_and_check_return_value', async function() {
	// Call fn_1
	let resp = await api.callFunction({ deviceId, name: 'fn_1', argument: 'argument string', auth });
	expect(resp.body.return_value).to.equal(1234);
	// Call fn_2
	// 1024-character string
	const arg = '1eD5TLw9CLc7CNolLSIbhpTYuc8V3cDGXeEVGSjAEPCQqzzhBNO97UPuF5OInUHP4wbySaLRCAw682m8AkK3VqEPXTVP2aMrsUHJYZB9B1Euz2ZIA6xFU0TlRwq1MAXpn7njmyzlJLNJioYcNQ4B6Id1GhDFX0DZOueDRf9KCLyMa5JXXlOlcYsO7yrJP7RuHgpDztbJz8UHEnW5uxsRjt8qlt4789b4JieWHqRSaf8LwczHiEBGeZXnTnsP09qr5WGXnvpdo0zoDxUH4rg07TbLph6luGj4pDR0Tl1lEKRurP0suvq7lZhbLniI6fmbTMV58lAc6oWs3N6inIdOyIMrwOhfIPRa7714pReW1yZCy1lGh7E3HMChMx27tlwxCoa5qcyXuq3GxKwMJvrAgTd0ZXF05JRK5CWwapr4rrs1hNrkniM6q7x7k00tS5E9umjiaQWPwgfu2HBdrtId4Y7wESeS61Z8xM15oBFZ6LxD1EiBqRvnY314q5yy6LHEgLBGmq8MDzKPLemoFNy5ysWATf5XZIx6BfooajFW4WN1bNFF66PznhN2Cy5w9HDzO1lGhshYx3gbzoynW4yXSN1MBqtSfORRgnDdwpe8ADozXXHbvwS0ZMfPN27DreZD24cw9K7AarXCToKom4fbZAQzz8NgWiNGerJZ5vkc6Qsk1qrC9tQ6t1r79wmm4z9bIgy1IiEnlX7zPUBqegJRpQETIIY7oaWcwtf1FBPtiGIbrIcBbneV0rjsCdx7h343uoiKubciQz3ewqNEFJsIvg7caVsamIVCgBVClxax6dOut6fI0n29JdOrpVtYtxMtaY7fGPxwdjTRpACVRg3FAhN8JDzetsoOlEuuUQCSdTFMnqvVGhauQpbQPupn8N2cVkeQYbc1nT1cnY0JZq1PvLZXWyXmoONFK1rxwsBj2W54NoQ0L9pqe3DSV3PZNSnprJInoIwsp9br37QzihSA7doUDAbnAkVxqoJF3Ewmyb46tuzpf8D8JB1fS2idltIG';
	const maxLen = device.platform.is('gen2') ? 864 : arg.length; // See MAX_FUNCTION_ARG_LENGTH in protocol_defs.h
	resp = await api.callFunction({ deviceId, name: 'fn_2', argument: arg.slice(0, maxLen), auth });
	expect(resp.body.return_value).to.equal(-1234);
});

test('check_function_argument_value', async function() { // See functions.cpp
});

test('check_current_thread', async function() { // See functions.cpp
});
