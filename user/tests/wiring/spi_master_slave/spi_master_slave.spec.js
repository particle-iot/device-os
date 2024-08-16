suite('SPI MASTER SLAVE');

platform('p2','argon');
systemThread('enabled');
timeout(20 * 60 * 1000);

fixture('spi_master', 'spi_slave');

// This tag should be filtered out by default
tag('fixture');