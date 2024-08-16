suite('I2C MASTER SLAVE');

platform('p2','argon');
systemThread('enabled');

fixture('i2c_slave', 'i2c_master');

// This tag should be filtered out by default
tag('fixture');