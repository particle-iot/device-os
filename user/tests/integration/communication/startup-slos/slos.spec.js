suite('Cloud events');

platform('gen2', 'gen3');

test('Particle.publish() publishes an event', async function() {
	const value = await this.particle.receiveEvent('startup_stats');
	expect(value).to.equal('event data');
});
