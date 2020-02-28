BOOST_VERSION=1_72_0

export BOOST_ROOT=$HOME/.ci/boost/boost_$BOOST_VERSION
export BOOST_LIBRARYDIR=$BOOST_ROOT/stage/lib
mkdir -p $BOOST_ROOT
test -d $BOOST_ROOT || ( echo "boost root $BOOST_ROOT not created." && exit 1)
test -f $BOOST_ROOT/INSTALL || wget --quiet https://dl.bintray.com/boostorg/release/${BOOST_VERSION//_/.}/source/boost_${BOOST_VERSION}.tar.gz -O - | tar -xz -C $BOOST_ROOT --strip-components 1
export DYLD_LIBRARY_PATH="${BOOST_LIBRARYDIR}:$DYLD_LIBRARY_PATH"
export LD_LIBRARY_PATH="${BOOST_LIBRARYDIR}:$LD_LIBRARY_PATH"
