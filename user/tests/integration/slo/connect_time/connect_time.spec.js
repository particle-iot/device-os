// The assertions here and other .cpp file help to validate the following:
// Devices connect to cloud quickly
//   Given good connectivity or better
//   and LTS based firmware
//   75th percentile latency to breathing cyan
//   is less than 60 seconds when starting from a cold boot
//   and less than 30 seconds when starting from a warm boot
suite('Network/cloud connection time SLOs');

platform('gen3');
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

// IMPORTANT: Make sure CONNECT_COUNT * PERCENTILE / 100 is a whole number
// -----------------------------------------------------------------------
//
// Number of connection time measurements to make. When changing this parameter, make sure to
// update the test application accordingly
const CONNECT_COUNT = 8;
// The test uses the Nth percentile of all connection time measurements
const PERCENTILE = 75;

let device = null;

function percentile(values, p) {
	if (!values.length) {
		return NaN;
	}
	values = values.slice().sort(function(a, b) {
		return a - b; // default .sort() is alphanumeric and will not sort integers properly
	});
	const i = Math.floor((values.length) * p / 100) - 1;
	return values[i];
}

async function fetchLog() {
	return device.mailBox;
}

before(function() {
	device = this.particle.devices[0];
});

// TODO: The test runner doesn't support resetting the device in a loop from within a test
// FIXME: it does now, the tests need to be fixed
for (let i = 1; i <= CONNECT_COUNT; ++i) {
	test(`cloud_connect_time_from_cold_boot_${i.toString().padStart(2, '0')}`, async () => {
		// dump any failure logs from this test
		let testLog = await fetchLog();
		if (testLog.length) {
			for (let msg of testLog) {
				console.log(msg.d);
			}
		}
		await device.reset(); // Reset the device before running the next test
	});
}

for (let i = 1; i <= CONNECT_COUNT; ++i) {
	test(`cloud_connect_time_from_warm_boot_${i.toString().padStart(2, '0')}`, async () => {
		// dump any failure logs from this test
		let testLog = await fetchLog();
		if (testLog.length) {
			for (let msg of testLog) {
				console.log(msg.d);
			}
		}
		await device.reset();
	});
}

test('publish_and_validate_stats', async function() {
	let stats = await fetchLog();
	stats = JSON.parse(stats[0].d);
	console.log('stats:');
	console.dir(stats, { depth: null, colors: true, compact: true, maxArrayLength: 100 });
	// Cold boot
	expect(stats.cold.cloud_connect).to.have.lengthOf(CONNECT_COUNT);
	const timeFromColdBoot = percentile(stats.cold.cloud_connect, PERCENTILE);
	console.log('cloud_connect_time_from_cold_boot:', timeFromColdBoot);
	// Warm boot
	expect(stats.warm.cloud_connect).to.have.lengthOf(CONNECT_COUNT);
	const timeFromWarmBoot = percentile(stats.warm.cloud_connect, PERCENTILE);
	console.log('cloud_connect_time_from_warm_boot:', timeFromWarmBoot);
	if (stats.exclude_from_slo_validation) {
		console.log('Skipping SLO validation');
		return;
	}
	const thresh = Object.assign({}, THRESHOLDS['default'], THRESHOLDS[device.platform.name]);
	expect(timeFromColdBoot).to.be.lessThan(thresh.maxCloudConnectTimeFromColdBoot);
	expect(timeFromWarmBoot).to.be.lessThan(thresh.maxCloudConnectTimeFromWarmBoot);
});
