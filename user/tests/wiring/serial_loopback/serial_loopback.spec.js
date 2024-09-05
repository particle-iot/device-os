suite('Serial loopback');
platform('gen3', 'p2');
fixture('serial_loopback');
systemThread('enabled');
// This tag should be filtered out by default
tag('fixture');
