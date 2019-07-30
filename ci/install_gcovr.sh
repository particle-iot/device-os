#!/bin/bash
#
# Install gcovr-4.1 as the default gcovr
KERNEL_NAME=$(uname -s)

if [ "${KERNEL_NAME}" == "Darwin" ]; then
  brew install gcovr || exit 1
else
  (apt-get -qq update &&
  apt-get -qq install python-pip &&
  pip install gcovr==4.1) || exit 2
fi

gcovr --version
