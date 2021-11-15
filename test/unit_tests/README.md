Unit tests
==========

Building and running tests
--------------------------

Create a build directory:

```bash
rm -rf .build && mkdir .build && cd .build
```

Generate build files:

```bash
cmake ..
```

Build and run the tests and coverage:

```bash
make all test coverage
```


MacOSX
------

Github issue: https://github.com/Homebrew/homebrew-core/issues/67427

You may run into an error `ld: library not found for -licudata` and will need to export the following before running `make`

`export LIBRARY_PATH=${LIBRARY_PATH}:/usr/local/opt/icu4c/lib`
