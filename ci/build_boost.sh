# Load config
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export BOOST_BUILD_USER_CONFIG=$DIR/user-config.jam

pushd $BOOST_ROOT
./bootstrap.sh
./b2  --debug-configuration --with-thread  --with-system --with-program_options --with-random  --threading=single toolset=gcc-arm
popd
