# Ledger Test

This test application provides a control request interface for managing ledgers stored on the device. It is accompanied with a command line utility that provides a user interface for the functionality exposed by the application.

## Building

The application is built and flashed like any other test application:
```sh
cd path/to/device-os/main
make -s all program-dfu PLATFORM=boron TEST=app/ledger_test
```

## Installation

The command line utility is internal to this repository and can be executed directly from the source tree. In this case, the package dependencies need to be installed:
```sh
cd path/to/device-os/user/tests/app/ledger_test/cli
npm install
./ledger --help
```

Alternatively, `npm link` can be used to create a global symlink to the utility:
```sh
npm link path/to/device-os/user/tests/app/ledger_test/cli
ledger --help
```

## Usage

Make sure a Particle device running the test application is connected to the computer via USB. Note that the command line utility does not support interacting with multiple devices and expects exactly one device to be connected to the computer.

Enumerating the ledgers stored on the device:
```sh
ledger list
```

Setting the contents of a ledger:
```sh
ledger set my_ledger '{ "key1": "value1", "key2": 123 }'
```

Getting the contents of a ledger:
```sh
ledger get my_ledger
```

See `ledger --help` for the full list of commands supported by the application.

## Debugging

Logging output is printed to the device's USB serial interface. The logging level can be changed at run time using the `debug` command.
