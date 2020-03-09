toolset="gcc"
if [[ $OSTYPE == darwin* ]]; then
  toolset="darwin"
fi

pushd $BOOST_ROOT
./bootstrap.sh
./b2  --with-thread  --with-system --with-program_options --with-random --with-regex --threading=multi toolset=$toolset
popd
