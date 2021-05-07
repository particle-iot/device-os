// The assertions here and other .cpp file help to validate the following:
// Devices connect to cloud quickly
//   Given good connectivity or better
//   And LTS based firmware
//   99% of devices will be breathing cyan within 60 seconds of a cold boot (exception of 90s cat-1 modems, i.e. b5som + tracker)
suite('Network/cloud connection time SLOs');

platform('gen2', 'gen3');

const MAX_VALUES = {
	// boron: {
	// 	network_cold_connect_time_from_startup: 90000,
	// 	network_warm_connect_time_from_startup: 30000,
	// 	cloud_full_handshake_duration: 15000,
	// 	cloud_session_resume_duration: 5000
	// },
	default: {
		network_cold_connect_time_from_startup: 90000,
		network_warm_connect_time_from_startup: 30000,
		cloud_full_handshake_duration: 15000,
		cloud_session_resume_duration: 5000
	}
};

test('network_connect_cold', async function() {
	// Reset the device before running the test for the warm boot scenario
	const dev = this.particle.devices[0];
	await dev.reset();
});

test('network_connect_warm', async function() {
});

test('cloud_connect_full_handshake', async function() {
});

test('cloud_connect_session_resume', async function() {
});

test('publish_and_validate_stats', async function() {
	let stats = await this.particle.receiveEvent('stats');
	stats = JSON.parse(stats);
	const platform = this.particle.devices[0].platform.name;
	const maxValues = (platform in MAX_VALUES) ? MAX_VALUES[platform] : MAX_VALUES['default'];
	expect(stats.network_cold_connect_time_from_startup).to.be.lessThanOrEqual(maxValues.network_cold_connect_time_from_startup);
	expect(stats.network_warm_connect_time_from_startup).to.be.lessThanOrEqual(maxValues.network_warm_connect_time_from_startup);
	expect(stats.cloud_full_handshake_duration).to.be.lessThanOrEqual(maxValues.cloud_full_handshake_duration);
	expect(stats.cloud_session_resume_duration).to.be.lessThanOrEqual(maxValues.cloud_session_resume_duration);
});
