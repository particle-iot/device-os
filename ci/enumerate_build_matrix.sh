#!/bin/bash
############
# About
# ===
# Build matrix enumeration in bash.
# Stops executing and returns exit 1 if any of them fail
############

. ./ci/functions.sh

set -x # be noisy + log everything that is happening in the script

function runmake()
{
	make -s clean all $*
}

MAKE=runmake

# define build matrix dimensions
# "" means execute execute the $MAKE command without that var specified
DEBUG_BUILD=( y n )
PLATFORM=( core photon P1 electron )
# P1 bootloader built with gcc 4.8.4 doesn't fit flash, disabling for now
PLATFORM_BOOTLOADER=( core photon electron )
SPARK_CLOUD=( y n )
APP=( "" tinker product_id_and_version)
TEST=( wiring/api wiring/no_fixture )

MODULAR_PLATFORM=( photon P1 electron)

filterPlatform PLATFORM 
filterPlatform MODULAR_PLATFORM 
filterPlatform PLATFORM_BOOTLOADER

echo "runing matrix PLATFORM=$PLATFORM MODULAR_PLATFORM=$MODULAR_PLATFORM PLATFORM_BOOTLOADER=$PLATFORM_BOOTLOADER"


# set current working dir
cd main

# Newhal Build
if platform newhal; then
echo
echo '-----------------------------------------------------------------------'
$MAKE  PLATFORM="newhal" COMPILE_LTO="n"
HAS_NO_SECTIONS=`echo $? | grep 'has no sections'`;
[[ ! -z HAS_NO_SECTIONS || "$?" -eq 0 ]]; 
testcase
fi

# GCC Build
if platform gcc; then
echo
echo '-----------------------------------------------------------------------'
$MAKE  PLATFORM=gcc
testcase
fi


# COMPILE_LTO required on the Core for wiring/no_fixture to fit
for t in "${TEST[@]}"
do
  for p in "${MODULAR_PLATFORM[@]}"
  do
    echo
    echo '-----------------------------------------------------------------------'
    $MAKE  PLATFORM="$p" COMPILE_LTO="n" TEST="$t"
	testcase
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
	   # core debug build overflows at present
           if [[ "$db" = "y" ]]; then
               continue
        fi
        fi
        echo
        echo '-----------------------------------------------------------------------'
        if [[ "$app" = "" ]]; then
          $MAKE  DEBUG_BUILD="$db" PLATFORM="$p" COMPILE_LTO="$c" SPARK_CLOUD="$sc"
        else
          $MAKE  DEBUG_BUILD="$db" PLATFORM="$p" COMPILE_LTO="$c" SPARK_CLOUD="$sc" APP="$app"
          fi
		testcase
      done
    done
  done
done

cd ../bootloader
for p in "${PLATFORM_BOOTLOADER[@]}"
do
  echo
  echo '-----------------------------------------------------------------------'
  $MAKE PLATFORM="$p"
  testcase
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
	testcase
  done
done

# Photon minimal build
if platform photon; then
echo
echo '-----------------------------------------------------------------------'
$MAKE  PLATFORM="photon" COMPILE_LTO="n" MINIMAL=y
	testcase
fi

checkFailures || exit 1
