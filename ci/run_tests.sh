#!/bin/bash
# ensure we are in the root
cd "$( dirname "${BASH_SOURCE[0]}" )/.."

. ./ci/functions.sh

cmd=""

if contains "${BUILD_PLATFORM[*]}" unit-test; then
	( source ./ci/install_gcovr.sh
	source ./ci/install_boost.sh
	./ci/build_boost.sh &&
	cp -r ${BOOST_ROOT}/boost/ /usr/include/ &&
	./ci/unit_tests.sh ) || die
fi

./ci/enumerate_build_matrix.sh
