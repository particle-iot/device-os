pushd $BOOST_ROOT
./bootstrap.sh
./b2  link=static runtime-link=static --with-system --with-program_options  --threading=single
popd
