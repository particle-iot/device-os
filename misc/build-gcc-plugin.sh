#!/bin/bash

set -e

check_defined() {
  [ ${!1+x} ] || ( echo "$1 is not defined" && exit 1 )
}

check_defined BOOST_ROOT
check_defined RAPIDJSON_PATH

this_dir=$(cd $(dirname "$0") && pwd)
plugin_dir=$this_dir/gcc-plugin

gcc_plugin_path=$(arm-none-eabi-gcc -print-file-name=plugin)

git submodule update --init
cd $plugin_dir

make -s release \
  GCC_PLUGIN_INCLUDE_PATH=$gcc_plugin_path/include \
  BOOST_INCLUDE_PATH=$BOOST_ROOT \
  BOOST_LIB_PATH=$BOOST_ROOT/stage/lib \
  RAPIDJSON_INCLUDE_PATH=$RAPIDJSON_PATH/include
