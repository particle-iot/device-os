# Resources used in the HAL on nRF52840 Platforms

## User Information Configuration Registers

CUSTOMER[0] - Serial Number 0
CUSTOMER[1] - Serial Number 1
CUSTOMER[2] - Serial Number 2
CUSTOMER[3-16] - Reserved for Particle Firmware
CUSTOMER[17-31] - Reserved for application firmware

### Serial Number - 15 alphanumeric uppercase digits. This is 15 digits base 36

The serial number is 15 characters wide. Each register from 0-2 holds 5 characters and 2 reserved bits.

CUSTOMER[0] - bits 31-26: serial code ASCII alphanumeric character 1 (leftmost)
CUSTOMER[0] - bits 25-20: serial code ASCII alphanumeric character 2
CUSTOMER[0] - bits 19-14: serial code ASCII alphanumeric character 3
CUSTOMER[0] - bits 13-08: serial code ASCII alphanumeric character 4
CUSTOMER[0] - bits 07-02: serial code ASCII alphanumeric character 5
CUSTOMER[0] - bits 01-00: reserved

CUSTOMER[1] - bits 31-26: serial code ASCII alphanumeric character 6
CUSTOMER[1] - bits 25-20: serial code ASCII alphanumeric character 7
CUSTOMER[1] - bits 19-14: serial code ASCII alphanumeric character 8
CUSTOMER[1] - bits 13-08: serial code ASCII alphanumeric character 9
CUSTOMER[1] - bits 07-02: serial code ASCII alphanumeric character 10
CUSTOMER[1] - bits 01-00: reserved

CUSTOMER[2] - bits 31-26: serial code ASCII alphanumeric character 11
CUSTOMER[2] - bits 25-20: serial code ASCII alphanumeric character 12
CUSTOMER[2] - bits 19-14: serial code ASCII alphanumeric character 13
CUSTOMER[2] - bits 13-08: serial code ASCII alphanumeric character 14
CUSTOMER[2] - bits 07-02: serial code ASCII alphanumeric character 15 (rightmost)
CUSTOMER[2] - bits 01-00: reserved







