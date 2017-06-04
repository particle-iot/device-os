#include "application.h"
#include "i2c_helper.h"

namespace i2c {

uint32_t errorCount = 0;
spark::Vector<uint8_t> devices;

void reset() {
    USE_WIRE.setSpeed(400000);
    USE_WIRE.begin();
    // Just in case reset the bus
    USE_WIRE.reset();

    errorCount = 0;
}

int32_t writeRegister(uint8_t slaveAddress, uint8_t addr, uint8_t val, bool stop) {
    USE_WIRE.beginTransmission(slaveAddress);
    USE_WIRE.write(addr);
    USE_WIRE.write(val);
    int32_t err = -USE_WIRE.endTransmission(stop);
    if (err < 0) {
        ++errorCount;
        ERROR("%d e %d 0x%02x 0x%02x 1 %d", errorCount, err, slaveAddress, addr, stop);
    }
    return err;
}

int32_t writeRegister(uint8_t slaveAddress, uint8_t addr, uint8_t* buffer, uint8_t len, bool stop) {
    USE_WIRE.beginTransmission(slaveAddress);
    USE_WIRE.write(addr);
    USE_WIRE.write(buffer, len);
    int32_t err = -USE_WIRE.endTransmission(stop);
    if (err < 0) {
        ++errorCount;
        ERROR("%d e %d 0x%02x 0x%02x %d %d", errorCount, err, slaveAddress, addr, len, stop);
    }
    return err;
}

int32_t readRegister(uint8_t slaveAddress, uint8_t addr, bool stop, bool stopAfterWrite) {
    USE_WIRE.beginTransmission(slaveAddress);
    USE_WIRE.write(addr);
    int32_t err = -USE_WIRE.endTransmission(stopAfterWrite);
    if (err == 0) {
        err = USE_WIRE.requestFrom(slaveAddress, (uint8_t)1, (uint8_t)stop);
        if (err == 1) {
            return USE_WIRE.read();
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
    USE_WIRE.beginTransmission(slaveAddress);
    USE_WIRE.write(addr);
    int32_t err = -USE_WIRE.endTransmission(stopAfterWrite);
    if (err == 0) {
        err = USE_WIRE.requestFrom(slaveAddress, len, (uint8_t)stop);
        if (err == len) {
            while (len-- > 0) {
                uint8_t b = USE_WIRE.read();
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
    int32_t err = USE_WIRE.requestFrom(slaveAddress, len, (uint8_t)stop);
    if (err == len) {
        while(len-- > 0) {
            (*buffer++) = USE_WIRE.read();
        }
    } else {
        ++errorCount;
        ERROR("%d, r %d 0x%02x %d %d", errorCount, err, slaveAddress, len, stop);
    }
    return err;
}

} // namespace i2c