# override PHOTON_WICED_REPO_PATH with your photon-wiced repo location, clean build with following:
# PHOTON_WICED_REPO_PATH=/Users/particle/code/photon-wiced/ ./wiced.sh clean
#
# or export in .bash_profile as:
# export PHOTON_WICED_REPO_PATH=/Users/particle/code/photon-wiced/
if [ -z "$PHOTON_WICED_REPO_PATH" ]; then
    PHOTON_WICED_REPO_PATH=/spark/photon-wiced
fi

OPTS=

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


# update platform libraries (any RTOS)
update lib $PHOTON_WICED_REPO_PATH/build/demo_soft_ap-BCM9WCDUSI14-FreeRTOS-LwIP-SDIO/libraries
update lib $PHOTON_WICED_REPO_PATH/build/demo_soft_ap-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO/libraries
update lib/FreeRTOS $PHOTON_WICED_REPO_PATH/build/demo_soft_ap-BCM9WCDUSI14-FreeRTOS-LwIP-SDIO/libraries
update lib/FreeRTOS $PHOTON_WICED_REPO_PATH/build/demo_soft_ap-BCM9WCDUSI09-FreeRTOS-LwIP-SDIO/libraries


