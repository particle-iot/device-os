[![CircleCI](https://circleci.com/gh/particle-iot/device-os/tree/develop.svg?style=svg)](https://circleci.com/gh/particle-iot/device-os/tree/develop)

# Particle Device OS Firmware

This is the main source code repository of the Particle firmware libraries.

# Getting Started

To get started building firmware locally, see [Getting Started](docs/gettingstarted.md).

# Resources

- [Latest Release](http://github.com/particle-iot/device-os/releases/)
- [Changelog](CHANGELOG.md)

## Build System

- [Requirements/Dependencies](docs/dependencies.md)

## Application Firmware Development

- [Debugging support](docs/debugging.md)
- [make command syntax](docs/build.md)
- [Firmware API](http://docs.particle.io/)

## System Firmware Development

- [System Flags](system/system-flags.md)
- [Continuous Integration](ci/README.md)
- [Module Descriptor linking and retrieval](dynalib/src/readme.md)
- [Testing](user/tests/readme.md)
- [build test suite](build/test/readme.md)
- [System Threading](system/system-threading.md)
- [system versions and releases](system/system-versions.md)

### Modules

- [Bootloader](bootloader/README.md)
- [Cloud Communications](communication/README.md)

### Platforms

- [Virtual Device](hal/src/gcc/readme.md)
- [Starting a New Platform Hardware Abstraction Layer](hal/src/newhal/readme.md)

### CREDITS AND ATTRIBUTIONS

The firmware uses the GNU GCC toolchain for ARM Cortex-M processors, ARM's CMSIS libraries, and Arduino's implementation of Wiring.

### LICENSE

Unless stated elsewhere, file headers or otherwise, all files herein are licensed under an LGPLv3 license. For more information, please read the LICENSE file.

If you have questions about software licensing, please contact Particle [support](https://support.particle.io/).


### LICENSE FAQ

**This firmware is released under LGPL version 3, what does that mean for you?**

 * You may use this commercially to build applications for your devices!  You **DO NOT** need to distribute your object files or the source code of your Application under LGPL.  Your source code belongs to you when you build an Application using this System Firmware.

**When am I required to share my code?**

 * You are **NOT required** to share your Application Firmware or object files when linking against libraries or System Firmware licensed under LGPL.

 * If you make and distribute changes to System Firmware licensed under LGPL, you must release the source code and documentation for those changes under a LGPL license.

**Why?**

 * This license allows businesses to confidently build firmware and make devices without risk to their intellectual property, while at the same time helping the community benefit from non-proprietary contributions to the shared System Firmware.

**Questions / Concerns?**

 * Particle intends for this firmware to be commercially useful and safe for our community of makers and enterprises.  Please [Contact Us](https://support.particle.io/) if you have any questions or concerns, or if you require special licensing.

_(Note!  This FAQ isn't meant to be legal advice, if you're unsure, please consult an attorney)_


### CONTRIBUTE

Want to contribute to the Particle firmware project? Follow [this link](CONTRIBUTING.md) to find out how.

### CONNECT

Having problems or have awesome suggestions? Connect with us [here.](https://community.particle.io/)
