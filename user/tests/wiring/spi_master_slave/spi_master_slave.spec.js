suite('SPI MASTER SLAVE');

platform('p2','argon');
systemThread('enabled');
timeout(20 * 60 * 1000);

// FIXME: currently no way to define multiple fixture configurations within a test suite
// fixture('spi_master', 'spi1_slave'); // use with spi_master_slave_1_bw.config.js and spi_master_slave_2_bw.config.js
fixture('spi1_master', 'spi1_slave'); // use with spi_master_slave_3_bw.config.js

// This tag should be filtered out by default
tag('fixture');