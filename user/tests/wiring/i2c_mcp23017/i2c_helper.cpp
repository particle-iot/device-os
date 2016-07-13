#include "application.h"
#include "i2c_helper.h"

namespace i2c {

uint32_t errorCount = 0;
std::vector<uint8_t> devices;

void reset() {
    Wire.setSpeed(400000);
    Wire.begin();
    // Just in case reset the bus
    Wire.reset();

    errorCount = 0;
}

int32_t writeRegister(uint8_t slaveAddress, uint8_t addr, uint8_t val, bool stop) {
    Wire.beginTransmission(slaveAddress);
    Wire.write(addr);
    Wire.write(val);
    int32_t err = -Wire.endTransmission(stop);
    if (err < 0) {
        ++errorCount;
        ERROR("%d e %d 0x%02x 0x%02x 1 %d", errorCount, err, slaveAddress, addr, stop);
    }
    return err;
}

int32_t writeRegister(uint8_t slaveAddress, uint8_t addr, uint8_t* buffer, uint8_t len, bool stop) {
    Wire.beginTransmission(slaveAddress);
    Wire.write(addr);
    Wire.write(buffer, len);
    int32_t err = -Wire.endTransmission(stop);
    if (err < 0) {
        ++errorCount;
        ERROR("%d e %d 0x%02x 0x%02x %d %d", errorCount, err, slaveAddress, addr, len, stop);
    }
    return err;
}

int32_t readRegister(uint8_t slaveAddress, uint8_t addr, bool stop, bool stopAfterWrite) {
    Wire.beginTransmission(slaveAddress);
    Wire.write(addr);
    int32_t err = -Wire.endTransmission(stopAfterWrite);
    if (err == 0) {
        err = Wire.requestFrom(slaveAddress, (uint8_t)1, (uint8_t)stop);
        if (err == 1) {
            return Wire.read();
        } else {
            ++errorCount;
            ERROR("%d r %d 0x%02x 0x%02x %d", errorCount, err, slaveAddress, addr, stop);
        }
    } else {
        ++errorCount;
        ERROR("%d e %d 0x%02x 0x%02x %d", errorCount, err, slaveAddress, addr, stop);
    }
    return err;
}

int32_t readRegister(uint8_t slaveAddress, uint8_t addr, uint8_t* buffer, uint8_t len, bool stop, bool stopAfterWrite) {
    Wire.beginTransmission(slaveAddress);
    Wire.write(addr);
    int32_t err = -Wire.endTransmission(stopAfterWrite);
    if (err == 0) {
        err = Wire.requestFrom(slaveAddress, len, (uint8_t)stop);
        if (err == len) {
            while (len-- > 0) {
                uint8_t b = Wire.read();
                if (buffer != nullptr)
                    (*buffer++) = b;
            }
        } else {
            ++errorCount;
            ERROR("%d r %d 0x%02x 0x%02x %d %d", errorCount, err, slaveAddress, addr, len, stop);
        }
    } else {
        ++errorCount;
        ERROR("%d e %d 0x%02x 0x%02x %d %d", errorCount, err, slaveAddress, addr, len, stop);
    }
    return err;
}

int32_t readAddr(uint8_t slaveAddress, uint8_t* buffer, uint8_t len, bool stop) {
    int32_t err = Wire.requestFrom(slaveAddress, len, (uint8_t)stop);
    if (err == len) {
        while(len-- > 0) {
            (*buffer++) = Wire.read();
        }
    } else {
        ++errorCount;
        ERROR("%d, r %d 0x%02x %d %d", errorCount, err, slaveAddress, len, stop);
    }
    return err;
}

} // namespace i2c