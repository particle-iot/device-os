suite('I2C MASTER SLAVE');

platform('gen3', 'p2');
systemThread('enabled');

fixture('i2c_slave', 'i2c_master');

// This tag should be filtered out by default
tag('fixture');