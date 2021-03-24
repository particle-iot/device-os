#!/bin/bash
#
# Prerequisites:
#   git
#   python
#   pip
KERNEL_NAME=$(uname -s)

# Fail on errors
set -e

# MacOS Support
if [ "${KERNEL_NAME}" == "Darwin" ]; then
  brew install git python

# Debian Support
else
  (apt-get -qq update &&
  apt-get -qq install git python3-pip libxml2-dev libxslt-dev)
fi


pip3 install git+https://github.com/gcovr/gcovr.git
pip3 install requests

gcovr --version
