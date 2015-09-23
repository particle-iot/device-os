#!/usr/bin/env bash
_path=$HOME/.particle
mkdir -p $_path
cat <<EOS > ${_path}/particle.config.json
{
  "apiUrl": "https://api.particle.io",
  "access_token": "${SPARK_ACCESS_TOKEN}",
  "username": "${SPARK_USER}"
}
EOS
