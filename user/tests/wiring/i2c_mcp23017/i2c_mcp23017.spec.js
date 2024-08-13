suite('I2C MCP23017');

platform('gen3');
systemThread('enabled');

fixture('i2c_mcp23017');

// This tag should be filtered out by default
tag('fixture');