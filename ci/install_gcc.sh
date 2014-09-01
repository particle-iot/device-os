#!/bin/bash
#
# Install gcc-4.8 as the default gcc

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test &&
sudo apt-get -qq update &&
sudo apt-get -qq install gcc-4.8 g++-4.8 &&
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 20 &&
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 20 &&
gcc --version
