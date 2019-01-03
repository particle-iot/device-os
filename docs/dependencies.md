# Particle toolchain dependencies

There are two options for building the Particle firmware.

1. You can download and install the dependencies natively
1. You can install Docker and run the toolchain in a container _(with all dependencies supplied therein)_

Both options are presented below, and only one is required to build the Particle firmware.

## Docker build container

The Docker build container will execute `make`, and any parameters you pass behind it will be passed directly to `make`. Although more verbose, the Docker build container has a couple of advantages over native installation.

1. Isolation of build dependencies required by the Particle firmware
1. Dependencies are always in sync and are managed by Particle

### Building Particle firmware

```bash
docker run --rm --volume <local firmware repository>:/particle-iot/firmware --workdir /particle-iot/firmware/modules particle/toolchain clean all PLATFORM=(core|electron|p1|photon)
```

**Option breakdown:**

- `<local firmware repository>` - This is the path to the firmware folder on your local machine.
- `(core|electron|p1|photon)` - Select the option from this list that matches the device for which you are compiling.

### Building a user application

```bash
docker run --rm --volume <local firmware repository>:/particle-iot/firmware --volume <user application folder>:/particle-iot/app --workdir /particle-iot/firmware/main particle/toolchain clean all PLATFORM=(core|electron|p1|photon) APPDIR=/particle-iot/app TARGET_FILE=<user application>
```

**Option breakdown:**

- `<local firmware repository>` - This is the path to the firmware folder on your local machine.
- `<user application folder>` - This is the path to the user application folder on your local machine.
- `(core|electron|p1|photon)` - Select the option from this list that matches the device for which you are compiling.
- `<user application>` - This will be the name of the resulting binary (i.e. `my_app.elf`).

