# Set a maximum amount of RAM
export guaranteed_ram=4096
export handshake_ram=7896

# Jump into the CI dir
cd ci

# Use spark-cli to compile a simple firmware that checks available ram
spark compile firmware/memory_available.ino | tee compile_output.txt

# Parse binary name from CLI output
# would be nice if there were a `--format json` flag
bin_name=$(cat compile_output.txt | grep "firmware.*bin" | awk ' { print ( $(NF) ) }')

# Flash the firmware to core OTA style
spark flash joe_prod_core2 $bin_name

# Wait 60 seconds for the flash to complete + core to reconnect;
# would be nice if this polled instead
sleep 60
echo "slept 60 seconds"

# Use Spark API to interogate core
spark get joe_prod_core2 free_mem | tee variable_get_output.txt

# If it's an integer, ensure it's below the limit
# and report amount of headroom
export available_ram=`cat variable_get_output.txt`
if [[ $available_ram =~ ^[0-9]+$ ]]; then
  export available_ram_minus_handshake=$(( $available_ram - $handshake_ram ))
  export headroom=$(( $available_ram_minus_handshake - $guaranteed_ram ))
  if [[ $headroom -ge 0 ]]; then
    echo "$headroom bytes of headroom after this commit, use it wisely grasshoppa."
    exit 0
  else
    echo "exceeded guaranteed ram by $headroom bytes."
    exit 1
  fi
else
  # note: this is going to yield false positives;
  # librato graph will annotate time series graph when this occurs
  echo "spark variable did not return an integer. Network connectivity problems?"
  exit 1;
fi
