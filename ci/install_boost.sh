BOOST_VERSION=1_78_0

export BOOST_ROOT=$HOME/.ci/boost/boost_$BOOST_VERSION
export BOOST_LIBRARYDIR=$BOOST_ROOT/stage/lib
mkdir -p $BOOST_ROOT
test -d $BOOST_ROOT || ( echo "boost root $BOOST_ROOT not created." && exit 1)
test -f $BOOST_ROOT/INSTALL || wget --quiet https://spark-assets.s3.amazonaws.com/boost_${BOOST_VERSION}.tar.gz -O - | tar -xz -C $BOOST_ROOT --strip-components 1

if [[ $OSTYPE == darwin* ]]; then
  export DYLD_LIBRARY_PATH="${BOOST_LIBRARYDIR}:$DYLD_LIBRARY_PATH"
else
  export LD_LIBRARY_PATH="${BOOST_LIBRARYDIR}:$LD_LIBRARY_PATH"
fi
