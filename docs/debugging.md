
## Debug Build

The firmware includes a debugging aid that enables debug output, from the system and from your own application code.

To create a debug build, add `DEBUG_BUILD=y` to the `make` command line. If the previous build was not a debug build then
you should add `clean` to perform a clean build.

On the photon, the system modules must also be rebuilt also with `DEBUG_BUILD` set.

Since 0.6.0 the firmware includes newer logging framework. The system API is described in the services/inc/logging.h file. The application API is described in the firmware reference: docs/reference/firmware.md
