suite('Cloud events (new API)');

platform('gen3', 'gen4');

before(function() {
	deviceId = this.particle.devices[0].id;
});
