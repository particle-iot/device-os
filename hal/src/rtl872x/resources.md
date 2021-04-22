# Resources used in the HAL on nRF52840 Platforms

## External QSPI Flash MX25L3233F

### OTP Region (512 bytes/4K bits)

The system firmware does not set the secure lock-down bit for the OTP region. See section 6.II of the datasheet. 

#### Partitioning scheme

| Address Range | Usage |
| ------------- | ----- |
| 0x000-0x07F | reserved for system use |
| 0x080-0x1FF | available for application use |

#### System OTP Memory Map

| Address Range | Usage |
| ------------- | ----- |
| 0x000-0x00F | 15-character null-terminated serial number |
| 0x010-0x07F | reserved |






