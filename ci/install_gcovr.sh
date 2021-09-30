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

# upgrade pip to the latest stable version
pip3 install --upgrade pip
python3 --version

pip3 install requests
# install latest stable gcovr. Warning: may not support python3.5
pip3 install gcovr
# install gcovr latest develop branch
#pip3 install git+https://github.com/gcovr/gcovr.git
# FIXME: MarkupSafe is broken on python3.5
ln -s /usr/local/lib/python3.5/dist-packages/MarkupSafe-0.0.0.dist-info /usr/local/lib/python3.5/dist-packages/MarkupSafe-2.0.0.dist-info

gcovr --version
