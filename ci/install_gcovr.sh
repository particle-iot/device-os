#!/bin/bash
#
# Prerequisites:
#   git
#   python
#   pip
KERNEL_NAME=$(uname -s)

# MacOS Support
if [ "${KERNEL_NAME}" == "Darwin" ]; then
  brew install git python || exit 1

# Debian Support
else
  (apt-get -qq update &&
  apt-get -qq install git python-pip) || exit 1
fi

# Install custom `gcovr` to support Coveralls output
pushd ~
git clone https://github.com/zfields/gcovr.git -b coveralls
cd gcovr
if [ "${KERNEL_NAME}" == "Darwin" ]; then
  pip3 install -e $(pwd)
  pip3 install requests
else
  pip install -e $(pwd)
  pip install requests
fi
popd

gcovr --version
