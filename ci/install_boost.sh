BOOST_VERSION=1_59_0

# this was previously hardcoded to 1.59.0 in the path, replacing with a fast source
#test -f boost_$BOOST_VERSION.tar.gz || wget --quiet http://downloads.sourceforge.net/project/boost/boost/1.59.0/boost_$BOOST_VERSION.tar.gz
test -f boost_$BOOST_VERSION.tar.gz || wget --quiet https://s3.amazonaws.com/spark-assets/boost_1_59_0.tar.gz
export BOOST_HOME=$HOME/.ci/boost
export BOOST_ROOT=$BOOST_HOME/boost_$BOOST_VERSION
mkdir -p $BOOST_HOME
test -d $BOOST_ROOT || (
   tar zxf boost_$BOOST_VERSION.tar.gz
   mv boost_$BOOST_VERSION  $BOOST_HOME
)
test -d $BOOST_ROOT || ( echo "boost root $BOOST_ROOT not created." && exit 1)
export DYLD_LIBRARY_PATH="$BOOST_ROOT/stage/lib:$DYLD_LIBRARY_PATH"
export LD_LIBRARY_PATH="$BOOST_ROOT/stage/lib:$LD_LIBRARY_PATH"
