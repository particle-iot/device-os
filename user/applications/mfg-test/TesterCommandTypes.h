#ifndef TESTERCOMMANDTYPES_H
#define	TESTERCOMMANDTYPES_H

enum MfgTestKeyType {
    MFG_TEST_FLASH_ENCRYPTION_KEY,
    MFG_TEST_SECURE_BOOT_KEY,
    MFG_TEST_WIFI_MAC,
    MFG_TEST_MOBILE_SECRET,
    MFG_TEST_SERIAL_NUMBER,
    MFG_TEST_HW_VERSION,
    MFG_TEST_HW_MODEL,
    MFG_TEST_END
};

enum EfuseDataTypes {
    EFUSE_DATA_FLASH_ENCRYPTION_KEY,
    EFUSE_DATA_FLASH_ENCRYPTION_LOCK_BITS,
    EFUSE_DATA_SECURE_BOOT_KEY,
    EFUSE_DATA_SECURE_BOOT_LOCK_BITS,
    EFUSE_DATA_WIFI_MAC,
    EFUSE_DATA_MOBILE_SECRET,
    EFUSE_DATA_SERIAL_NUMBER,
    EFUSE_DATA_HARDWARE_VERSION,
    EFUSE_DATA_HARDWARE_MODEL,
    EFUSE_DATA_MAX
};

#endif	/* TESTERCOMMANDTYPES_H */
