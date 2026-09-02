#!/bin/bash

KERNEL_NAME=$(uname -s)

# Fail on errors
set -e

# MacOS Support
if [ "${KERNEL_NAME}" == "Darwin" ]; then
  brew install gcovr

# Debian Support
else
  (apt-get -qq update &&
  apt-get -qq install gcovr zlib1g-dev)
fi

gcovr --version
