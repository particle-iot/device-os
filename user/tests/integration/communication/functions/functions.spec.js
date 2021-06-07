suite('Cloud functions')

platform('gen2', 'gen3');

let api = null;
let auth = null;
let device = null;
let deviceId = null;
let maxFunctionArgumentSize = 0;

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

test('get_max_function_argument_size', async function() {
	const data = await this.particle.receiveEvent('max_function_argument_size');
	maxFunctionArgumentSize = Number.parseInt(data);
	expect(maxFunctionArgumentSize).to.be.above(622); // Maximum supported size of a function argument in pre-3.0.0 Device OS
	console.log('Particle.maxFunctionArgumentSize() returned', maxFunctionArgumentSize);
});

test('call_function_and_check_return_value', async function() {
	// Call fn_1
	let resp = await api.callFunction({ deviceId, name: 'fn_1', argument: 'argument string', auth });
	expect(resp.body.return_value).to.equal(1234);
	// Call fn_2
	// 1500-character string
	const arg = 'WlbOWabfZl6J5H5vrB6KDnJhsI18avqx4RZHSJyDjQGOahaTyT52rHrE1cfUMLxwxZPFxjmDYeQuRUZKLEdwBynABXmQpRkJ09hdBbYZesdUiMqhCS5CcqXApZfidk5w8zb9LoWSPbI45vLod8cjSPsSykOhj64VUzH9FtamoU0a4Mq9pXr3Sz51kwNFpFpBG6a0dfWEzVBL5iYJn680MbUSK6RsMPUTSLrQLUbTlCt5loI4DKmppmg9yc4wsagDvd0AbU86dGeQTwLWHPL1i8k7EpEhjs3Ynf5mvcVwRv5Ik8HAQrtehSrAsWnvez1xqbMa7VxuLsgrBCeoTkiAyeIvXUUXy4DuwyTuM1VQ9kx9uksOLrQDPQEFfhchMe27SnWFwmPOD8vHQPR6P31YRgdnhL1dA91icnULzWD5qh4B9HyTshF0x4bRLRRQwgqJMkURXXqeYhn1vO1OxuSMl3yc0uAGiqBNrgD8DKVHsmnPMoQWN1igBYTvr6EGnuOFgmqhOVRPj9LJ4qfQFFJ7EkKPXSpddca2LoeWcxWNxBBMqISoeWV4GfynNQcLtPvPchj4mf9J1at4j0jiQsSF65Sw5Cy18RbYqazcl1pkchOl3YBk0jkJEF19KCniHD64uPeSeQT8HRtczK1bYCWjeQzZMyKMoRx8YdoUlt55gJfS8MSvDgvsdpMb39WWlCCnYJzQMOCFXmWlOEhWnABcML5ozmIWlB6LaDNcP3kC1Ou87VyslM5IbLSIBw8GNHBetNDnVsFhOntGldkVAapcIDakhunQHXcWB8QbECpy9ijZvSSDtMSGsYu7xFNznSR5QwXJQToITOsvpSffblarWmaXlCWnwfqTKxzAife4U7EmFdOrHQwq4RIHDgOUTtogzgkia9O3e4g1e7qEuweGEJm6YDZqXeGUdcxXvHmQxFmvKoGijur3jEGKVkv2SSanVQ5weOklb4se91mEcA6nUtQXZePx7T6aSf9UumuUeoAToIGTGweMViIxjI2wV5Eikqazfn7AEiXWbRLzKfC7XAZ6ZzqzWqFME4bPgxIbI2aHXe50sa3iA7NEkXZVWfLs8bCjNF66DyfZHahCMjevIpzclwUnlT2Q1Bg1LMyHXD4d6frplmlTvYAKAwVyGz10kZ38oxPtQoYGVF1wkNwXT8cBG1yT6vU6NUpkB22oHdXHzjL6fJ0jvPhIm2a4Sh5pO0Gg7NCktVRBj9jGqn3qo6uggAPTiqBPAoJM2yNJ3aQ5tJLdrN3EM3Brkdm27OaWvbbDQrFneBDjQy8aijwxjtYPz8jXxjGG9c3AW5hPTHxzDKuu64XvTjvbFLr8048on0V7RvvNg7PNmX4mRJr34PKYuEkoJ1KxRvttzziVVlQbvAiy72Inw6DqON0JBpDhg6RhHxXseqZkyrQtFJsOWB1A81cPu5eTzdD9izYpjo7kxpe9iUrb5AL36fhc5WJOBJIFwYEpsD7zh3GycAKc3yWb9BBv';
	resp = await api.callFunction({ deviceId, name: 'fn_2', argument: arg.slice(0, maxFunctionArgumentSize), auth });
	expect(resp.body.return_value).to.equal(-1234);
});

test('check_function_argument_value', async function() { // See functions.cpp
});

test('check_current_thread', async function() { // See functions.cpp
});
