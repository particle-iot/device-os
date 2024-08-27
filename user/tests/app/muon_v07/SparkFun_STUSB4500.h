/*
  This is a library written for the STUSB4500 Power Delivery Board.
  SparkFun sells these at its website: https://www.sparkfun.com

  The functions were based on the NVM_Flasher code for
  the STUSB4500 which can be found here: https://github.com/usb-c/STUSB4500

  Written by Alex Wende @ SparkFun Electronics, February 6th, 2020

  https://github.com/sparkfun/SparkFun_STUSB4500_Arduino_Library

  Do you like this library? Help support SparkFun. Buy a board!

  Development environment specifics:
  Arduino IDE 1.8.6

  For licence information see LICENSE.md
  https://github.com/sparkfun/SparkFun_STUSB4500_Arduino_Library/blob/master/LICENSE.md
*/

#ifndef SPARKFUN_STUSB4500_H
#define SPARKFUN_STUSB4500_H

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "stusb4500_register_map.h"
#include <Wire.h>

#define USBPD_REV_3_0_SUPPORT
typedef union {
    uint16_t d16;
    struct {
#if defined(USBPD_REV_3_0_SUPPORT)
        uint16_t messageType : 5;     // USBPD rev >= 3.0 message type
#else
        uint16_t messageType : 4;     // USBPD rev  < 3.0 message type
        uint16_t reserved_4 : 1;      // reserved
#endif
        uint16_t portDataRole : 1;    // port data role
        uint16_t specRevision : 2;    // spec revision
        uint16_t portPowerRole : 1;   // port power role/cable plug
        uint16_t messageID : 3;       // message ID
        uint16_t dataObjectCount : 3; // number of data objects
        uint16_t extended : 1;        // reserved
    } b;
} USBPDMessageHeader;

typedef union
  {
    uint32_t d32;
    struct
      {
        uint32_t Max_Operating_Current :10;
        uint32_t Voltage :10;
        uint8_t PeakCurrent:2;
        uint8_t Reserved :2;
        uint8_t Unchuncked_Extended:1;
        uint8_t Dual_RoleData:1;
        uint8_t Communication:1;
        uint8_t UnconstraintPower:1;
        uint8_t SuspendSupported:1;
        uint8_t DualRolePower :1;
        uint8_t FixedSupply:2;
      } fix;
    struct
      {
        uint32_t Operating_Current :10;
        uint32_t Min_Voltage:10;
        uint32_t Max_Voltage:10;
        uint8_t VariableSupply:2;
      }var;
    struct
      {
        uint32_t Operating_Power :10;
        uint32_t Min_Voltage:10;
        uint32_t Max_Voltage:10;
        uint8_t Battery:2;
      }bat;
    struct
      {
        /*
        uint8_t Max_Current :7;
        uint8_t  Reserved0 :1;
        uint8_t Min_Voltage:8;
        uint8_t  Reserved1 :1;
        uint8_t Max_Voltage:9;
        uint8_t  Reserved2 :2;
        uint8_t ProgDev : 2 ;
        uint8_t Battery:2;
        */
       uint8_t Max_Current :7;
       uint8_t  Reserved0 :1;
       uint16_t Min_Voltage:8; /* to prevent packing issue ?? */
       uint8_t  Reserved1 :1;
       uint16_t Max_Voltage:8;
       uint8_t  Reserved2 :3;
       uint8_t ProgDev : 2 ;
       uint8_t Battery:2;

      }apdo;
  }USB_PD_SRC_PDOTypeDef;

enum class CableStatus {
    NONE = -1,
    NotConnected, // = 0
    CC1Connected, // = 1
    CC2Connected, // = 2
    COUNT         // = 3
};

enum class PDOSupplyType {
    NONE = -1,
    Fixed,    // = 0
    Variable, // = 1
    Battery,  // = 2
    COUNT     // = 3
};

#define CABLE_CONNECTED(c) ((CableStatus::CC1Connected == (c)) || (CableStatus::CC2Connected == (c)))

class STUSB4500 {
public:
    /*
      Initializes the I2C bus. If the device ID is configured for a address other than the default
      it should be intialized here. Valid IDs are 0x28 (default), 0x29, 0x2A, and 0x2B. If another
      I2C bus is used (such as Wire1 or Wire2), it can be defined here as well, the default is Wire.
    */
    uint8_t begin(uint8_t deviceAddress = 0x28, TwoWire& wirePort = Wire);

