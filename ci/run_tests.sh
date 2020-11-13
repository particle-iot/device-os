#!/bin/bash
# ensure we are in the root
cd "$( dirname "${BASH_SOURCE[0]}" )/.."

./ci/enumerate_build_matrix.sh
