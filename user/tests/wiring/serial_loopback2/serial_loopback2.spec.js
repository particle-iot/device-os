suite('Serial loopback2');
platform('gen3', 'gen4');
fixture('serial_loopback2');
systemThread('enabled');
// This tag should be filtered out by default
tag('fixture');
