#!/bin/bash
cd "$( dirname "${BASH_SOURCE[0]}" )/.."
source ./ci/install_boost.sh
./ci/unit_tests.sh
./ci/build_boost.sh
./ci/enumerate_build_matrix.sh
