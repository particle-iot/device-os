echo 'hi'
# guaranteed_ram=4096
# available_ram=4100
# curl \
#   -u ${LIBRATO_USER}:${LIBRATO_ACCESS_TOKEN} \
#   -d 'source=core-firmware-travis-ci' \
#   -d 'gauges[0][name]=spark_dev.ci.firmware.memory.available' \
#   -d "gauges[0][value]=$available_ram" \
#   -d 'gauges[1][name]=spark_dev.ci.firmware.memory.available_minus_handshake' \
#   -d "gauges[1][value]=$available_ram_minus_handshake" \
#   -d 'gauges[2][name]=spark_dev.ci.firmware.memory.guanantee_headroom' \
#   -d "gauges[2][value]=$headroom" \
#   -d 'gauges[3][name]=spark_dev.ci.firmware.memory.guaranteed' \
#   -d "gauges[3][value]=$guaranteed_ram" \
#   -X POST \
#   https://metrics-api.librato.com/v1/metrics
