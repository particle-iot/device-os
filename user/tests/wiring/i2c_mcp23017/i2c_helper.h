#ifndef I2C_REG_H_
#define I2C_REG_H_

// Some helper functions

namespace i2c {
void reset();

int32_t writeRegister(uint8_t slaveAddress, uint8_t addr, uint8_t val, bool stop);
int32_t writeRegister(uint8_t slaveAddress, uint8_t addr, uint8_t* buffer, uint8_t len, bool stop);

int32_t readRegister(uint8_t slaveAddress, uint8_t addr, bool stop, bool stopAfterWrite);
int32_t readRegister(uint8_t slaveAddress, uint8_t addr, uint8_t* buffer, uint8_t len,
                     bool stop,
                     bool stopAfterWrite);
int32_t readAddr(uint8_t slaveAddress, uint8_t* buffer, uint8_t len, bool stop);

extern uint32_t errorCount;
extern spark::Vector<uint8_t> devices;
} // namespace i2c

#endif // I2C_REG_H_
