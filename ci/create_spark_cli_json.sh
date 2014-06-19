#!/usr/bin/env bash
_path=$HOME/.spark
mkdir $_path
cat <<EOS > ${_path}/spark.config.json
{
  "apiUrl": "https://api.spark.io",
  "access_token": "123",
  "username": "somedude"
}
EOS