> _**NOTE:** Special thanks to Kevin Sidwar for pioneering containerized builds in [his blog](https://www.kevinsidwar.com/iot/2018/3/1/local-particle-build-and-debug-through-docker-on-mac)._

## Native Dependency Installation

### Dependency List

- [`arm-none-eabi-gcc`](#gcc-for-arm-cortex-processors)
- [`make`](#gnu-make)
- [`dfu-util`](#device-firmware-upgrade-utilities)
- [`crc32`](#32-bit-cyclic-redundency-check-utility)
- [`xxd`](#xxd-hex-dump-utility)
- [Zadig (Windows only)](#zadig-windows-only)

### GCC for ARM Cortex processors

| Confirmed version | Download page |
|:-----------------:|:-------------:|
| 5.3.1 | [launchpad.net](https://launchpad.net/gcc-arm-embedded) |

The Core/Photon uses an ARM Cortex M3 CPU based microcontroller. All of the code is built around the GNU GCC toolchain offered and maintained by ARM.

The build requires version 5.3.1 20160307 or newer of ARM GCC and will print an error
message if the version is older than this.

| OS | Distribution | Package Manager | Command |
|:-- |:------------ |:--------------- |:------- |
| Linux | Ubuntu | apt-get | `sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa && sudo apt-get update && sudo apt-get install gcc-arm-embedded` |
| Linux | _(non-Ubuntu)_ | n/a | `# See detailed instructions below` |
| OSX | n/a | [Homebrew](http://brew.sh/) | `# See detailed instructions below` |
| Windows | n/a | n/a | `# See detailed instructions below` |

**Linux _(non-Ubuntu)_:**

Install from the download page.

**OSX:**

- `brew tap PX4/homebrew-px4`
- `brew update`
- copy/paste this in Terminal and press ENTER to create the proper Brew formula

  ```bash
  echo -e "gcc-arm-none-eabi-53.rb
  require 'formula'

  class GccArmNoneEabi53 < Formula
    homepage 'https://launchpad.net/gcc-arm-embdded'
    version '20160307'
    url 'https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q1-update/+download/gcc-arm-none-eabi-5_3-2016q1-20160330-mac.tar.bz2'
    sha256 '480843ca1ce2d3602307f760872580e999acea0e34ec3d6f8b6ad02d3db409cf'

    def install
      ohai 'Copying binaries...'
      system 'cp', '-rv', 'arm-none-eabi', 'bin', 'lib', 'share', "#{prefix}/"
    end
  end" > /usr/local/Homebrew/Library/Taps/px4/homebrew-px4/gcc-arm-none-eabi-53.rb
  ```

- install it!

  ```bash
  brew install gcc-arm-none-eabi-53
  ```

- confirm installation

  ```bash
  $ arm-none-eabi-gcc --version
  arm-none-eabi-gcc (GNU Tools for ARM Embedded Processors) 5.3.1 20160307 (release) [ARM/embedded-5-branch revision 234589]
  Copyright (C) 2015 Free Software Foundation, Inc.
  This is free software; see the source for copying conditions.  There is NO
  warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  ```

If you are upgrading an existing installation you will have to unlink and link your symblinks:

- `brew update`
- `brew install gcc-arm-none-eabi-53` (when you run this, it will tell you the following commands)
- `brew unlink gcc-arm-none-eabi-49` (example)
- `brew link --overwrite gcc-arm-none-eabi-53` (example)
- `arm-none-eabi-gcc --version` (should now say v5.3.1)

**Windows:**

Install from the download page.

### GNU Make

| Confirmed version | Download page |
|:-----------------:|:-------------:|
| 4.1 | [sourceforge.net](http://gnuwin32.sourceforge.net/packages/make.htm) |

In order to turn your source code into binaries, you will need a tool called `make`. Windows users need to explicitly install `make` on their machines. Make sure you can use it from the terminal window.

| OS | Distribution | Package Manager | Command |
|:-- |:------------ |:--------------- |:------- |
| Linux | Debian | apt-get | `sudo apt-get install make` |
| OSX | n/a | [Homebrew](http://brew.sh/) | `brew install make` |
| OSX | n/a | [Macports](http://www.macports.org) | `port install make` |
| Windows | n/a | n/a | `# See detailed instructions below` |

**Windows:**

Install from the download page.

### Device Firmware Upgrade Utilities

`dfu-util` - Device firmware update (DFU) USB programmer

| Confirmed version | Download page |
|:-----------------:|:-------------:|
| 0.8 | [sourceforge.net](http://dfu-util.sourceforge.net/) |

| OS | Distribution | Package Manager | Command |
|:-- |:------------ |:--------------- |:------- |
| Linux | Debian | apt-get | `sudo apt-get install dfu-util` |
| OSX | n/a | [Homebrew](http://brew.sh/) | `brew install dfu-util` |
| OSX | n/a | [Macports](http://www.macports.org) | `port install dfu-util` |
| Windows | n/a | n/a | `# See detailed instructions below` |

**Windows:**

Install from the download page.

### 32-bit Cyclic Redundency Check Utility

`crc32` - Calculates CRC sums of 32 bit length

| Confirmed version | Download page |
|:-----------------:|:-------------:|
| ? | N/A |

| OS | Distribution | Package Manager | Command |
|:-- |:------------ |:--------------- |:------- |
| Linux | Debian | apt-get | `sudo apt-get install libarchive-zip-perl` |
| OSX | n/a | n/a | `# Available by default` |
| Windows | n/a | n/a | `# See detailed instructions below` |

**Windows:**

Install `MinGW` and add it to your path.

### XXD Hex Dump Utility

`xxd` - tool to make (or reverse) a hex dump

| Confirmed version | Download page |
|:-----------------:|:-------------:|
| 1.10 | N/A |

| OS | Distribution | Package Manager | Command |
|:-- |:------------ |:--------------- |:------- |
| Linux | Debian | apt-get | `sudo apt-get install xxd` |
| OSX | n/a | n/a | `# Available by default` |
| Windows | n/a | n/a | `# See detailed instructions below` |

**Windows:**

Install `MinGW` and add it to your path.

### Zadig (Windows only)

Zadig - A Windows application that installs generic USB drivers

| Confirmed version | Download page |
|:-----------------:|:-------------:|
| N/A | [akeo.ie](http://zadig.akeo.ie/) |

In order for the device to show up on the dfu list, you need to replace the USB driver with Zadig.

For a tutorial on how to use Zadig, visit [community.particle.io](https://community.particle.io/t/tutorial-installing-dfu-driver-on-windows-24-feb-2015/3518).
