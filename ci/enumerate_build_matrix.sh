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
  make -s all $*
}

export BUILD_JOBS_DIRECTORY="$PWD/build/jobs"

function runBuildJob()
{
  build_directory="${BUILD_JOBS_DIRECTORY}/$2"
  make_directory="$PWD/$1"
  build_log="${BUILD_JOBS_DIRECTORY}/$2.log"
  echo
  echo '-----------------------------------------------------------------------'
  echo "Running ${@:3} in $1, build directory ${build_directory}"
  mkdir -p "${build_directory}"
  cd "${BUILD_JOBS_DIRECTORY}"
  eval ${@:3} -C "${make_directory}" BUILD_PATH_BASE="${build_directory}" |& tee "${build_log}"
  result_code=${PIPESTATUS[0]}
  # I didn't want to deal with another build type, so we are specifically dealing
  # with newhal build here
  if echo "${@:3}" | grep --quiet "newhal"; then
    HAS_NO_SECTIONS=$(grep 'has no sections' "${build_log}")
    [[ ! -z "${HAS_NO_SECTIONS}" ]] || [[ ${result_code} -eq 0 ]]
    result_code=$?
  fi
  [[ ${result_code} -eq 0 ]]
  testcase
}

MAKE=runmake

# define build matrix dimensions
# "" means execute execute the $MAKE command without that var specified
DEBUG_BUILD=( y n )
PLATFORM=( core photon p1 electron xenon argon boron xsom asom bsom )
# P1 bootloader built with gcc 4.8.4 doesn't fit flash, disabling for now
PLATFORM_BOOTLOADER=( core photon electron xenon argon boron xsom asom bsom )
SPARK_CLOUD=( y n )
APP=( "" tinker product_id_and_version)
TEST=( wiring/api wiring/no_fixture )

MODULAR_PLATFORM=( photon p1 electron xenon argon boron xsom asom bsom )

filterPlatform PLATFORM
filterPlatform MODULAR_PLATFORM
filterPlatform PLATFORM_BOOTLOADER

echo "running matrix PLATFORM=$PLATFORM MODULAR_PLATFORM=$MODULAR_PLATFORM PLATFORM_BOOTLOADER=$PLATFORM_BOOTLOADER"

# Build the list of build jobs
BUILD_JOBS=()
# Just in case remove build/jobs folder
rm -rf build/jobs

# Build any necessary prerequisites first
# Only build Boost if gcc is being built separately without unit tests
if platform gcc; then
  source ci/install_boost.sh
  if ! platform 'unit-test'; then
    echo
    echo '-----------------------------------------------------------------------'
    ci/build_boost.sh
    testcase
  fi
  cmd="${MAKE} PLATFORM=gcc"
  BUILD_JOBS+=("main ${#BUILD_JOBS[@]} ${cmd}")
fi

# Newhal Build
if platform newhal; then
  cmd="${MAKE} PLATFORM=\"newhal\" COMPILE_LTO=\"n\""
  BUILD_JOBS+=("main ${#BUILD_JOBS[@]} ${cmd}")
fi

# COMPILE_LTO required on the Core for wiring/no_fixture to fit
for t in "${TEST[@]}"
do
  for p in "${MODULAR_PLATFORM[@]}"
  do
    cmd="${MAKE} PLATFORM=\"$p\" COMPILE_LTO=\"n\" TEST=\"$t\""
    BUILD_JOBS+=("main ${#BUILD_JOBS[@]} ${cmd}")
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
        if [[ "$app" = "" ]]; then
          cmd="${MAKE} DEBUG_BUILD=\"$db\" PLATFORM=\"$p\" COMPILE_LTO=\"$c\" SPARK_CLOUD=\"$sc\""
          BUILD_JOBS+=("main ${#BUILD_JOBS[@]} ${cmd}")
        else
          cmd="${MAKE} DEBUG_BUILD=\"$db\" PLATFORM=\"$p\" COMPILE_LTO=\"$c\" SPARK_CLOUD=\"$sc\" APP=\"$app\""
          BUILD_JOBS+=("main ${#BUILD_JOBS[@]} ${cmd}")
        fi
      done
    done
  done
done

for p in "${PLATFORM_BOOTLOADER[@]}"
do
  cmd="${MAKE} PLATFORM=\"$p\""
  BUILD_JOBS+=("bootloader ${#BUILD_JOBS[@]} ${cmd}")
done

# enumerate the matrix, exit 1 if anything fails
for db in "${DEBUG_BUILD[@]}"
do
  for p in "${MODULAR_PLATFORM[@]}"
  do
    # Gen 3 and Photon overflow with modular DEBUG_BUILD=y, so skip those
    if [[ "$db" = "y" ]]; then
      if [[ "$p" = "photon" ]] || [[ "$p" = "p1" ]] || [[ "$p" = "xenon" ]] || [[ "$p" = "argon" ]] || [[ "$p" = "boron" ]] || [[ "$p" = "xsom" ]] || [[ "$p" = "asom" ]] || [[ "$p" = "bsom" ]]; then
        continue
      fi
    fi
    cmd="${MAKE} DEBUG_BUILD=\"$db\" PLATFORM=\"$p\" COMPILE_LTO=\"n\""
    BUILD_JOBS+=("modules ${#BUILD_JOBS[@]} ${cmd}")
  done
done

# Photon minimal build
if platform photon; then
  cmd="${MAKE} PLATFORM=\"photon\" COMPILE_LTO=\"n\" MINIMAL=y"
  BUILD_JOBS+=("modules ${#BUILD_JOBS[@]} ${cmd}")
fi

NPROC=$(nproc)

# Export necessary functions
export -f runBuildJob
export -f runmake
export -f testcase

echo
echo "Running ${#BUILD_JOBS[@]} build jobs on ${NPROC} cores"

# Silence an annoying notice
echo "will cite" | parallel --citation > /dev/null 2>&1

printf '%s\n' "${BUILD_JOBS[@]}" | parallel --colsep ' ' -j "${NPROC}" runBuildJob

cd "${BUILD_JOBS_DIRECTORY}"

checkFailures || exit 1
