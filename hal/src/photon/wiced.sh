#!/bin/bash

set -e

# override PHOTON_WICED_REPO_PATH with your photon-wiced repo location, clean build with following:
# PHOTON_WICED_REPO_PATH=/Users/particle/code/photon-wiced/ ./wiced.sh clean
#
# or export in .bash_profile as:
# export PHOTON_WICED_REPO_PATH=/Users/particle/code/photon-wiced/
if [ -z "$PHOTON_WICED_REPO_PATH" ]; then
    PHOTON_WICED_REPO_PATH=/spark/photon-wiced
fi

OPTS="UPDATE_FROM_SDK=3_3_0_PARTICLE DEFINE_BOOTLOADER=1 DEFINE_PARTICLE_DCT_COMPATIBILITY=1"

function update()
{
   rsync -avz --update --existing $2/ $1
}

pushd $PHOTON_WICED_REPO_PATH

if [ "$1" == "clean" ]; then
  ./make clean
fi

# ./make demo.soft_ap-BCM9WCDUSI09-ThreadX-NetX-SDIO $OPTS
# ./make demo.soft_ap-BCM9WCDUSI14-ThreadX-NetX-SDIO $OPTS
./make demo.soft_ap-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO $OPTS
./make demo.soft_ap-BCM9WCDUSI14-FreeRTOS-LwIP-SDIO $OPTS

popd

# update headers
update . $PHOTON_WICED_REPO_PATH
update wiced $PHOTON_WICED_REPO_PATH/WICED


# update platform libraries (any RTOS)
update lib $PHOTON_WICED_REPO_PATH/build/demo.soft_ap-BCM9WCDUSI14-FreeRTOS-LwIP-SDIO/libraries
update lib $PHOTON_WICED_REPO_PATH/build/demo.soft_ap-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO/libraries
update lib/FreeRTOS $PHOTON_WICED_REPO_PATH/build/demo.soft_ap-BCM9WCDUSI14-FreeRTOS-LwIP-SDIO/libraries
update lib/FreeRTOS $PHOTON_WICED_REPO_PATH/build/demo.soft_ap-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO/libraries

# Bootloader optimized build
# pushd $PHOTON_WICED_REPO_PATH
# We have to clean anyway, otherwise PARTICLE_FLASH_SPACE_OPTIMIZE=y will not take effect
# ./make clean
# ./make demo.soft_ap-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO $OPTS PARTICLE_FLASH_SPACE_OPTIMIZE=y
# popd

cp $PHOTON_WICED_REPO_PATH/build/demo.soft_ap-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO/libraries/STM32F2xx.a lib/FreeRTOS/STM32F2xx_bootloader.a

# Split Lib_crypto_open.a into two separate libraries
rm -f lib/Lib_crypto_open_part2.a
rm -f des.o md4.o sha2.o x509parse.o md5.o sha256.o
arm-none-eabi-ar x lib/Lib_crypto_open.a des.o md4.o sha2.o x509parse.o md5.o sha256.o
arm-none-eabi-ar d lib/Lib_crypto_open.a des.o md4.o sha2.o x509parse.o md5.o sha256.o
arm-none-eabi-ar r lib/Lib_crypto_open_part2.a des.o md4.o sha2.o x509parse.o md5.o sha256.o
rm -f des.o md4.o sha2.o x509parse.o md5.o sha256.o
arm-none-eabi-objcopy --weaken-symbols lib/Lib_crypto_open.a.weaken \
                      lib/Lib_crypto_open.a

arm-none-eabi-objcopy --strip-symbols lib/BESL.ARM_CM3.release.a.strip \
                      --weaken-symbols lib/BESL.ARM_CM3.release.a.weaken \
                      --strip-unneeded \
                      lib/BESL.ARM_CM3.release.a
