About
===

Travis-CI is setup to flash these `firmware/*.ino` files to cores and run bash scripts that interrogate them
via Spark.variable, Spark.function, Spark.publish, Spark.subscribe (with the Spark CLI).

See `.travis.yml` for details.

On Device Tests
---------------
The core firmware is part of this repo so changes to the firmware can be evaluated
before integrating into a production branch.

Folder layout for tests is

```
  \tests\[platform]\[suite]\[sources *.cpp/*.h]
```

The script `ci\integration-tests.sh` runs each test suite and collects the results.

Running each test involves:

- making the test suite binary
- OTA flashing the binary to the designated spark for the test platform
- waiting for the OTA flash to complete and the test suite to initialize
- configuring the test suite - which tests are included/excluded
- running the tests
- collecting the test log and test result

When all suites return success, the build succeeds.





        

