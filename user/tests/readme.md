# Tests

- libraries - supporting libraries for test code
- reflection - back to back tests running on two cores (driver/subject arrangement)
- unit - gcc compiled unit tests
- wiring - on-device integration tests running on a regular Core, Photon or P1 (Electron to be tested.)

## Building platform tests

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


## Building unit tests

The unit tests are run on the host gcc platform and are compiled using regular
gcc. (So gcc should be in the path.)

They are built and executed by running

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