    /*
      Reads the NVM memory from the STUSB4500
    */
    void read(void);

    /*
      Write NVM settings to the STUSB4500. Optional: Passing a 255 value to the function will write
      the default NVM values to the STUSB4500.
    */
    void write(uint8_t defaultVals = 0);

    /*
      Returns the voltage stored for the three power data objects (PDO).
      Parameter: pdo_numb - the PDO number to be read (1 to 3).
      1 - PDO1 (fixed at 5V)
      2 - PDO2 (0-20V, 20mV resolution)
      3 - PDO3 (0-20V, 20mV resolution)
    */
    float getVoltage(uint8_t pdo_numb);

    /*
      Returns the current stored for the three power data objects (PDO).
      Parameter: pdo_numb - the PDO number to be read (1 to 3).
    */
    float getCurrent(uint8_t pdo_numb);

    /*
      Retruns the over voltage lock out variable (5-20%)
      Parameter: pdo_numb - the PDO number to be read (1 to 3).
    */
    uint8_t getUpperVoltageLimit(uint8_t pdo_numb);

    /*
      Retruns the under voltage lock out variable (5-20%)
      Note: PDO1 has a fixed threshold of 3.3V.
      Parameter: pdo_numb - the PDO number to be read (1 to 3).
    */
    uint8_t getLowerVoltageLimit(uint8_t pdo_numb);

    /*
      Returns the global current value common to all PDO numbers.
    */
    float getFlexCurrent(void);

    /*
      Returns the number of sink PDOs
      0,1 - PDO1
      2   - PDO2
      3   - PDO3
    */
    uint8_t getPdoNumber(void);

    /*
      Returns the SNK_UNCONS_POWER parameter value.
      0 - No external source of power
      1 - An external power source is available and is sufficient to
          adequately power the system while charging external devices.
    */
    uint8_t getExternalPower(void);

    /*
      Returns the USB_COMM_CAPABLE parameter value.
      0 - Sink does not support data communication
      1 - Sink does support data communication
    */
    uint8_t getUsbCommCapable(void);

    /*
      Returns the POWER_OK_CFG parameter value.
      0 - Configuration 1
      1 - Not applicable
      2 - Configuration 2 (default)
      3 - Configuration 3

      Configuration 1:
      - VBUS_EN_SNK: Hi-Z - No source attached
                        0 - Source attached
      - POWER_OK2:   Hi-Z - No functionality
      - POWER_OK3:   Hi-Z - No functionality

      Configuration 2 (defualt):
      - VBUS_EN_SNK: Hi-Z - No source attached
                        0 - Source attached
      - POWER_OK2:   Hi-Z - No PD explicit contract
                        0 - PD explicit contract with PDO2
      - POWER_OK3:   Hi-Z - No PD explicit contract
                        0 - PD explicit contract with PDO3

      Configuration 3:
      - VBUS_EN_SNK: Hi-Z - No source attached
                        0 - source attached
      - POWER_OK2:   Hi-Z - No source attached or source supplies default
                            USB Type-C current at 5V when source attached.
                        0 - Source supplies 3.0A USB Type-C current at 5V
                            when source is attached.
      - POWER_OK3:   Hi-Z - No source attached or source supplies default
                            USB Type-C current at 5V when source attached.
                        0 - Source supplies 1.5A USB Type-C current at 5V
                            when source is attached.
    */
    uint8_t getConfigOkGpio(void);

    /*
      Returns the GPIO pin configuration.
      0 - SW_CTRL_GPIO
      1 - ERROR_RECOVERY
      2 - DEBUG
      3 - SINK_POWER

      SW_CTRL_GPIO:
      - Software controlled GPIO. The output state is defined by the value
        of I2C register bit-0 at address 0x2D.

        Hi-Z - When bit-0 value is 0 (at start-up)
           0 - When bit-0 value is 1

      ERROR_RECOVERY:
      - Hardware fault detection (i.e. overtemperature is detected, overvoltage is
        detected on the CC pins, or after a hard reset the power delivery communication
        with the source is broken).

        Hi-Z - No hardware fault detected
           0 - Hardware fault detected

      DEBUG:
      - Debug accessory detection

        Hi-Z - No debug accessory detected
           0 - debug accessory detected

      SINK_POWER:
      - Indicates USB Type-C current capability advertised by the source.

        Hi-Z - Source supplies defualt or 1.5A USB Type-C current at 5V
           0 - Source supplies 3.0A USB Type-C current at 5V
    */
    uint8_t getGpioCtrl(void);

