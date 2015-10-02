# ABOUT
# =====
#
# This CI test checks that freeMemoryAvailable() minus the (dynamically allocated)
# RAM required to do the secure handshake with the cloud is greater or equal to the
# user guaranteed RAM.
#
# ASSUMPTIONS + REQUIREMENTS
# ---
# - You have a core connected to the Spark Cloud.

# - You've set the correct the secure environment variables and added them to your .travis-ci.yml file like this
#
#     travis encrypt 'SPARK_ACCESS_TOKEN=<SOME_TOKEN>' --add
#     travis encrypt 'SPARK_USER=<SOME_USER>' --add
#     travis encrypt 'LIBRATO_USER=<SOME_USER>' --add
#     travis encrypt 'LIBRATO_ACCESS_TOKEN=<SOME_TOKEN>' --add
#

########
# VARIABLES YOU MIGHT NEED TO TWEAK
########

# Set a maximum amount of RAM
export guaranteed_ram=4096

# Set a how much the handshake takes
export handshake_ram=7896

# Set a metric prefix name for Librato/graphing
export metric_prefix=spark_dev

# Post to librato or not
export post_to_librato=true

# Jump into the CI dir
cd ci

# read in list of cores
. ./cores

core_name=${name['ci-mem-test']}


# Use particle-cli to compile a simple firmware that checks available ram
# TODO: THIS IS WHERE WE NEED TO USE THE `--latest` flag
particle compile firmware/memory_available.ino | tee compile_output.txt

# Parse binary name from CLI output
# would be nice if there were a `--format json` flag
bin_name=$(cat compile_output.txt | grep "firmware.*bin" | awk ' { print ( $(NF) ) }')

# Flash the firmware to core OTA style
echo "Flashing mem-test to $(core_name)"
particle flash $core_name $bin_name

# Wait 60 seconds for the flash to complete + core to reconnect;
# would be nice if this polled instead
sleep 60
echo "slept 60 seconds"

# Use Spark API to interogate core
particle get $core_name free_mem | tee variable_get_output.txt

# If it's an integer, ensure it's below the limit
# and report amount of headroom
export available_ram=`cat variable_get_output.txt`
if [[ $available_ram =~ ^[0-9]+$ ]]; then
  export available_ram_minus_handshake=$(( $available_ram - $handshake_ram ))
  export headroom=$(( $available_ram_minus_handshake - $guaranteed_ram ))
  if [[ $post_to_librato ]]; then
    echo "posting stats to librato"
    curl \
     -u ${LIBRATO_USER}:${LIBRATO_ACCESS_TOKEN} \
     -d 'source=core-firmware-travis-ci' \
     -d 'gauges[0][name]='$metric_prefix'.ci.firmware.memory.available' \
     -d "gauges[0][value]=$available_ram" \
     -d 'gauges[1][name]='$metric_prefix'.ci.firmware.memory.available_minus_handshake' \
     -d "gauges[1][value]=$available_ram_minus_handshake" \
     -d 'gauges[2][name]='$metric_prefix'.ci.firmware.memory.guanantee_headroom' \
     -d "gauges[2][value]=$headroom" \
     -d 'gauges[3][name]='$metric_prefix'.ci.firmware.memory.guaranteed' \
     -d "gauges[3][value]=$guaranteed_ram" \
     -X POST \
     https://metrics-api.librato.com/v1/metrics
  else
    echo "NOT posting stats to librato"
  fi

  if [[ $headroom -ge 0 ]]; then
    echo "available ram: $available_ram"
    echo "available ram minus handshake: $available_ram_minus_handshake"
    echo "guaranteed ram: $guaranteed_ram"
    echo "headroom: $headroom bytes of headroom after this commit, use it wisely grasshoppa."
    exit 0
  else
    echo "exceeded guaranteed ram by $headroom bytes."
    exit 1
  fi
else
  # note: this is going to yield false positives;
  # librato graph will annotate time series graph when this occurs
  echo "particle variable did not return an integer. Network connectivity problems?"
  exit 1;
fi
