suite('Cloud events');

platform('gen3', 'gen4');

let deviceId;
let maxEventDataSize = 0;

function parseDataUri(str) {
	const m = /^data:(.+?)(;base64)?,(.*)$/.exec(str);
	if (!m) {
		throw new Error('Failed to parse data URI');
	}
	let data;
	const base64 = !!m[2];
	if (base64) {
		data = Buffer.from(m[3], 'base64');
	} else {
		data = decodeURI(m[3]);
	}
	return {
		type: m[1],
		data
	};
}

before(function() {
	deviceId = this.particle.devices[0].id;
});

test('connect', async function() {
	// See events.cpp
});

test('particle_publish_publishes_an_event', async function() {
	const data = await this.particle.receiveEvent('my_event');
	expect(data).to.equal('event data');
});

test('get_max_event_data_size', async function() {
	const data = await this.particle.receiveEvent('max_event_data_size');
	maxEventDataSize = Number.parseInt(data);
	expect(maxEventDataSize).to.be.above(622); // Maximum supported size of event data in pre-3.0.0 Device OS
	console.log('Particle.maxEventDataSize() returned', maxEventDataSize);
});

test('verify_max_event_data_size', async function() {
	// Original 1500-character string used in the application code
	const str = 'eI568Df9nXQmUyaDeNE7A4pZnrcdaAxetam6QYQe3lXFwzN3A6ZO2VGutxVBbIWc8EyrqFMtzKByspno2vL1bGB9H6btc5GWysJZ3XLa3paAmAG4P3UZcbg4NuSRTSEr2YsDMTIEF2lSdd51YR0BPsbcEiQN29ufOpfEHXqK7LfJ3lfEMySnl0iX3ajaQ9rlLsKF4vhSoLFQDp3SRAmzfHhLCDHqVFDT9o8I4Ac5ER6cPl5k8wucWJqxQVWCHB2jdrtSX3WNX8Uq14mAuS4L4s2SeP6UlCcWXrzV9AAuBeTON9Jw7Lbe09F7Ijz0KxIPlwnVZDqXV09GbxKXIOA41E1ZeR9Cg23vozKZZzn2cWeeYtJmRi5Evmwmjus72XQM1W7KGZABrQbzSZawK0pRk9Cp7kl2uy39IjxL6ev3nlC8EA2DE7zi1DJHW7bJceUvFevQcHjWHU5FNKx7m48SG2046PDxxl0vnkXQ6hompl04RFmjUnIgEfIT9XZCkes5lPa8T2V8Ueo7aDfPBYSZOX35XBCczj6nXZ9oxVqn9zxH5NrLcmeDsLop77PVmdJles0CWEAAr5zNVOxIETN2jJcksLXRfQ1pESo9YLaBTyjSuDRQqMenYwuv2qFFnEbaZCMqBQRvE4ql0Oo6K9rXKdfO5G8b9c9jSI4g56f1DAiv7iWU99NdMUMVFt2LmYZsT0azi6MztjRsbtVRG2thZUqAhaPuhvZd0Efbd5H01oUN2CIsh9NiMdEkG5ouSMVaLGjIuvfDeFnlKjL7wSvmNauWYQY021dCKfpJCx0Q7XRB9kFDWZLcew61CmCHsEctM4JldvVhKLdWcnKFDttz3CfbFgtkGBVPWSW0hOwA2e5SLNwHyyJyJXNsicFxMpelYlVAhFjSR8nXe0cJqylvmKYUQ85H2Qet4kehs4boQLIqTHeDoDy1ITDbNVnv3PWzbna5kmEiBhyRw4kn6Di1a6r7uamd5fgFAGURi9LYCp3wAuw6PbYpq8rFXFFzkOUniI3q5c1bLDFxRS4zxNOuH31W819DZGM57zimuZ8YeEfAljxmSOeUWQQdlJjZbjgvERF1Dlexe4nROXyDOadc4qlznOKL0u2ttG0hCVPHMXG4s4uP8YLXJMhyNZod6mkdW9R42aWAsJgDMZZnuU7J7HJL9OpOZXPDCl1l2wOlPCyUtVQzG7PD1Db0dIaTMe9YnFtNAPPxAD4JQXNKMkmWRrhVE2VuJlNvokoCZp9pBDYBFJEPHOYWZI93gsR2tdSIa7YQslZRykJRAF90xlBfNvljN9yR64g7Q1IKCbGwr59H2I5WFEHruiIFJpPs9QQOYxlgq9juAJ9GyfmpEwuvF6n49Bi34v9dQGwt5ZMRFB6HgoRTb9PaLCp4e0Ns7zYYY2rWwESeZnPsqADsFFG3pxsisIn8pjLdAlJrAdMiyUGaIvi7Vj6uFmClZMI8i39pnWXfJbUSJtofdeCthZD2awxZJMjC';
	const data = await this.particle.receiveEvent('my_event');
	expect(data).to.equal(str.slice(0, maxEventDataSize));
});