    /*
      Returns the POWER_ONLY_ABOVE_5V parameter configuration.
      0 - VBUS_EN_SNK pin enabled when source is attached whatever VBUS_EN_SNK
          voltage (5V or any PDO voltage)
      1 - VBUS_EN_SNK pin enabled only when source attached and VBUS voltage
          negotiated to PDO2 or PDO3 voltage
    */
    uint8_t getPowerAbove5vOnly(void);

    /*
      Return the REQ_SRC_CURRENT parameter configuration. In case of match, selects
      which operation current from the sink or the source is to be requested in the
      RDO message.
      0 - Request I(SNK_PDO) as operating current in RDO message
      1 - Request I(SRC_PDO) as operating current in RDO message
    */
    uint8_t getReqSrcCurrent(void);

    /*
      Sets the voltage to be requested for each of the three power data objects (PDO).
      Parameter: pdo_numb - the PDO number to be read (1 to 3).
                 voltage  - the voltage to write to the PDO number.
      Note: PDO1 - Fixed at 5V
            PDO2 - 5-20V, 20mV resolution
            PDO3 - 5-20V, 20mV resolution
    */
    void setVoltage(uint8_t pdo_numb, float voltage);

    /*
      Sets the current value to be requested for each of the three power data objects (PDO).
      Parameter: pdo_numb - the PDO number to be read (1 to 3).
                 current  - the current to write to the PDO number.
      Note: Valid current values are:
      0.00*, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00,
      2.25, 2.50, 2.75, 3.00, 3.50, 4.00, 4.50, 5.00

      *A value of 0 will use the FLEX_I value instead
    */
    void setCurrent(uint8_t pdo_numb, float current);

    /*
      Sets the over votlage lock out parameter for each of the three power data objects (PDO).
      Parameter: pdo_numb - the PDO number to be read (1 to 3).
                 value    - the coefficent to shift up nominal VBUS high voltage limit
                            to the PDO number.
      Note: Valid high voltage limits are 5-20% in 1% increments
    */
    void setUpperVoltageLimit(uint8_t pdo_numb, uint8_t value);

    /*
      Sets the under votlage lock out parameter for each of the three power data objects (PDO).
      Parameter: pdo_numb - the PDO number to be read (1 to 3).
                 value    - the coefficent to shift down nominal VBUS lower voltage limit
                            to the PDO number.
      Note: Valid high voltage limits are 5-20% in 1% increments.
            PDO1 has a fixed lower limit to 3.3V.
    */
    void setLowerVoltageLimit(uint8_t pdo_numb, uint8_t value);

    /*
      Set the flexible current value common to all PDOs.
      Parameter: value - the current value to set to the FLEX_I parameter.
                         (0-5A, 10mA resolution)
    */
    void setFlexCurrent(float value);

    /*
      Sets the number of sink PDOs
      Paramter: value - Number of sink PDOs
      0 - 1 PDO (5V only)
      1 - 1 PDO (5V only)
      2 - 2 PDOs (PDO2 has the highest priority, followed by PDO1)
      3 - 3 PDOs (PDO3 has the highest priority, followed by PDO2, and then PDO1).
    */
    void setPdoNumber(uint8_t value);

    /*
      Sets the SNK_UNCONS_POWER parameter value.
      Parameter: value - Value to set to SNK_UNCONS_POWER
      0 - No external source of power
      1 - An external power source is available and is sufficient to
          adequately power the system while charging external devices.
    */
    void setExternalPower(uint8_t value);

    /*
      Sets the USB_COMM_CAPABLE parameter value.
      Parameter: value - Value to set to USB_COMM_CAPABLE
      0 - Sink does not support data communication
      1 - Sink does support data communication
    */
    void setUsbCommCapable(uint8_t value);

