// The assertions here and other .cpp file help to validate the following:
// Devices connect to cloud quickly
//   Given good connectivity or better
//   And LTS based firmware
//   99% of devices will be breathing cyan
//     within 60 seconds of a cold boot
//     within 30 seconds of a warm boot
suite('Network/cloud connection time SLOs');

platform('gen2', 'gen3');
systemThread('enabled');

// Parameters validated by this test
const THRESHOLDS = {
	// boron: {
	// 	maxCloudConnectTimeFromColdBoot: 60000,
	// 	maxCloudConnectTimeFromWarmBoot: 30000
	// },
	default: {
		maxCloudConnectTimeFromColdBoot: 60000,
		maxCloudConnectTimeFromWarmBoot: 30000
	}
};

// Number of connection time measurements to make. When changing this parameter, make sure to
// update the test application accordingly
const CONNECT_COUNT = 10;

// The test uses the Nth percentile of all connection time measurements
const PERCENTILE = 75;

let device = null;

function percentile(values, p) {
	if (!values.length) {
		return NaN;
	}
	values = values.slice().sort();
	const i = Math.floor((values.length - 1) * p / 100);
	return values[i];
}

before(function() {
	device = this.particle.devices[0];
});

// TODO: The test runner doesn't support resetting the device in a loop from within a test
for (let i = 1; i <= CONNECT_COUNT; ++i) {
	test(`cloud_connect_time_from_cold_boot_${i.toString().padStart(2, '0')}`, async () => {
		await device.reset(); // Reset the device before running the next test
	});
}

for (let i = 1; i <= CONNECT_COUNT; ++i) {
	test(`cloud_connect_time_from_warm_boot_${i.toString().padStart(2, '0')}`, async () => {
		await device.reset();
	});
}

test('publish_and_validate_stats', async function() {
	let stats = await this.particle.receiveEvent('stats');
	this.particle.log.verbose('stats:', stats);
	stats = JSON.parse(stats);
	expect(stats.cloud_connect_time_from_cold_boot).to.have.lengthOf(CONNECT_COUNT);
	expect(stats.cloud_connect_time_from_warm_boot).to.have.lengthOf(CONNECT_COUNT);
	const thresh = Object.assign({}, THRESHOLDS['default'], THRESHOLDS[device.platform.name]);
	// Cold boot
	let t = percentile(stats.cloud_connect_time_from_cold_boot, PERCENTILE);
	this.particle.log.verbose('cloud_connect_time_from_cold_boot:', t);
	expect(t).to.be.lessThanOrEqual(thresh.maxCloudConnectTimeFromColdBoot);
	// Warm boot
	t = percentile(stats.cloud_connect_time_from_warm_boot, PERCENTILE);
	this.particle.log.verbose('cloud_connect_time_from_warm_boot:', t);
	expect(t).to.be.lessThanOrEqual(thresh.maxCloudConnectTimeFromWarmBoot);
});
