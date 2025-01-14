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
  # FIXME: delete build directory to save some space in CI context
  rm -rf ${build_directory}
  [[ ${result_code} -eq 0 ]]
  testcase "$2 ${@:3}"
}

MAKE=runmake

# define build matrix dimensions
# "" means execute execute the $MAKE command without that var specified
DEBUG_BUILD=( y n )
PLATFORM=( argon boron asom bsom b5som esomx p2 trackerm msom electron2 )
# All modules are now built by release scripts instead, skip
# Only building applications and tests here
# PLATFORM_BOOTLOADER=( argon boron asom bsom b5som tracker esomx p2 trackerm msom electron2 )
# PLATFORM_PREBOOTLOADER=( p2 trackerm msom )
PLATFORM_BOOTLOADER=()
PLATFORM_PREBOOTLOADER=()
APP=( "" product_id_and_version )
TEST=( wiring/api )

MODULAR_PLATFORM=( argon boron asom bsom b5som tracker esomx p2 trackerm msom electron2 )

filterPlatform PLATFORM
filterPlatform MODULAR_PLATFORM
filterPlatform PLATFORM_BOOTLOADER
filterPlatform PLATFORM_PREBOOTLOADER

echo "running matrix PLATFORM=$PLATFORM MODULAR_PLATFORM=$MODULAR_PLATFORM PLATFORM_BOOTLOADER=$PLATFORM_BOOTLOADER PLATFORM_PREBOOTLOADER=$PLATFORM_PREBOOTLOADER"

# Build the list of build jobs
BUILD_JOBS=()
# Just in case remove build/jobs folder
rm -rf build/jobs

# Build/install any necessary prerequisites first
# gcovr for coverage reports
if platform 'unit-test'; then
  ./ci/install_gcovr.sh
  testcase "0 PLATFORM=\"unit-test\" ci/install_gcovr.sh"
fi

# Boost for gcc or unit-test builds
if platform gcc || platform 'unit-test'; then
  source ci/install_boost.sh
  testcase "0 PLATFORM=\"gcc\" PLATFORM=\"unit-test\" ci/install_boost.sh"
  echo
  echo '-----------------------------------------------------------------------'
  ci/build_boost.sh
  testcase "0 PLATFORM=\"gcc\" PLATFORM=\"unit-test\" ci/build_boost.sh"
fi
echo $BOOST_ROOT

if [[ -f "${PWD}/.has_failures" ]]; then
  mkdir build/jobs
  cp "${PWD}/.has_failures" build/jobs/
fi

# Add GCC platform build
if platform gcc; then
  cmd="${MAKE} PLATFORM=\"gcc\""
  BUILD_JOBS+=("main ${#BUILD_JOBS[@]} ${cmd}")
fi

# Add unit tests to build jobs
if platform 'unit-test'; then
  cmd="PLATFORM=\"unit-test\" ../../ci/unit_tests.sh"
  BUILD_JOBS+=("ci ${#BUILD_JOBS[@]} ${cmd}")
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
    for app in "${APP[@]}"
    do
      if [[ "$app" = "" ]]; then
        cmd="${MAKE} DEBUG_BUILD=\"$db\" PLATFORM=\"$p\" COMPILE_LTO=\"n\""
        BUILD_JOBS+=("main ${#BUILD_JOBS[@]} ${cmd}")
      else
        cmd="${MAKE} DEBUG_BUILD=\"$db\" PLATFORM=\"$p\" COMPILE_LTO=\"n\" APP=\"$app\""
        BUILD_JOBS+=("main ${#BUILD_JOBS[@]} ${cmd}")
      fi
    done
  done
done

for p in "${PLATFORM_BOOTLOADER[@]}"
do
  cmd="${MAKE} PLATFORM=\"$p\""
  BUILD_JOBS+=("bootloader ${#BUILD_JOBS[@]} ${cmd}")
done

for p in "${PLATFORM_PREBOOTLOADER[@]}"
do
  cmd="${MAKE} PLATFORM=\"$p\""
  BUILD_JOBS+=("bootloader/prebootloader ${#BUILD_JOBS[@]} ${cmd}")
done

# enumerate the matrix, exit 1 if anything fails
for db in "${DEBUG_BUILD[@]}"
do
  for p in "${MODULAR_PLATFORM[@]}"
  do
    # Gen 3 overflows with modular DEBUG_BUILD=y, so skip those
    if [[ "$db" = "y" ]]; then
      if [[ "$p" = "argon" ]] || [[ "$p" = "boron" ]] || [[ "$p" = "asom" ]] || [[ "$p" = "bsom" ]] || [[ "$p" = "b5som" ]] || [[ "$p" = "tracker" ]] || [[ "$p" = "esomx" ]] || [[ "$p" = "p2" ]] || [[ "$p" = "trackerm" ]] || [[ "$p" = "msom" ]] || [[ "$p" = "electron2" ]]; then
        continue
      fi
    fi
    cmd="${MAKE} DEBUG_BUILD=\"$db\" PLATFORM=\"$p\" COMPILE_LTO=\"n\""
    BUILD_JOBS+=("modules ${#BUILD_JOBS[@]} ${cmd}")
  done
done

NPROC=$(nproc)

# Export necessary functions
export -f runBuildJob
export -f runmake
export -f testcase

echo
echo "Running ${#BUILD_JOBS[@]} build jobs on ${NPROC} cores"

# Silence an annoying notice
echo "will cite" | parallel --citation > /dev/null 2>&1

printf '%s\n' "${BUILD_JOBS[@]}"

printf '%s\n' "${BUILD_JOBS[@]}" | parallel --colsep ' ' -j "${NPROC}" runBuildJob

cd "${BUILD_JOBS_DIRECTORY}"
checkFailures || exit 1
