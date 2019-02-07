#!/bin/bash

version=0.9.0-rc.1

set -e

rm -rf release
mkdir -p release

# Bootloader

cd bootloader

rm -rf ../build/target
make -s all PLATFORM=xenon COMPILE_LTO=n
cp ../build/target/bootloader/platform-14/bootloader.bin ../release/bootloader-${version}-xenon.bin
cp ../build/target/bootloader/platform-14/bootloader.elf ../release/bootloader-${version}-xenon.elf

rm -rf ../build/target
make -s all PLATFORM=argon COMPILE_LTO=n
cp ../build/target/bootloader/platform-12/bootloader.bin ../release/bootloader-${version}-argon.bin
cp ../build/target/bootloader/platform-12/bootloader.elf ../release/bootloader-${version}-argon.elf

rm -rf ../build/target
make -s all PLATFORM=boron COMPILE_LTO=n
cp ../build/target/bootloader/platform-13/bootloader.bin ../release/bootloader-${version}-boron.bin
cp ../build/target/bootloader/platform-13/bootloader.elf ../release/bootloader-${version}-boron.elf

# Modular firmware

cd ../modules
rm -rf ../build/target
make -s all PLATFORM=xenon APP=tinker
cp ../build/target/system-part1/platform-14-m/system-part1.bin ../release/system-part1-${version}-xenon.bin
cp ../build/target/system-part1/platform-14-m/system-part1.elf ../release/system-part1-${version}-xenon.elf
cp ../build/target/user-part/platform-14-m/tinker.bin ../release/tinker-${version}-xenon.bin
cp ../build/target/user-part/platform-14-m/tinker.elf ../release/tinker-${version}-xenon.elf
cd ../main
make -s all PLATFORM=xenon APP=tinker-serial1-debugging
cp ../build/target/user-part/platform-14-m/tinker-serial1-debugging.bin ../release/tinker-serial1-debugging-${version}-xenon.bin
cp ../build/target/user-part/platform-14-m/tinker-serial1-debugging.elf ../release/tinker-serial1-debugging-${version}-xenon.elf

cd ../modules
rm -rf ../build/target
make -s all PLATFORM=argon APP=tinker
cp ../build/target/system-part1/platform-12-m/system-part1.bin ../release/system-part1-${version}-argon.bin
cp ../build/target/system-part1/platform-12-m/system-part1.elf ../release/system-part1-${version}-argon.elf
cp ../build/target/user-part/platform-12-m/tinker.bin ../release/tinker-${version}-argon.bin
cp ../build/target/user-part/platform-12-m/tinker.elf ../release/tinker-${version}-argon.elf
cd ../main
make -s all PLATFORM=argon APP=tinker-serial1-debugging
cp ../build/target/user-part/platform-12-m/tinker-serial1-debugging.bin ../release/tinker-serial1-debugging-${version}-argon.bin
cp ../build/target/user-part/platform-12-m/tinker-serial1-debugging.elf ../release/tinker-serial1-debugging-${version}-argon.elf

cd ../modules
rm -rf ../build/target
make -s all PLATFORM=boron APP=tinker
cp ../build/target/system-part1/platform-13-m/system-part1.bin ../release/system-part1-${version}-boron.bin
cp ../build/target/system-part1/platform-13-m/system-part1.elf ../release/system-part1-${version}-boron.elf
cp ../build/target/user-part/platform-13-m/tinker.bin ../release/tinker-${version}-boron.bin
cp ../build/target/user-part/platform-13-m/tinker.elf ../release/tinker-${version}-boron.elf
cd ../main
make -s all PLATFORM=boron APP=tinker-serial1-debugging
cp ../build/target/user-part/platform-13-m/tinker-serial1-debugging.bin ../release/tinker-serial1-debugging-${version}-boron.bin
cp ../build/target/user-part/platform-13-m/tinker-serial1-debugging.elf ../release/tinker-serial1-debugging-${version}-boron.elf

# Non-modular tinker with debugging enabled

cd ../main

rm -rf ../build/target
make -s all PLATFORM=xenon APP=tinker-serial1-debugging MODULAR=n DEBUG_BUILD=y
cp ../build/target/main/platform-14/tinker-serial1-debugging.bin ../release/tinker-serial1-debugging-${version}-xenon-mono.bin
cp ../build/target/main/platform-14/tinker-serial1-debugging.elf ../release/tinker-serial1-debugging-${version}-xenon-mono.elf

rm -rf ../build/target
make -s all PLATFORM=argon APP=tinker-serial1-debugging MODULAR=n DEBUG_BUILD=y
cp ../build/target/main/platform-12/tinker-serial1-debugging.bin ../release/tinker-serial1-debugging-${version}-argon-mono.bin
cp ../build/target/main/platform-12/tinker-serial1-debugging.elf ../release/tinker-serial1-debugging-${version}-argon-mono.elf

rm -rf ../build/target
make -s all PLATFORM=boron APP=tinker-serial1-debugging MODULAR=n DEBUG_BUILD=y
cp ../build/target/main/platform-13/tinker-serial1-debugging.bin ../release/tinker-serial1-debugging-${version}-boron-mono.bin
cp ../build/target/main/platform-13/tinker-serial1-debugging.elf ../release/tinker-serial1-debugging-${version}-boron-mono.elf

# Hybrid firmware

cd ../modules

rm -rf ../build/target
make -s all PLATFORM=xenon HYBRID_BUILD=y INCLUDE_APP=y
cp ../build/target/system-part1/platform-14-m/system-part1.bin ../release/hybrid-${version}-xenon.bin
cp ../build/target/system-part1/platform-14-m/system-part1.elf ../release/hybrid-${version}-xenon.elf

rm -rf ../build/target
make -s all PLATFORM=argon HYBRID_BUILD=y INCLUDE_APP=y
cp ../build/target/system-part1/platform-12-m/system-part1.bin ../release/hybrid-${version}-argon.bin
cp ../build/target/system-part1/platform-12-m/system-part1.elf ../release/hybrid-${version}-argon.elf

rm -rf ../build/target
make -s all PLATFORM=boron HYBRID_BUILD=y INCLUDE_APP=y
cp ../build/target/system-part1/platform-13-m/system-part1.bin ../release/hybrid-${version}-boron.bin
cp ../build/target/system-part1/platform-13-m/system-part1.elf ../release/hybrid-${version}-boron.elf
