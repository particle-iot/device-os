# Tests

- libraries - supporting libraries for test code
- reflection - back to back tests running on two cores (driver/subject arrangement)
- unit - gcc compiled unit tests
- core-v1 - on-device integration tests running on a regular spark core

## Building platform tests

Test applications are built from the parent directory `main` using make with additional
parameters:

```
make TEST=<platform>/<testappname> 
```

For example, to build `testapp1` in `core-v1`, you would write

```
make TEST=core-v1/testapp1
```

As this is the main firmware makefile, all the usual main targets are available, `program-dfu`, `program-cloud`, `clean`
etc.

The test applications are based on [Spark customizations](https://github.com/m-mcgowan/spark-unit-test) of the Arduino Unit test
library. Please see that repo for details on how to write tests, start and interact
with the test runner.


## Building unit tests

The unit tests are run on the host gcc platform and are compiled using regular
gcc. (So gcc should be in the path.)

They are built by running 

```
make
```

from the `unit` directory.

The output execute is `obj/runner` which can then be run to execute the unit tests.

The unit tests are based on the [Catch](https://github.com/philsquared/Catch)
test framework.


## Reflections tests

Please see [reflection/readme.md](reflection/readme.md).
