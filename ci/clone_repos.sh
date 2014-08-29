#$/bin/bash

# $1 - branch name to clone

git clone --depth 50 --branch=$1 git://github.com/spark/core-common-lib.git
git clone --depth 50 --branch=$1 git://github.com/spark/core-communication-lib.git
