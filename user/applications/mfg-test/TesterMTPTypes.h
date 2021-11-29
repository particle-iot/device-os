#ifndef TESTERMTPTYPES_H
#define	TESTERMTPTYPES_H

const unsigned ALL_MTP_DATA_BUFFERED = 0x0F;

enum class TesterMTPTypes {
    MOBILE_SECRET = (1 << 0),
    SERIAL_NUMBER = (1 << 1),
    HARDWARE_DATA = (1 << 2),
    HARDWARE_MODEL= (1 << 3)
    // TODO: Expand for ALL provision data
};

#endif	/* TESTERMTPTYPES_H */