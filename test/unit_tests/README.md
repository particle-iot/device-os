Unit tests
==========

Building and running tests
--------------------------

Create a build directory:

```bash
rm -rf build && mkdir build && cd build
```

Generate build files:

```bash
cmake ..
```

Build and run the tests:

```bash
device-os/test/unit_tests/build $ make all test
```

Build and run just one set of tests (ncp_fw_update):

```bash
device-os/test/unit_tests/build/ $ cd ncp_fw_update
device-os/test/unit_tests/build/ncp_fw_update $ make all test
```

Build and run the tests and coverage:

```bash
device-os/test/unit_tests/build $ make all test coverage
```

Build and run the tests and coverage, and output verbose errors on failure:

```bash
device-os/test/unit_tests/build $ make all test coverage CTEST_OUTPUT_ON_FAILURE=TRUE
```

Enable verbose output for monitoring all of those printf() statements while debugging tests
(note: please don't forget to disable these printf's when finished debugging)

```bash
device-os/test/unit_tests/build $ make all test coverage ARGS=--verbose
```


MacOSX
------

Github issue: https://github.com/Homebrew/homebrew-core/issues/67427

You may run into an error `ld: library not found for -licudata` and will need to export the following before running `make`

`export LIBRARY_PATH=${LIBRARY_PATH}:/usr/local/opt/icu4c/lib`
