#!/bin/bash
############
# About
# ===
# Build matrix enumeration in bash.
# Stops executing and returns exit 1 if any of them fail
############

set -x # be noisy + log everything that is happening in the script

function runmake()
{
	make -s clean all $*
}

MAKE=runmake
GREEN="\033[32m"
RED="\033[31m"
NO_COLOR="\033[0m"

# define build matrix dimensions
# "" means execute execute the $MAKE command without that var specified
DEBUG_BUILD=( y n )
PLATFORM=( core photon P1 electron )
SPARK_CLOUD=( y n )
# TODO: Once FIRM-161 is fixed, change APP to this: APP=( "" tinker product_id_and_version )
APP=( "" tinker )
TEST=( wiring/api wiring/no_fixture )

MODULAR_PLATFORM=( photon P1 )

# set current working dir
cd main

# Newhal Build
echo
echo '-----------------------------------------------------------------------'
$MAKE  PLATFORM="newhal" COMPILE_LTO="n"
if [[ "$?" -eq 0 ]]; then
  echo "✓ SUCCESS"
else
  echo "✗ FAILED"
  exit 1
fi

# GCC Build
echo
echo '-----------------------------------------------------------------------'
$MAKE  PLATFORM=gcc
if [[ "$?" -eq 0 ]]; then
  echo "✓ SUCCESS"
else
  echo "✗ FAILED"
  exit 1
fi




# COMPILE_LTO required on the Core for wiring/no_fixture to fit
for t in "${TEST[@]}"
do
  for p in "${MODULAR_PLATFORM[@]}"
  do
    echo
    echo '-----------------------------------------------------------------------'
    $MAKE  PLATFORM="$p" COMPILE_LTO="n" TEST="$t"
    if [[ "$?" -eq 0 ]]; then
      echo -e "$GREEN ✓ SUCCESS $NO_COLOR"
    else
      echo -e "$RED ✗ FAILED $NO_COLOR"
      exit 1
    fi
  done
done

# enumerate the matrix, exit 1 if anything fails
for db in "${DEBUG_BUILD[@]}"
do
  for p in "${PLATFORM[@]}"
  do
    for sc in "${SPARK_CLOUD[@]}"
    do
      for app in "${APP[@]}"
      do
        # only do SPARK_CLOUD=n for core
        if [[ "$sc" = "n" ]] && [[ "$p" != "core" ]]; then
          continue
        fi
        c=n
        if [[ "$p" = "core" ]]; then
           c=y
        fi
        echo
        echo '-----------------------------------------------------------------------'
        if [[ "$app" = "" ]]; then
          $MAKE  DEBUG_BUILD="$db" PLATFORM="$p" COMPILE_LTO="$c" SPARK_CLOUD="$sc"
          if [[ "$?" -eq 0 ]]; then
            echo -e "$GREEN ✓ SUCCESS $NO_COLOR"
          else
            echo -e "$RED ✗ FAILED $NO_COLOR"
            exit 1
          fi
        else
          $MAKE  DEBUG_BUILD="$db" PLATFORM="$p" COMPILE_LTO="$c" SPARK_CLOUD="$sc" APP="$app"
          if [[ "$?" -eq 0 ]]; then
            echo -e "$GREEN ✓ SUCCESS $NO_COLOR"
          else
            echo -e "$RED ✗ FAILED $NO_COLOR"
            exit 1
          fi
        fi
      done
    done
  done
done

cd ../modules

# enumerate the matrix, exit 1 if anything fails
for db in "${DEBUG_BUILD[@]}"
do
  for p in "${MODULAR_PLATFORM[@]}"
  do
    echo
    echo '-----------------------------------------------------------------------'
    $MAKE  DEBUG_BUILD="$db" PLATFORM="$p" COMPILE_LTO="n"
    if [[ "$?" -eq 0 ]]; then
      echo -e "$GREEN ✓ SUCCESS $NO_COLOR"
    else
      echo -e "$RED ✗ FAILED $NO_COLOR"
      exit 1
    fi
  done
done

# Photon minimal build
echo
echo '-----------------------------------------------------------------------'
$MAKE  PLATFORM="photon" COMPILE_LTO="n" MINIMAL=y
if [[ "$?" -eq 0 ]]; then
  echo -e "$GREEN ✓ SUCCESS $NO_COLOR"
else
  echo -e "$RED ✗ FAILED $NO_COLOR"
  exit 1
fi
