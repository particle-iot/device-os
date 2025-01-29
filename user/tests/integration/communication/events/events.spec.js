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

test('publish_device_to_cloud_event_with_content_type', async function() {
	const expectedData = Buffer.from('92b2d100238c490bd6e7a89050dbc5bbe53ff329b24cefadadd810d9c3a4f1b0e7748e7e2ecf48be4dd3ae08368f76a8d550ec139d5bca624e3c6b3cbc758565356c00afee12f2bd3ff22700dc4cc6fa02168ee5a1e6e9404a71d17da5a6b78d7d475ff2', 'hex');
	let d = await this.particle.receiveEvent('my_event');
	d = parseDataUri(d);
	expect(d.type).to.equal('application/octet-stream');
	expect(d.data.equals(expectedData)).to.be.true;
});

test('publish_device_to_cloud_event_as_variant', async function() {
	let d = await this.particle.receiveEvent('my_event');
	d = JSON.parse(d);
	expect(d).to.deep.equal({
		a: 123,
		b: {
			_type: 'buffer',
			_data: Buffer.from('6e7200463f1bb774472f', 'hex').toString('base64')
		}
	});
});

test('publish_cloud_to_device_event_with_content_type', async function() {
	const data = Buffer.from('cbdf4383008631728baf35c2aaae5d2e777691b131a5f1051d7fc147a81f2a90e575309ddc2890688bb86e6e85140d95c064fdf3ce3dfb45a3a7fe3dcf94d869cb21392c9ac9bb5bcb2dd943b5be21dd3be18c876468004d981e6ae02a42e805b989efcd', 'hex');
	await this.particle.publishEvent({
		name: `${deviceId}/my_event1`,
		data: `data:application/octet-stream;base64,${data.toString('base64')}`,
		auth: this.particle.apiClient.token
	});
});

test('validate_cloud_to_device_event_with_content_type', async function() {
	// See events.cpp
});

test('publish_cloud_to_device_event_with_cbor_data', async function() {
	const data = Buffer.from('a261631901c861644a921bff008d91814e789b', 'hex');
	await this.particle.publishEvent({
		name: `${deviceId}/my_event2`,
		data: `data:application/cbor;base64,${data.toString('base64')}`,
		auth: this.particle.apiClient.token
	});
});

test('validate_cloud_to_device_event_with_cbor_data', async function() {
	// See events.cpp
});
