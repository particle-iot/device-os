printf "===========================================\r\n"
printf "         Load KM4 init script              \r\n"
printf "===========================================\r\n"

set mem inaccessible-by-default off
# KM0 ROM 128KB + 32KB
mem 0x00000000 0x00027fff ro
# KM0 RAM 64KB
mem 0x00080000 0x0008ffff rw
# KM4 ROM 768KB(actually 256KB) + 96KB
mem 0x10100000 0x101d7fff ro
# KM4 RAM 512KB
mem 0x10000000 0x1007ffff rw
# External flash 8MB
mem 0x08000000 0x087fffff ro
# External PSRAM 4MB
mem 0x02000000 0x023fffff ro
# xip address
mem 0x0c000000 0x0c7fffff ro
mem 0x0e000000 0x0e7fffff ro

# KM4 jump out of boot
set (*0x480003F8) = 0x02000201

info mem
