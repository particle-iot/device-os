#!/bin/bash

RAPIDJSON_VERSION=1.1.0

export RAPIDJSON_PATH=$HOME/.ci/rapidjson-$RAPIDJSON_VERSION
[ -d $RAPIDJSON_PATH ] || ( mkdir -p $RAPIDJSON_PATH && wget https://github.com/Tencent/rapidjson/archive/v$RAPIDJSON_VERSION.tar.gz -q -O - | tar -xz -C $RAPIDJSON_PATH --strip-components 1 )