test('device_to_cloud_event_with_content_type', async function() {
	const expectedData = Buffer.from([0x92, 0xb2, 0xd1, 0x00, 0x23, 0x8c, 0x49, 0x0b, 0xd6, 0xe7, 0xa8, 0x90, 0x50, 0xdb, 0xc5, 0xbb, 0xe5, 0x3f, 0xf3, 0x29, 0xb2, 0x4c, 0xef, 0xad, 0xad, 0xd8, 0x10, 0xd9, 0xc3, 0xa4, 0xf1, 0xb0, 0xe7, 0x74, 0x8e, 0x7e, 0x2e, 0xcf, 0x48, 0xbe, 0x4d, 0xd3, 0xae, 0x08, 0x36, 0x8f, 0x76, 0xa8, 0xd5, 0x50, 0xec, 0x13, 0x9d, 0x5b, 0xca, 0x62, 0x4e, 0x3c, 0x6b, 0x3c, 0xbc, 0x75, 0x85, 0x65, 0x35, 0x6c, 0x00, 0xaf, 0xee, 0x12, 0xf2, 0xbd, 0x3f, 0xf2, 0x27, 0x00, 0xdc, 0x4c, 0xc6, 0xfa, 0x02, 0x16, 0x8e, 0xe5, 0xa1, 0xe6, 0xe9, 0x40, 0x4a, 0x71, 0xd1, 0x7d, 0xa5, 0xa6, 0xb7, 0x8d, 0x7d, 0x47, 0x5f, 0xf2]);
	let d = await this.particle.receiveEvent('my_event');
	d = parseDataUri(d);
	expect(d.type).to.equal('application/octet-stream');
	expect(d.data.equals(expectedData)).to.be.true;
});

test('publish_cloud_to_device_event_with_content_type', async function() {
	const data = Buffer.from([0xcb, 0xdf, 0x43, 0x83, 0x00, 0x86, 0x31, 0x72, 0x8b, 0xaf, 0x35, 0xc2, 0xaa, 0xae, 0x5d, 0x2e, 0x77, 0x76, 0x91, 0xb1, 0x31, 0xa5, 0xf1, 0x05, 0x1d, 0x7f, 0xc1, 0x47, 0xa8, 0x1f, 0x2a, 0x90, 0xe5, 0x75, 0x30, 0x9d, 0xdc, 0x28, 0x90, 0x68, 0x8b, 0xb8, 0x6e, 0x6e, 0x85, 0x14, 0x0d, 0x95, 0xc0, 0x64, 0xfd, 0xf3, 0xce, 0x3d, 0xfb, 0x45, 0xa3, 0xa7, 0xfe, 0x3d, 0xcf, 0x94, 0xd8, 0x69, 0xcb, 0x21, 0x39, 0x2c, 0x9a, 0xc9, 0xbb, 0x5b, 0xcb, 0x2d, 0xd9, 0x43, 0xb5, 0xbe, 0x21, 0xdd, 0x3b, 0xe1, 0x8c, 0x87, 0x64, 0x68, 0x00, 0x4d, 0x98, 0x1e, 0x6a, 0xe0, 0x2a, 0x42, 0xe8, 0x05, 0xb9, 0x89, 0xef, 0xcd]);
	await this.particle.apiClient.instance.publishEvent({
		name: `${deviceId}/my_event`,
		data: `data:application/octet-stream;base64,${data.toString('base64')}`,
		auth: this.particle.apiClient.token
	});
});

test('validate_cloud_to_device_event_with_content_type', async function() {
	// See events.cpp
});
