About
===

Travis-CI is setup to flash these `firmware/*.ino` files to cores and run bash scripts that interrogate them
via `Spark.variable`, `Spark.function`, `Spark.publish`, `Spark.subscribe` (with the Spark CLI).

See `.travis.yml` for details.

On Device Tests
---------------

Folder layout for tests is

```
  tests/[platform]/[suite]/[sources *.cpp/*.h]
```

The script `ci/integration-tests.sh` runs each test suite and collects the results.

Running each test involves:

- making the test suite binary
- OTA flashing the binary to the designated spark for the test platform
- waiting for the OTA flash to complete and the test suite to initialize
- configuring the test suite - which tests are included/excluded
- running the tests
- collecting the test log and test result

When all suites return success, the build succeeds.

Unit Tests
----------

The unit tests are also built and executed. These are found under

```
   tests/unit/
```

These tests are compiled under regular gcc (not the ARM version) and executed on
the build server locally.

Only one test runner is built since all tests can be compiled together.

Adding a new test done by creating a new `.cpp` file named after the functional
area the tests will cover, and then coding tests using the
[Catch](https://github.com/philsquared/Catch) framework.


