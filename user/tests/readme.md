# Tests

## Directory Overview

- app - test applications
- libraries - supporting libraries for test code
- reflection - back to back tests running on two cores (driver/subject arrangement)
- unit - gcc compiled unit tests
- wiring - on-device integration tests running on Gen3 devices


## Platform Tests

Platform tests execute on real hardware.

### Building Platform Tests

Test applications are built from the parent directory `main` using make with additional
parameters:

```
make TEST=<platform>/<testappname>
```

For example, to build `no_fixture` in `wiring`, you would write

```
make TEST=wiring/no_fixture
```

As this is the main firmware makefile, all the usual main targets are available, `program-dfu`, `program-cloud`, `clean`
etc.

The test applications are based on [Spark customizations](https://github.com/m-mcgowan/spark-unit-test) of the Arduino Unit test
library. Please see that repo for details on how to write tests, start and interact
with the test runner.

## Running Platform Tests

The first step is to flash the test app to a device, e.g.

```
make TEST=wiring/no_fixture all program-dfu
```

The device will then restart and connect to the cloud - most tests enable the cloud connection so tha the tests can be monitored remotely. 

The tests can then be started using either the cloud functions or the serial interface.

### Using the serial interface:

The serial interface allows the set of tests to run to be changed before the test suite is started. This is done by using `i` or `e` commands, and specifying a glob for the tests to include or exclude.

For example, after connecting to USB serial:

```

Glob me for tests to include: spi*
Included tests matching 'spi*'.


Commands:

- `i`: tests to include - if this is the first command then all tests are exluded by default
- `e`: tests to exclude (skip) - if this is the first command then all tests are included by default
- `I`: include all tests
- `E`: exclude all tests
- `l`: list tests included
- `t`: run the tests

```

# Unit Tests

Unit tests are executed on your development machine. They test the code indepedently from any hardware.

## Building unit tests

The unit tests are run on the host gcc platform and are compiled using regular
gcc. (So gcc should be in the path.)

[BOOST](http://www.boost.org/) standard C++ libraries are also required.  On Mac OSX, use Homebrew to install boost with `brew install boost`.  The command will end with the location of boost.  Copy this and add it to your `~/.bash_profile` like so `export BOOST_ROOT="/usr/local/Cellar/boost/1.62.0/"`

Unit tests are then built and executed by running:

```
cd user/tests/unit
make
```

The unit tests are based on the [Catch](https://github.com/philsquared/Catch)
test framework.


## Reflections tests

This is work in progress.
Please see [reflection/readme.md](reflection/readme.md).

## Test Applications
A number of manually executable test applications are located in the `user/tests/app/` folder. Each application is stored in it's own folder.  The application is built and flashed to the device by running

```
cd main
make APP=../tests/app/<appname> all program-dfu
```