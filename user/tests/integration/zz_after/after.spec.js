suite('Cleanup for HIL Testing');

platform('gen3', 'gen4');
systemThread('enabled');

before(function() {
    // console.log('before js runs');
});

test('01_erase_factory_module', async function () {

});

test('02_remove_static_ip', async function () {

});

test('03_enable_listening_mode', async function () {

});

test('04_cleanup_env', async function () {

});

after(function() {
    // console.log('after js runs');
});