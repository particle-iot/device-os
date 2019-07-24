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
