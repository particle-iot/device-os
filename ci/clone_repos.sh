#$/bin/bash

# $1 - branch name to in core-common-lib
# $2 - branch name to clone in core-communication-lib

git clone --depth 50 --branch=$1 git://github.com/spark/core-common-lib.git
git clone --depth 50 --branch=$2 git://github.com/spark/core-communication-lib.git
