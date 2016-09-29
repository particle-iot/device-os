# The cloud test app

This cloud test app implements some automated end to end tests using a device and the cloud. 

## Prerequisites

- [BATS](https://github.com/sstephenson/bats#installing-bats-from-source)
- Particle CLI 
- jq - [command line json parser](https://stedolan.github.io/jq/)
 - on OSX `brew install js`
- this firmware repo checked out in git
- `gem install retryit`

## Device Setup

- ensure your device is online and claimed to your currently active particle-cli account
- put the device in DFU mode
- from the root of firmware `cd main`
- `make PLATFORM=xxx APP=../tests/app/cloudtest all program-dfu` to flash the cloud test app to your device
- `export DEVICE_ID=<your device ID>`
- `export DEVICE_NAME=<your device name>`
- `export PLATFORM_ID=10`
- `export API_URL=https://api.staging.particle.io`
- `export ACCESS_TOKEN=....`

## Running the tests
- `cd` to this folder
- `bats cloud.bats`

