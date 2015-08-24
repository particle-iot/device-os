#!/bin/bash
############
# About
# ===
# Build matrix enumeration in bash.
# Stops executing and returns exit 1 if any of them fail
############

set -x # be noisy + log everything that is happening in the script

# set current working dir
cd main

# define build matrix dimensions
# "" means execute execute the make command without that var specified
DEBUG_BUILD=( y n )
PLATFORM=( core photon P1 )
COMPILE_LTO=( y n )
SPARK_CLOUD=( y n )
# TODO: Once FIRM-161 is fixed, change APP to this: APP=( "" tinker blank product_id_and_version )
APP=( "" tinker blank )

# enumerate the matrix, exit 1 if anything fails
for db in "${DEBUG_BUILD[@]}"
do
  for p in "${PLATFORM[@]}"
  do
    for c in "${COMPILE_LTO[@]}"
    do
      for sc in "${SPARK_CLOUD[@]}"
      do
        for app in "${APP[@]}"
        do
          if [[ "$app" = "" ]]; then
            make clean all DEBUG_BUILD="$db" PLATFORM="$p" COMPILE_LTO="$c" SPARK_CLOUD="$sc" > /dev/null 2>&1
            if [[ "$?" -eq 0 ]]; then
              echo "✓ SUCCESS"
            else
              echo "✗ FAILED"
              exit 1
            fi
          else
            make clean all DEBUG_BUILD="$db" PLATFORM="$p" COMPILE_LTO="$c" SPARK_CLOUD="$sc" APP="$app"  > /dev/null 2>&1
            if [[ "$?" -eq 0 ]]; then
              echo "✓ SUCCESS"
            else
              echo "✗ FAILED"
              exit 1
            fi
          fi
        done
      done
    done
  done
done
