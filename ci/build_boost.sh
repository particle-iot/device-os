pushd $BOOST_ROOT
./bootstrap.sh
./b2  --link=static --runtime-link=static --layout=tagged --with-system --with-program_options  threading=single
popd
