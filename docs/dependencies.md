## 1. Download and Install Dependencies

Building the firmware locally requires these dependencies ot be installed:

1. [GCC for ARM Cortex processors](#1-gcc-for-arm-cortex-processors)
2. [Make](#2-make)
3. [Device Firmware Upgrade Utilities](#3-device-firmware-upgrade-utilities)
4. [Zatig](#4-zatig) (for Windows users only)
5. [Git](#5-git)
6. [Command line tools](#6-command-line-tools)

#### 1. GCC for ARM Cortex processors
The Core/Photon uses an ARM Cortex M3 CPU based microcontroller. All of the code is built around the GNU GCC toolchain offered and maintained by ARM.

The build requires version 5.3.1 20160307 or newer of ARM GCC and will print an error
message if the version is older than this.


**Linux and Windows**:
- Download and install version 5.3.1 from: https://launchpad.net/gcc-arm-embedded

**OS X** users can install the toolchain with [Homebrew](http://brew.sh/):
- `brew tap PX4/homebrew-px4`
- `brew update`
- copy/paste this in Terminal and press ENTER to create the proper Brew formula
```
echo -e "
require 'formula'

class GccArmNoneEabi53 < Formula
  homepage 'https://launchpad.net/gcc-arm-embdded'
  version '20160307'
  url 'https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q1-update/+download/gcc-arm-none-eabi-5_3-2016q1-20160330-mac.tar.bz2'
  sha256 '480843ca1ce2d3602307f760872580e999acea0e34ec3d6f8b6ad02d3db409cf'

  def install
    ohai 'Copying binaries...'
    system 'cp', '-rv', 'arm-none-eabi', 'bin', 'lib', 'share', \"#{prefix}/\"
  end
end" > /usr/local/Homebrew/Library/Taps/px4/homebrew-px4/gcc-arm-none-eabi-53.rb
```

- install it!
```
brew install gcc-arm-none-eabi-53
```
- `arm-none-eabi-gcc --version` (should now say v5.3.1)

If you are upgrading an existing installation you will have to unlink and link your symblinks:
- `brew update`
- `brew install gcc-arm-none-eabi-53` (when you run this, it will tell you the following commands)
- `brew unlink gcc-arm-none-eabi-49` (example)
- `brew link --overwrite gcc-arm-none-eabi-53` (example)
- `arm-none-eabi-gcc --version` (should now say v5.3.1)

#### 2. Make
In order to turn your source code into binaries, you will need a tool called `make`. Windows users need to explicitly install `make` on their machines. Make sure you can use it from the terminal window.

Download and install the latest version from: http://gnuwin32.sourceforge.net/packages/make.htm

#### 3. Device Firmware Upgrade Utilities
Install dfu-util 0.8. Mac users can install dfu-util with [Homebrew](http://brew.sh/) `brew install dfu-util` or [Macports](http://www.macports.org), Linux users may find it in their package manager, and everyone can get it from http://dfu-util.gnumonks.org/index.html

#### 4. Zadig
In order for the device to show up on the dfu list, you need to replace the USB driver with a utility called [Zadig](http://zadig.akeo.ie/). Here is a [tutorial](https://community.spark.io/t/tutorial-installing-dfu-driver-on-windows/3518) on using it. This is only required for Windows users.

#### 5. Git

Download and install Git: http://git-scm.com/

#### 6. Command line tools

On Windows, you'll need to install MinGW and have it in your path to make available some of the typical *nix command line tools. 

The tool `crc32` is also needed:
 - available in MinGW on Windows
 - available by default on OS X
 - linux users, please check with your package manager. On debian based systems it can be installed via `sudo apt-get install libarchive-zip-perl`


