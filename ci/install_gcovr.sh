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

python3 --version

# get the last version of pip compatible with python 3.5
# pip3 install --upgrade pip==20.3.4 

pip3 --version

pip3 install requests

# FIXME: Bug with the latest jinja2 (https://github.com/gcovr/gcovr/pull/576) so we'll downgrade for now.
#        Remove once this is fixed in a new release of gcovr (>5.0)
pip3 install jinja2==3.0.3

# install a specific gcovr version. May not work if that version isn't compatible with the pip3 version
pip3 install gcovr==5.0

# install latest stable gcovr version
# pip3 install gcovr

# install gcovr latest develop branch directly from github
# pip3 install git+https://github.com/gcovr/gcovr.git

gcovr --version