    /*
      Sets the POWER_OK_CFG parameter value.
      Parameter: value - Value to set to POWER_OK_CFG
      0 - Configuration 1
      1 - No applicable
      2 - Configuration 2 (default)
      3 - Configuration 3

      Configuration 1:
      - VBUS_EN_SNK: Hi-Z - No source attached
                        0 - Source attached
      - POWER_OK2:   Hi-Z - No functionality
      - POWER_OK3:   Hi-Z - No functionality

      Configuration 2 (defualt):
      - VBUS_EN_SNK: Hi-Z - No source attached
                        0 - Source attached
      - POWER_OK2:   Hi-Z - No PD explicit contract
                        0 - PD explicit contract with PDO2
      - POWER_OK3:   Hi-Z - No PD explicit contract
                        0 - PD explicit contract with PDO3

      Configuration 3:
      - VBUS_EN_SNK: Hi-Z - No source attached
                        0 - source attached
      - POWER_OK2:   Hi-Z - No source attached or source supplies default
                            USB Type-C current at 5V when source attached.
                        0 - Source supplies 3.0A USB Type-C current at 5V
                            when source is attached.
      - POWER_OK3:   Hi-Z - No source attached or source supplies default
                            USB Type-C current at 5V when source attached.
                        0 - Source supplies 1.5A USB Type-C current at 5V
                            when source is attached.
    */
    void setConfigOkGpio(uint8_t value);

    /*
      Sets the GPIO pin configuration.
      Paramter: value - Value to set to GPIO_CFG
      0 - SW_CTRL_GPIO
      1 - ERROR_RECOVERY
      2 - DEBUG
      3 - SINK_POWER

      SW_CTRL_GPIO:
      - Software controlled GPIO. The output state is defined by the value
        of I2C register bit-0 at address 0x2D.

        Hi-Z - When bit-0 value is 0 (at start-up)
           0 - When bit-0 value is 1

      ERROR_RECOVERY:
      - Hardware fault detection (i.e. overtemperature is detected, overvoltage is
        detected on the CC pins, or after a hard reset the power delivery communication
        with the source is broken).

        Hi-Z - No hardware fault detected
           0 - Hardware fault detected

      DEBUG:
      - Debug accessory detection

        Hi-Z - No debug accessory detected
           0 - debug accessory detected

      SINK_POWER:
      - Indicates USB Type-C current capability advertised by the source.

        Hi-Z - Source supplies defualt or 1.5A USB Type-C current at 5V
           0 - Source supplies 3.0A USB Type-C current at 5V
    */
    void setGpioCtrl(uint8_t value);

    /*
      Sets the POWER_ONLY_ABOVE_5V parameter configuration.
      Parameter: value - Value to select VBUS_EN_SNK pin configuration
      0 - VBUS_EN_SNK pin enabled when source is attached whatever VBUS_EN_SNK
          voltage (5V or any PDO voltage)
      1 - VBUS_EN_SNK pin enabled only when source attached and VBUS voltage
          negotiated to PDO2 or PDO3 voltage
    */
    void setPowerAbove5vOnly(uint8_t value);

    /*
      Sets the REQ_SRC_CURRENT parameter configuration. In case of match, selects
      which operation current from the sink or the source is to be requested in the
      RDO message.
      Parameter: value - Value to set to REQ_SRC_CURRENT
      0 - Request I(SNK_PDO) as operating current in RDO message
      1 - Request I(SRC_PDO) as operating current in RDO message
    */
    void setReqSrcCurrent(uint8_t value);

    /*
      Performs a soft reset to force the STUSB4500 to re-negotiate with the source.
    */
    void softReset(void);

public:
    CableStatus cableStatus(void) const;
    void printRDO();

private:
    uint8_t sector[5][8];
    bool readSectors;

    // I-squared-C Class
    TwoWire* _i2cPort; // The generic connection to user's chosen I2C hardware
    // Variables
    uint8_t _deviceAddress;

    uint32_t readPDO(uint8_t pdo_numb);
    void writePDO(uint8_t pdo_numb, uint32_t pdoData);
    uint8_t CUST_EnterWriteMode(unsigned char ErasedSector);
    uint8_t CUST_ExitTestMode(void);
    uint8_t CUST_WriteSector(char SectorNum, unsigned char* SectorData);

public:
    CableStatus cableStatus(void);

    uint8_t I2C_Write_USB_PD(uint16_t Register, uint8_t* DataW, uint16_t Length);
    uint8_t I2C_Read_USB_PD(uint16_t Register, uint8_t* DataR, uint16_t Length);
    bool wireRead(uint16_t addr, uint8_t* buff, uint16_t size);
    bool wireWrite(uint16_t addr, uint8_t* buff, uint16_t size);
};

#endif