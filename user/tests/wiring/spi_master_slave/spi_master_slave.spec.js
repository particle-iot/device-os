suite('SPI MASTER SLAVE');

platform('gen3');
systemThread('enabled');
timeout(20 * 60 * 1000);

fixture('spi_master', 'spi_slave');

// This tag should be filtered out by default
tag('fixture');