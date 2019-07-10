# Usage: DEVICE_ID=123412341234123412341234 PORT=$port4p ./upgrade-downgrade.sh
# set globals
: ${DEVICE_ID:?"REQUIRED"}
: ${PORT:?"REQUIRED"}
# or hardcode it here
#export DEVICE_ID="123412341234123412341234"
#
# install
# requires Bash 4.x for associative arrays, see upgrade instructions here:
# http://clubmate.fi/upgrade-to-bash-4-in-mac-os-x/
#!/usr/bin/env bash
#

# It is assumed that 0.5.3-rc.1 includes feature/full-deps-validation
# It is assumed that 0.6.0-rc.1 includes feature/full-deps-validation

export PHOTON_TKR_049="https://github.com/particle-iot/device-os/releases/download/v0.4.9-rc.3/tinker-v0.4.9-photon.bin"
export PHOTON_BOOT_054="https://github.com/particle-iot/device-os/releases/download/v0.5.4/bootloader-0.5.4-photon.bin"
export PHOTON_SP1_050="https://github.com/particle-iot/device-os/releases/download/v0.5.0/system-part1-0.5.0-photon.bin"
export PHOTON_SP2_050="https://github.com/particle-iot/device-os/releases/download/v0.5.0/system-part2-0.5.0-photon.bin"
export PHOTON_SP1_050_MOD=13
export PHOTON_SP2_050_MOD=13
export PHOTON_SP1_051="https://github.com/particle-iot/device-os/releases/download/v0.5.1/system-part1-0.5.1-photon.bin"
export PHOTON_SP2_051="https://github.com/particle-iot/device-os/releases/download/v0.5.1/system-part2-0.5.1-photon.bin"
export PHOTON_SP1_051_MOD=15
export PHOTON_SP2_051_MOD=15
export PHOTON_SP1_052="https://github.com/particle-iot/device-os/releases/download/v0.5.2/system-part1-0.5.2-photon.bin"
export PHOTON_SP2_052="https://github.com/particle-iot/device-os/releases/download/v0.5.2/system-part2-0.5.2-photon.bin"
export PHOTON_SP1_052_MOD=17
export PHOTON_SP2_052_MOD=17
export PHOTON_SP1_053_RC1="https://github.com/particle-iot/device-os/releases/download/v0.5.3-rc.1/system-part1-0.5.3-rc.1-photon.bin"
export PHOTON_SP2_053_RC1="https://github.com/particle-iot/device-os/releases/download/v0.5.3-rc.1/system-part2-0.5.3-rc.1-photon.bin"
export PHOTON_SP1_053_RC1_MOD=18
export PHOTON_SP2_053_RC1_MOD=18
export PHOTON_SP1_055="https://github.com/particle-iot/device-os/releases/download/v0.5.5/system-part1-0.5.5-photon.bin"
export PHOTON_SP2_055="https://github.com/particle-iot/device-os/releases/download/v0.5.5/system-part2-0.5.5-photon.bin"
export PHOTON_SP1_055_MOD=23
export PHOTON_SP2_055_MOD=23
export PHOTON_SP1_060_RC1="https://github.com/particle-iot/device-os/releases/download/v0.6.0-rc.1/system-part1-0.6.0-rc.1-photon.bin"
export PHOTON_SP2_060_RC1="https://github.com/particle-iot/device-os/releases/download/v0.6.0-rc.1/system-part2-0.6.0-rc.1-photon.bin"
export PHOTON_SP1_060_RC1_MOD=100
export PHOTON_SP2_060_RC1_MOD=100
export PHOTON_SP1_061_RC1="https://github.com/particle-iot/device-os/releases/download/v0.6.1-rc.1/system-part1-0.6.1-rc.1-photon.bin"
export PHOTON_SP2_061_RC1="https://github.com/particle-iot/device-os/releases/download/v0.6.1-rc.1/system-part2-0.6.1-rc.1-photon.bin"
export PHOTON_SP1_061_RC1_MOD=103
export PHOTON_SP2_061_RC1_MOD=103
export PHOTON_SP1_061_RC2="https://github.com/particle-iot/device-os/releases/download/v0.6.1-rc.2/system-part1-0.6.1-rc.2-photon.bin"
export PHOTON_SP2_061_RC2="https://github.com/particle-iot/device-os/releases/download/v0.6.1-rc.2/system-part2-0.6.1-rc.2-photon.bin"
export PHOTON_SP1_061_RC2_MOD=104
export PHOTON_SP2_061_RC2_MOD=104
export PHOTON_SP1_062="https://github.com/particle-iot/device-os/releases/download/v0.6.2/system-part1-0.6.2-photon.bin"
export PHOTON_SP2_062="https://github.com/particle-iot/device-os/releases/download/v0.6.2/system-part2-0.6.2-photon.bin"
export PHOTON_SP1_062_MOD=108
export PHOTON_SP2_062_MOD=108
export PHOTON_SP1_063="https://github.com/particle-iot/device-os/releases/download/v0.6.3/system-part1-0.6.3-photon.bin"
export PHOTON_SP2_063="https://github.com/particle-iot/device-os/releases/download/v0.6.3/system-part2-0.6.3-photon.bin"
export PHOTON_SP1_063_MOD=109
export PHOTON_SP2_063_MOD=109
export PHOTON_SP1_070_RC2="https://github.com/particle-iot/device-os/releases/download/v0.7.0-rc.2/system-part1-0.7.0-rc.2-photon.bin"
export PHOTON_SP2_070_RC2="https://github.com/particle-iot/device-os/releases/download/v0.7.0-rc.2/system-part2-0.7.0-rc.2-photon.bin"
export PHOTON_SP1_070_RC2_MOD=201
export PHOTON_SP2_070_RC2_MOD=201
export PHOTON_SP1_070="https://github.com/particle-iot/device-os/releases/download/v0.7.0/system-part1-0.7.0-photon.bin"
export PHOTON_SP2_070="https://github.com/particle-iot/device-os/releases/download/v0.7.0/system-part2-0.7.0-photon.bin"
export PHOTON_SP1_070_MOD=207
export PHOTON_SP2_070_MOD=207
export PHOTON_SP1_080_RC3="https://github.com/particle-iot/device-os/releases/download/v0.8.0-rc.3/system-part1-0.8.0-rc.3-photon.bin"
export PHOTON_SP2_080_RC3="https://github.com/particle-iot/device-os/releases/download/v0.8.0-rc.3/system-part2-0.8.0-rc.3-photon.bin"
export PHOTON_SP1_080_RC3_MOD=302
export PHOTON_SP2_080_RC3_MOD=302
export PHOTON_SP1_100="https://github.com/particle-iot/device-os/releases/download/v1.0.0/system-part1-1.0.0-photon.bin"
export PHOTON_SP2_100="https://github.com/particle-iot/device-os/releases/download/v1.0.0/system-part2-1.0.0-photon.bin"
export PHOTON_SP1_100_MOD=1000
export PHOTON_SP2_100_MOD=1000
export PHOTON_SP1_101="https://github.com/particle-iot/device-os/releases/download/v1.0.1/system-part1-1.0.1-photon.bin"
export PHOTON_SP2_101="https://github.com/particle-iot/device-os/releases/download/v1.0.1/system-part2-1.0.1-photon.bin"
export PHOTON_SP1_101_MOD=1002
export PHOTON_SP2_101_MOD=1002
export PHOTON_SP1_110="https://github.com/particle-iot/device-os/releases/download/v1.1.0/photon-system-part1@1.1.0.bin"
export PHOTON_SP2_110="https://github.com/particle-iot/device-os/releases/download/v1.1.0/photon-system-part2@1.1.0.bin"
export PHOTON_SP1_110_MOD=1102
export PHOTON_SP2_110_MOD=1102
export PHOTON_SP1_120_RC1="https://github.com/particle-iot/device-os/releases/download/v1.2.0-rc.1/photon-system-part1@1.2.0-rc.1.bin"
export PHOTON_SP2_120_RC1="https://github.com/particle-iot/device-os/releases/download/v1.2.0-rc.1/photon-system-part2@1.2.0-rc.1.bin"
export PHOTON_SP1_120_RC1_MOD=1202
export PHOTON_SP2_120_RC1_MOD=1202
export PHOTON_SP1_121_RC1="https://github.com/particle-iot/device-os/releases/download/v1.2.1-rc.1/photon-system-part1@1.2.1-rc.1.bin"
export PHOTON_SP2_121_RC1="https://github.com/particle-iot/device-os/releases/download/v1.2.1-rc.1/photon-system-part2@1.2.1-rc.1.bin"
export PHOTON_SP1_121_RC1_MOD=1210
export PHOTON_SP2_121_RC1_MOD=1210
export PHOTON_SP1_121_RC2="https://github.com/particle-iot/device-os/releases/download/v1.2.1-rc.2/photon-system-part1@1.2.1-rc.2.bin"
export PHOTON_SP2_121_RC2="https://github.com/particle-iot/device-os/releases/download/v1.2.1-rc.2/photon-system-part2@1.2.1-rc.2.bin"
export PHOTON_SP1_121_RC2_MOD=1211
export PHOTON_SP2_121_RC2_MOD=1211
export PHOTON_SP1_121="https://github.com/particle-iot/device-os/releases/download/v1.2.1/photon-system-part1@1.2.1.bin"
export PHOTON_SP2_121="https://github.com/particle-iot/device-os/releases/download/v1.2.1/photon-system-part2@1.2.1.bin"
export PHOTON_SP1_121_MOD=1213
export PHOTON_SP2_121_MOD=1213

# instead of these set options,
#   set script options
#   set -e # fail on any error
#   set -x # be verbose as fuck
# we are going for the 3 fingered claw approach (try <task>)
yell() { echo "$0: $*" >&2; }
die() { yell "$*"; exit 111; }
try() { "$@" || die "cannot $*"; }

declare -gA _inspect=()

# Finds and puts device in dfu mode automatically, must only have one device connected.
# or select a specific device with a specified $PORT
dfu() {
  local x=`compgen -f -- "/dev/cu.usbmodem"`;
  if [ -z "${x}" ]; then
    echo "No USB device found";
  else if [ ! -z "${PORT}" ]; then
      eval $(stty -f ${PORT} 14400);
    else
      eval $(stty -f ${x} 14400);
    fi
  fi
  return 0;
}
check_bash_version() {
  if  [[ $BASH_VERSION == 4.* ]] || [[ $BASH_VERSION == 5.* ]] ;
  then
      echo "Bash $BASH_VERSION found! let's do this ~_~";
  else
      echo "Bash >= 4.x required for associative arrays! See upgrade instructions here:"\
           "http://clubmate.fi/upgrade-to-bash-4-in-mac-os-x/";
      exit 1;
  fi
}
# Flash Photon System Part 1
dfu6part1() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo flashing $1
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  dfu-util -d 2b04:d006 -a 0 -s 0x8020000 -D $1
}
# Flash Photon System Part 2
dfu6part2() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo flashing $1
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  dfu-util -d 2b04:d006 -a 0 -s 0x8060000 -D $1
}
# Flash Photon User Part
dfu6user() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo flashing user image $1
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  dfu-util -d 2b04:d006 -a 0 -s 0x80a0000 -D $1
}
dfu6factory() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo flashing factory image $1
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  dfu-util -d 2b04:d006 -a 0 -s 0x80e0000 -D $1
}
dfu_system() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo flashing system parts $1 $2
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  if [[ $2 == photon ]] ;
  then
    dfu6part1 "system-part1-"$1"-photon.bin"
    dfu6part2 "system-part2-"$1"-photon.bin"
  else
    if [[ $2 == p1 ]] ;
    then
      dfu8part1 "system-part1-"$1"-p1.bin"
      dfu8part2 "system-part2-"$1"-p1.bin"
    fi
  fi
}
ymodem_flash() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo flashing $1
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  # echo | particle flash --serial $1 --port ${PORT}
  particle flash --serial $1 --port ${PORT} --yes
}
ymodem_part() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo flashing system part$1 $2 $3
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  ymodem_flash "system-part${1}-${2}-${3}.bin"
}
ymodem_boot() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo flashing bootloader $1 $2
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  ymodem_flash "bootloader-${1}-${2}.bin"
  sleep 5
}
serial_inspect() {
  local _result=$(particle serial inspect --port ${PORT} 2>/dev/null|sed -En 's/^.*System\ module \#([0-9]+)\ \-\ version\ ([0-9]+).*location.*$/\1 \2/p')
  unset ${_inspect}
  if [ -n "${_result}" ]; then
    while IFS=' ' read part vers; do
      _inspect["${part}"]=${vers}
    done <<<"${_result}"
    return 0
  fi
  return 1
}

compare_system_version() {
  echo "system-part${1} ${_inspect[${1}]} <> ${2} ?"
  [[ "${_inspect[${1}]}" = "${2}" ]]
  return
}

get_photon_dct() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo retrieving photon dct memory
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  dfu-util -d 2b04:d006 -a 0 -s 0x8004000:0x8000 -U $1
}
# need a quick serial terminal?
usb() {
  x=`compgen -f -- "/dev/cu.usbmodem"`;
  if [ -z "$x" ]; then
    echo "No USB device found";
  else
    eval $(screen $x);
  fi
}
# puts photon/p1/electron/core in Listening Mode
ymodem() {
  eval $(stty -f ${PORT} 28800);
  # x=`compgen -f -- "/dev/tty.usbmodem"`;
  # if [ -z "$x" ]; then
  #   echo "No USB device found";
  # else
  #   eval $(stty -f $x 28800);
  # fi
}
# alias for opening a new tab, killscreen, then entering listening mode, then connecting a screen session
killscreen_new_window() {
  #!/bin/sh
  pwd=`pwd`
  osascript -e "tell application \"Terminal\"" \
    -e "tell application \"System Events\" to keystroke \"t\" using {command down}" \
    -e "do script \"cd $pwd; clear; killscreen\" in front window" \
    -e "do script \"echo "done"\" in front window" \
    -e "end tell"
    > /dev/null
    #-e "tell application \"System Events\" to keystroke \"w\" using {command down}" \
}
# kill all screen sessions
killscreen () {
    for session in $(screen -ls | grep -o '[0-9]\{3,\}')
    do
        screen -S "${session}" -X quit;
    done
}
# enter DFU mode
enter_dfu_mode() {
  killscreen &> /dev/null
  try dfu
  sleep 2 # req. to wait for device to respond to linerate change
}
enter_ymodem() {
  killscreen &> /dev/null
  try ymodem
  sleep 5 # req. to wait for device to respond to linerate change
}
# kicks photon out of dfu mode
exit_dfu_mode() {
  dfu-util -d 2b04:d006 -a 0 -s 0x080A0000:leave -D /dev/null &> /dev/null
  sleep 5
}
set_country() {
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  echo setting country code $1
  echo =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  if [[ $1 == JP2 ]] ; then
    # set to japan
    echo -n "JP2" > JP2
    dfu-util -d 2b04:d006 -a 1 -s 1758 -D JP2
  else
    if [[ $1 == US4 ]] ; then
      # set to usa
      echo -n "US4" > US4
      dfu-util -d 2b04:d006 -a 1 -s 1758 -D US4
    else
      if [[ $1 == GB0 ]] ; then
        # set to uk
        echo -n "GB0" > GB0
        dfu-util -d 2b04:d006 -a 1 -s 1758 -D GB0
      else
        if [[ $1 == DEFAULT ]] ; then
          # reset to default
          echo "0: 00000000" | xxd -r > DEF
          dfu-util -d 2b04:d006 -a 1 -s 1758 -D def
        fi
      fi
    fi
  fi
}

# Rules:
#
# - if up/downgrading in correct order, next test needs to start with the up/downgrade version
# - if up/downgrading in rejected order, next test needs to start with the version
# - add tests to the end, try to cover these cases:
#     up, part2, part1, rejected
#     up, part1, part2, accepted
#     down, part1, part2, rejected
#     down, part2, part1, accepted

# +----+--------+------------+----------------------+--------------+--------------------+---------+
# |  # |  dev   |  version   | up/downgrade version |    order     |      flashed       | status  |
# +----+--------+------------+----------------------+--------------+--------------------+---------+
# |  0 | photon | 0.5.2      |           initial setup             | yes                | ok      |"
#
# |  1 | photon | 0.5.3-rc.1 | 0.5.2                | part1, part2 | no, part1 rejected | ok      |
# |  2 | photon | 0.5.3-rc.1 | 0.5.2                | part2, part1 | yes                | ok      |
# |  3 | photon | 0.5.2      | 0.5.0                | part2, part1 | yes                | ok      |
# |  4 | photon | 0.5.0      | 0.5.2                | part1, part2 | yes                | ok      |
# |  5 | photon | 0.5.2      | 0.5.3-rc.1           | part1, part2 | yes                | ok      |
# |  6 | photon | 0.5.3-rc.1 | 0.5.3-rc.1           | part1, part2 | yes                | ok      |
# |  7 | photon | 0.5.3-rc.1 | 0.5.3-rc.1           | part2, part1 | yes                | ok      |
# |  8 | photon | 0.5.3-rc.1 | 0.6.0-rc.1           | part1, part2 | yes                | ok      |
# |  9 | photon | 0.6.0-rc.1 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok      |
# | 10 | photon | 0.6.0-rc.1 | 0.5.3-rc.1           | part2, part1 | yes                | ok      |
# | 11 | photon | 0.5.3-rc.1 | 0.6.0-rc.1           | part2, part1 | no, part2 rejected | ok      |
# | 12 | photon | 0.5.3-rc.1 | 0.5.2                | part2, part1 | yes                | ok      |
# | 13 | photon | 0.5.2      | 0.5.0                | part2, part1 | yes                | ok-ish  | 0.5.2 still on device after
#
# | 14 | photon | 0.5.2      | 0.6.1-rc.1           | part1, part2 | yes                | ok      |
# | 15 | photon | 0.6.1-rc.1 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok      |
# | 16 | photon | 0.6.1-rc.1 | 0.5.3-rc.1           | part2, part1 | yes                | ok      |
# | 17 | photon | 0.5.3-rc.1 | 0.6.1-rc.1           | part2, part1 | no, part2 rejected | ok      |
# | 18 | photon | 0.5.3-rc.1 | 0.6.1-rc.1           | part1, part2 | yes                | ok      |
#
# | 19 | photon | 0.5.2      | 0.6.1-rc.2           | part1, part2 | yes                | ok      |
# | 20 | photon | 0.6.1-rc.2 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok      |
# | 21 | photon | 0.6.1-rc.2 | 0.5.3-rc.1           | part2, part1 | yes                | ok      |
# | 22 | photon | 0.5.3-rc.1 | 0.6.1-rc.2           | part2, part1 | no, part2 rejected | ok      |
# | 23 | photon | 0.5.3-rc.1 | 0.6.1-rc.2           | part1, part2 | yes                | ok      |
#
# | 24 | photon | 0.5.2      | 0.6.2                | part1, part2 | yes                | ok      |
# | 25 | photon | 0.6.2      | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok      |
# | 26 | photon | 0.6.2      | 0.5.3-rc.1           | part2, part1 | yes                | ok      |
# | 27 | photon | 0.5.3-rc.1 | 0.6.2                | part2, part1 | no, part2 rejected | ok      |
# | 28 | photon | 0.5.3-rc.1 | 0.6.2                | part1, part2 | yes                | ok      |
#
# | 29 | photon | 0.5.2      | 0.7.0-rc.2           | part1, part2 | yes                | ok      |
# | 30 | photon | 0.7.0-rc.2 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok      |
# | 31 | photon | 0.7.0-rc.2 | 0.5.3-rc.1           | part2, part1 | yes                | ok      |
# | 32 | photon | 0.5.3-rc.1 | 0.7.0-rc.2           | part2, part1 | no, part2 rejected | ok      |
# | 33 | photon | 0.5.3-rc.1 | 0.7.0-rc.2           | part1, part2 | yes                | ok      |
#
# | 34 | photon | 0.5.2      | 0.6.3                | part2, part1 | no, part2 rejected | ok      |
# | 35 | photon | 0.5.2      | 0.6.3                | part1, part2 | yes                | ok      |
# | 36 | photon | 0.6.3      | 0.7.0                | part2, part1 | no, part2 rejected | ok      |
# | 37 | photon | 0.6.3      | 0.7.0                | part1, part2 | yes                | ok      |
# | 38 | photon | 0.7.0      | 0.6.3                | part1, part2 | no, part1 rejected | ok      |
# | 39 | photon | 0.7.0      | 0.6.3                | part2, part1 | yes                | ok      |
# | 40 | photon | 0.6.3      | 0.5.5                | part1, part2 | no, part1 rejected | ok      |
# | 41 | photon | 0.6.3      | 0.5.5                | part2, part1 | yes                | ok      |
# | 42 | photon | 0.5.5      | 0.5.2                | part1, part2 | no, part1 rejected | ok      |
# | 43 | photon | 0.5.5      | 0.5.2                | part2, part1 | yes                | ok      |
# ------------------------------------------------------------------------------------------------+
#
# ------------------------ upgrade ---------------------------------------------------------------+
# | 44 | photon | 0.5.2      | 0.6.3                | part2, part1 | no, part2 rejected | ok      |
# | 45 | photon | 0.5.2      | 0.6.3                | part1, part2 | yes                | ok      |
# | 46 | photon | 0.6.3      | 0.7.0                | part2, part1 | no, part2 rejected | ok      |
# | 47 | photon | 0.6.3      | 0.7.0                | part1, part2 | yes                | ok      |
# | 48 | photon | 0.7.0      | 0.8.0-rc.3           | part2, part1 | no, part2 rejected | ok      |
# | 49 | photon | 0.7.0      | 0.8.0-rc.3           | part1, part2 | yes                | ok      |
# ------------------------ downgrade -------------------------------------------------------------+
# | 50 | photon | 0.8.0-rc.3 | 0.7.0                | part1, part2 | no, part1 rejected | ok      |
# | 51 | photon | 0.8.0-rc.3 | 0.7.0                | part2, part1 | yes                | ok      |
# | 52 | photon | 0.7.0      | 0.6.3                | part1, part2 | no, part1 rejected | ok      |
# | 53 | photon | 0.7.0      | 0.6.3                | part2, part1 | yes                | ok      |
# | 54 | photon | 0.6.3      | 0.5.5                | part1, part2 | no, part1 rejected | ok      |
# | 55 | photon | 0.6.3      | 0.5.5                | part2, part1 | yes                | ok      |
# | 56 | photon | 0.5.5      | 0.5.2                | part1, part2 | no, part1 rejected | ok      |
# | 57 | photon | 0.5.5      | 0.5.2                | part2, part1 | yes                | ok      |
# ------------------------------------------------------------------------------------------------+
#
# ------------------------ upgrade ---------------------------------------------------------------+
# | 58 | photon | 0.5.2      | 0.6.3                | part2, part1 | no, part2 rejected | ok      |
# | 59 | photon | 0.5.2      | 0.6.3                | part1, part2 | yes                | ok      |
# | 60 | photon | 0.6.3      | 0.7.0                | part2, part1 | no, part2 rejected | ok      |
# | 61 | photon | 0.6.3      | 0.7.0                | part1, part2 | yes                | ok      |
# | 62 | photon | 0.7.0      | 1.2.1                | part2, part1 | no, part2 rejected | ok      |
# | 63 | photon | 0.7.0      | 1.2.1                | part1, part2 | yes                | ok      |
# ------------------------ downgrade -------------------------------------------------------------+
# | 64 | photon | 1.2.1      | 0.7.0                | part1, part2 | no, part1 rejected | ok      |
# | 65 | photon | 1.2.1      | 0.7.0                | part2, part1 | yes                | ok      |
# | 66 | photon | 0.7.0      | 0.6.3                | part1, part2 | no, part1 rejected | ok      |
# | 67 | photon | 0.7.0      | 0.6.3                | part2, part1 | yes                | ok      |
# | 68 | photon | 0.6.3      | 0.5.5                | part1, part2 | no, part1 rejected | ok      |
# | 69 | photon | 0.6.3      | 0.5.5                | part2, part1 | yes                | ok      |
# | 70 | photon | 0.5.5      | 0.5.2                | part1, part2 | no, part1 rejected | ok      |
# | 71 | photon | 0.5.5      | 0.5.2                | part2, part1 | yes                | ok      |
# ------------------------------------------------------------------------------------------------+
#
# Not tested any more...
# --------------------------------------------------------------------------------------------------+
# | LAST | photon | 0.5.2      | 0.5.0                | user, part1, part2 | yes          | bricked |
# +------+--------+------------+----------------------+--------------+--------------------+---------+
# define the program in terms of helpers (main gets called at the end of the script)
main() {
  try check_bash_version
  try curl_all_required_system_parts
if true; then
  heading
  echo "+----+--------+------------+-------------------------------------+--------------------+--------+"
  echo "|  0 | photon | 0.5.2      |           initial setup             | yes                | ok     |"
  echo "+----+--------+------------+-------------------------------------+--------------------+--------+"
  # ensure an old compatible version of tinker, system firmware and bootloader is on the device
  enter_dfu_mode
  try dfu6user "tinker-v0.4.9-photon.bin"
  # first downgrade to 0.5.5 to downgrade the bootloader
  try dfu_system 0.5.5 photon
  try exit_dfu_mode
  sleep 10
  # then downgrade to 0.5.2
  enter_dfu_mode
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  # enter_ymodem
  # try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass
fi

if false; then
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  1 | photon | 0.5.3-rc.1 | 0.5.2                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  2 | photon | 0.5.3-rc.1 | 0.5.2                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass
fi

if false; then
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  3 | photon | 0.5.2      | 0.5.0                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.0 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_050_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.0 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_050_MOD}
  try compare_system_version 2 ${PHOTON_SP2_050_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  4 | photon | 0.5.0      | 0.5.2                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_050_MOD}

  enter_ymodem
  try ymodem_part 2 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  5 | photon | 0.5.2      | 0.5.3-rc.1           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  enter_ymodem
  try ymodem_part 2 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  6 | photon | 0.5.3-rc.1 | 0.5.3-rc.1           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"

  # Unreliable, as ymodem doesn't report the error, so system-parts might have been rejected
  # for all we know
  enter_ymodem
  try ymodem_part 1 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 2 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  7 | photon | 0.5.3-rc.1 | 0.5.3-rc.1           | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"

  # Unreliable, as ymodem doesn't report the error, so system-parts might have been rejected
  # for all we know
  enter_ymodem
  try ymodem_part 2 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  8 | photon | 0.5.3-rc.1 | 0.6.0-rc.1           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.0-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_060_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 2 0.6.0-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_060_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_060_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  9 | photon | 0.6.0-rc.1 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_060_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_060_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 10 | photon | 0.6.0-rc.1 | 0.5.3-rc.1           | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_060_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 11 | photon | 0.5.3-rc.1 | 0.6.0-rc.1           | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.6.0-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 12 | photon | 0.5.3-rc.1 | 0.5.2                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass
fi

if false; then
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 13 | photon | 0.5.2      | 0.5.0                | part1, part2 | yes                | ok-ish |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.5.0 photon
  # wait for self healing back to 0.5.2
  # this is bad, but is the case fixed and tested in above rejected part 1 tests.
  # THIS IS OUR PSUEDO-BRICKED TEST CASE.
  sleep 60
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass
fi

if false; then
      # | 14 | photon | 0.5.2      | 0.6.1-rc.1           | part1, part2 | yes                | ok      |
      # | 15 | photon | 0.6.1-rc.1 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok      |
      # | 16 | photon | 0.6.1-rc.1 | 0.5.3-rc.1           | part2, part1 | yes                | ok      |
      # | 17 | photon | 0.5.3-rc.1 | 0.6.1-rc.1           | part2, part1 | no, part2 rejected | ok      |
      # | 18 | photon | 0.5.3-rc.1 | 0.6.1-rc.1           | part1, part2 | yes                | ok      |
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 14 | photon | 0.5.2      | 0.6.1-rc.1           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.1-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass

  enter_ymodem
  try ymodem_part 2 0.6.1-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_061_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 15 | photon | 0.6.1-rc.1 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.3-rc.1 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_061_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 16 | photon | 0.6.1-rc.1 | 0.5.3-rc.1           | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 17 | photon | 0.5.3-rc.1 | 0.6.1-rc.1           | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.6.1-rc.1 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 18 | photon | 0.5.3-rc.1 | 0.6.1-rc.2           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.1-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 2 0.6.1-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_061_RC1_MOD}
  pass
fi

if false; then
      # | 19 | photon | 0.5.2      | 0.6.1-rc.2           | part1, part2 | yes                | ok      |
      # | 20 | photon | 0.6.1-rc.2 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok      |
      # | 21 | photon | 0.6.1-rc.2 | 0.5.3-rc.1           | part2, part1 | yes                | ok      |
      # | 22 | photon | 0.5.3-rc.1 | 0.6.1-rc.2           | part2, part1 | no, part2 rejected | ok      |
      # | 23 | photon | 0.5.3-rc.1 | 0.6.1-rc.2           | part1, part2 | yes                | ok      |
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 19 | photon | 0.5.2      | 0.6.1-rc.2           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.1-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass

  enter_ymodem
  try ymodem_part 2 0.6.1-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_061_RC2_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 20 | photon | 0.6.1-rc.2 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.3-rc.1 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_061_RC2_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 21 | photon | 0.6.1-rc.2 | 0.5.3-rc.1           | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 22 | photon | 0.5.3-rc.1 | 0.6.1-rc.2           | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.6.1-rc.2 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 23 | photon | 0.5.3-rc.1 | 0.6.1-rc.2           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.1-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 2 0.6.1-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_061_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_061_RC2_MOD}
  pass
fi

if false; then
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 24 | photon | 0.5.2      | 0.6.2                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_062_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass

  enter_ymodem
  try ymodem_part 2 0.6.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_062_MOD}
  try compare_system_version 2 ${PHOTON_SP2_062_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 25 | photon | 0.6.2      | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.3-rc.1 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_062_MOD}
  try compare_system_version 2 ${PHOTON_SP2_062_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 26 | photon | 0.6.2      | 0.5.3-rc.1           | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_062_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 27 | photon | 0.5.3-rc.1 | 0.6.2                | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.6.2 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 28 | photon | 0.5.3-rc.1 | 0.6.2                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_062_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 2 0.6.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_062_MOD}
  try compare_system_version 2 ${PHOTON_SP2_062_MOD}
  pass
fi

if false; then
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 29 | photon | 0.5.2      | 0.7.0-rc.2           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.7.0-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass

  enter_ymodem
  try ymodem_part 2 0.7.0-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_RC2_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 30 | photon | 0.7.0-rc.2 | 0.5.3-rc.1           | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.3-rc.1 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_RC2_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 31 | photon | 0.7.0-rc.2 | 0.5.3-rc.1           | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.3-rc.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 32 | photon | 0.5.3-rc.1 | 0.7.0-rc.2           | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.7.0-rc.2 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_053_RC1_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 33 | photon | 0.5.3-rc.1 | 0.7.0-rc.2           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.7.0-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_053_RC1_MOD}

  enter_ymodem
  try ymodem_part 2 0.7.0-rc.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_RC2_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_RC2_MOD}
  pass
fi

if false; then
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 34 | photon | 0.5.2      | 0.6.3                | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.6.3 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 35 | photon | 0.5.2      | 0.6.3                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  enter_ymodem
  try ymodem_part 2 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 36 | photon | 0.6.3      | 0.7.0                | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.7.0 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 37 | photon | 0.6.3      | 0.7.0                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.7.0 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}

  enter_ymodem
  try ymodem_part 2 0.7.0 photon
  # wait for 0.7.0 bootloader from SMH
  sleep 30
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 38 | photon | 0.7.0      | 0.6.3                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.6.3 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 39 | photon | 0.7.0      | 0.6.3                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}

  enter_ymodem
  try ymodem_part 1 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 40 | photon | 0.6.3      | 0.5.5                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.5 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 41 | photon | 0.6.3      | 0.5.5                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.5 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_055_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.5 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_055_MOD}
  try compare_system_version 2 ${PHOTON_SP2_055_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 42 | photon | 0.5.5      | 0.5.2                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.2 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_055_MOD}
  try compare_system_version 2 ${PHOTON_SP2_055_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 43 | photon | 0.5.5      | 0.5.2                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_055_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass
fi

if false; then
  # ------------------------ upgrade ---------------------------------------------------------------
  # ------------------------ upgrade ---------------------------------------------------------------
  # ------------------------ upgrade ---------------------------------------------------------------
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 44 | photon | 0.5.2      | 0.6.3                | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.6.3 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 45 | photon | 0.5.2      | 0.6.3                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  enter_ymodem
  try ymodem_part 2 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 46 | photon | 0.6.3      | 0.7.0                | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.7.0 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 47 | photon | 0.6.3      | 0.7.0                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.7.0 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}

  enter_ymodem
  try ymodem_part 2 0.7.0 photon
  # wait for 0.7.0 bootloader from SMH
  sleep 30
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 48 | photon | 0.7.0      | 0.8.0-rc.3           | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.8.0-rc.3 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 49 | photon | 0.7.0      | 0.8.0-rc.3           | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.8.0-rc.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_080_RC3_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}

  enter_ymodem
  try ymodem_part 2 0.8.0-rc.3 photon
  # wait for 0.8.0-rc.3 bootloader from SMH
  # particle flash --serial bootloader-0.8.0-rc.3-photon.bin
  sleep 30

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_080_RC3_MOD}
  try compare_system_version 2 ${PHOTON_SP2_080_RC3_MOD}
  pass

  # ------------------------ downgrade -------------------------------------------------------------
  # ------------------------ downgrade -------------------------------------------------------------
  # ------------------------ downgrade -------------------------------------------------------------
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 50 | photon | 0.8.0-rc.3 | 0.7.0                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.7.0  photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_080_RC3_MOD}
  try compare_system_version 2 ${PHOTON_SP1_080_RC3_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 51 | photon | 0.8.0-rc.3 | 0.7.0                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.7.0  photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_080_RC3_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}

  enter_ymodem
  try ymodem_part 1 0.7.0  photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP2_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 52 | photon | 0.7.0      | 0.6.3                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.6.3 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 53 | photon | 0.7.0      | 0.6.3                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}

  enter_ymodem
  try ymodem_part 1 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 54 | photon | 0.6.3      | 0.5.5                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.5 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 55 | photon | 0.6.3      | 0.5.5                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.5 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_055_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.5 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_055_MOD}
  try compare_system_version 2 ${PHOTON_SP2_055_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 56 | photon | 0.5.5      | 0.5.2                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.2 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_055_MOD}
  try compare_system_version 2 ${PHOTON_SP2_055_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 57 | photon | 0.5.5      | 0.5.2                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_055_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}
  pass
fi


if true; then
  # ------------------------ upgrade ---------------------------------------------------------------
  # ------------------------ upgrade ---------------------------------------------------------------
  # ------------------------ upgrade ---------------------------------------------------------------
  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 58 | photon | 0.5.2      | 0.6.3                | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.6.3 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 59 | photon | 0.5.2      | 0.6.3                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  enter_ymodem
  try ymodem_part 2 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 60 | photon | 0.6.3      | 0.7.0                | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 0.7.0 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 61 | photon | 0.6.3      | 0.7.0                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 0.7.0 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}

  enter_ymodem
  try ymodem_part 2 0.7.0 photon
  # wait for 0.7.0 bootloader from SMH
  sleep 30
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 62 | photon | 0.7.0      | 1.2.1                | part2, part1 | no, part2 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 2 1.2.1 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 63 | photon | 0.7.0      | 1.2.1                | part1, part2 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 1 1.2.1 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_121_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}

  enter_ymodem
  try ymodem_part 2 1.2.1 photon
  # wait for 1.2.1 bootloader from SMH
  # particle flash --serial bootloader-1.2.1-photon.bin
  echo "+----------------------------------------------------------------------------+"
  echo "| UPDATE BOOTLOADER TO 1.2.1 TO SIMULATE SMH, AND PRESS ENTER TO CONTINUE    |"
  echo "+----------------------------------------------------------------------------+"
  read junkvar
  sleep 30

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_121_MOD}
  try compare_system_version 2 ${PHOTON_SP2_121_MOD}
  pass

  # ------------------------ downgrade -------------------------------------------------------------
  # ------------------------ downgrade -------------------------------------------------------------
  # ------------------------ downgrade -------------------------------------------------------------

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 64 | photon | 1.2.1      | 0.7.0                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.7.0 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_121_MOD}
  try compare_system_version 2 ${PHOTON_SP2_121_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 65 | photon | 1.2.1      | 0.7.0                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.7.0 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_121_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}

  enter_ymodem
  try ymodem_part 1 0.7.0 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 66 | photon | 0.7.0      | 0.6.3                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.6.3 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_070_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 67 | photon | 0.7.0      | 0.6.3                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_070_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}

  enter_ymodem
  try ymodem_part 1 0.6.3 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 68 | photon | 0.6.3      | 0.5.5                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.5 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_063_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 69 | photon | 0.6.3      | 0.5.5                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.5 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_063_MOD}
  try compare_system_version 2 ${PHOTON_SP2_055_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.5 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_055_MOD}
  try compare_system_version 2 ${PHOTON_SP2_055_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 70 | photon | 0.5.5      | 0.5.2                | part1, part2 | no, part1 rejected | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  ymodem_part 1 0.5.2 photon
  enter_dfu_mode
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_055_MOD}
  try compare_system_version 2 ${PHOTON_SP2_055_MOD}
  pass

  heading
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "| 71 | photon | 0.5.5      | 0.5.2                | part2, part1 | yes                | ok     |"
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  enter_ymodem
  try ymodem_part 2 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_055_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}

  enter_ymodem
  try ymodem_part 1 0.5.2 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_052_MOD}


  echo "+-----------+------------+"
  echo "| LAST TEST | ALL PASSED |"
  echo "+-----------+------------+"
  enter_dfu_mode
  try exit_dfu_mode
  pass
  exit 0;
fi

# This has since failed to brick because when it reboots the Cloud safe mode heals it back to 0.5.2
# therefor, we don't test this anymore
if false; then
  heading
  echo "+------+--------+------------+----------------------+--------------------+--------------------+---------+"
  echo "| LAST | photon | 0.5.2      | 0.5.0                | user, part1, part2 | yes                | bricked |"
  echo "+------+--------+------------+----------------------+--------------------+--------------------+---------+"
  # ensure an new 0.5.2 version of tinker is on the device
  enter_dfu_mode
  try dfu6user "tinker-v0.4.9-photon.bin"
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_part 1 0.5.0 photon
  sleep 5

  # This has since failed to brick because when it reboots the Cloud safe mode heals it back to 0.5.2
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_052_MOD}
  try compare_system_version 2 ${PHOTON_SP2_050_MOD}

  enter_ymodem
  try ymodem_part 2 0.5.0 photon
  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline
  enter_ymodem
  try serial_inspect
  try compare_system_version 1 ${PHOTON_SP1_050_MOD}
  try compare_system_version 2 ${PHOTON_SP2_050_MOD}
  pass
fi

  # -----------------------------------------------------------------------------
  # Test Matrix Iterator
  # -----------------------------------------------------------------------------
  # declare -A TEST_1=(["1"]="0.5.0" ["2"]="DEFAULT" ["3"]="0.5.2" ["4"]="0.5.0" ["5"]="creds")
  # declare -A TEST_2=(["1"]="0.5.0" ["2"]="DEFAULT" ["3"]="0.5.1" ["4"]="0.5.2" ["5"]="0.5.0"  ["6"]="creds")
  # declare -A TEST_3=(["1"]="0.5.0" ["2"]="DEFAULT" ["3"]="0.5.2" ["4"]="0.5.1" ["5"]="creds")
  # TEST_MATRIX=(
  #   "${TEST_1[*]}"
  #   "${TEST_2[*]}"
  #   "${TEST_3[*]}"
  # )
  # echo "TESTS: " ${#TEST_MATRIX[@]}
  # for testkey in ${!TEST_MATRIX[@]}; do
  #     IFS=' ' read -a tasks <<< ${TEST_MATRIX[$testkey]}
  #     echo "TEST "${testkey}": "${tasks[@]}
  #     if [[ ${#tasks[@]} -gt 1 ]]; then
  #         for taskkey in ${!tasks[@]}; do
  #             task=${tasks[$taskkey]}
  #             echo "TASK "${taskkey}": "${task}
  #             # if task == version, flash version
  #             # if task == country, set country
  #             # if task == result, test result
  #         done
  #     fi
  # done
}

curl_all_required_system_parts() {
  if [ ! -e "bootloader-0.5.4-photon.bin" ]; then
    curl -L "$PHOTON_BOOT_054" -o "bootloader-0.5.4-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.5.0-photon.bin" ]; then
    curl -L "$PHOTON_SP1_050" -o "system-part1-0.5.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.0-photon.bin" ]; then
    curl -L "$PHOTON_SP2_050" -o "system-part2-0.5.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  # if [ ! -e "system-part1-0.5.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_051" -o "system-part1-0.5.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-0.5.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_051" -o "system-part2-0.5.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  if [ ! -e "system-part1-0.5.2-photon.bin" ]; then
    curl -L "$PHOTON_SP1_052" -o "system-part1-0.5.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.2-photon.bin" ]; then
    curl -L "$PHOTON_SP2_052" -o "system-part2-0.5.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  # if [ ! -e "system-part1-0.5.3-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_053_RC1" -o "system-part1-0.5.3-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-0.5.3-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_053_RC1" -o "system-part2-0.5.3-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  if [ ! -e "system-part1-0.5.5-photon.bin" ]; then
    curl -L "$PHOTON_SP1_055" -o "system-part1-0.5.5-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.5-photon.bin" ]; then
    curl -L "$PHOTON_SP2_055" -o "system-part2-0.5.5-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  # if [ ! -e "system-part1-0.6.0-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_060_RC1" -o "system-part1-0.6.0-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-0.6.0-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_060_RC1" -o "system-part2-0.6.0-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  # if [ ! -e "system-part1-0.6.1-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_061_RC1" -o "system-part1-0.6.1-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-0.6.1-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_061_RC1" -o "system-part2-0.6.1-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  # if [ ! -e "system-part1-0.6.2-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_062" -o "system-part1-0.6.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-0.6.2-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_062" -o "system-part2-0.6.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  if [ ! -e "system-part1-0.6.3-photon.bin" ]; then
    curl -L "$PHOTON_SP1_063" -o "system-part1-0.6.3-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.6.3-photon.bin" ]; then
    curl -L "$PHOTON_SP2_063" -o "system-part2-0.6.3-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  # if [ ! -e "system-part1-0.7.0-rc.2-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_070_RC2" -o "system-part1-0.7.0-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-0.7.0-rc.2-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_070_RC2" -o "system-part2-0.7.0-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  if [ ! -e "system-part1-0.7.0-photon.bin" ]; then
    curl -L "$PHOTON_SP1_070" -o "system-part1-0.7.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.7.0-photon.bin" ]; then
    curl -L "$PHOTON_SP2_070" -o "system-part2-0.7.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  # if [ ! -e "system-part1-0.8.0-rc.3-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_080_RC3" -o "system-part1-0.8.0-rc.3-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-0.8.0-rc.3-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_080_RC3" -o "system-part2-0.8.0-rc.3-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  # if [ ! -e "system-part1-1.0.0-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_100" -o "system-part1-1.0.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-1.0.0-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_100" -o "system-part2-1.0.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  # if [ ! -e "system-part1-1.0.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_101" -o "system-part1-1.0.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-1.0.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_101" -o "system-part2-1.0.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  # if [ ! -e "system-part1-1.1.0-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_110" -o "system-part1-1.1.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-1.1.0-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_110" -o "system-part2-1.1.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  # if [ ! -e "system-part1-1.2.0-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_121_RC1" -o "system-part1-1.2.0-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-1.2.0-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_121_RC1" -o "system-part2-1.2.0-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  # if [ ! -e "system-part1-1.2.1-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP1_121_RC1" -o "system-part1-1.2.1-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi
  # if [ ! -e "system-part2-1.2.1-rc.1-photon.bin" ]; then
  #   curl -L "$PHOTON_SP2_121_RC1" -o "system-part2-1.2.1-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  # fi

  if [ ! -e "system-part1-1.2.1-rc.2-photon.bin" ]; then
    curl -L "$PHOTON_SP1_121_RC1" -o "system-part1-1.2.1-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-1.2.1-rc.2-photon.bin" ]; then
    curl -L "$PHOTON_SP2_121_RC1" -o "system-part2-1.2.1-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "tinker-v0.4.9-photon.bin" ]; then
    curl -L "$PHOTON_TKR_049" -o "tinker-v0.4.9-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
}
run_dct_decoder_and_grep_country_plus() {
  # you can match application (set by us) or platform_dct (set by WICED)
  # based on what you pass here to be matched
  #
  # [application] (platform_dct)
  # default [0x00000000] (0x4a500002) - must set jp2 first, reboot, then set default
  # platform_dct.wifi_config.country_code (7536) = 0x4a500002 JP..
  # application_dct.country_code (9306) = 0x00000000 ....
  #
  # [JP2] (0x4a500002) - set JP2, reboot
  # platform_dct.wifi_config.country_code (7536) = 0x4a500002 JP..
  # application_dct.country_code (9306) = 0x4a503200 JP2.
  #
  # [US4] (0x55530004) - set US4, reboot
  # platform_dct.wifi_config.country_code (7536) = 0x55530004 US..
  # application_dct.country_code (9306) = 0x55533400 US4.
  #
  # [GB0] (0x47420000) - set GB0, reboot
  # platform_dct.wifi_config.country_code (7536) = 0x47420000 GB..
  # application_dct.country_code (9306) = 0x47423000 GB0.
  get_photon_dct photondct.bin
  local t=`dct-decoder photondct.bin | grep country | grep $1`;
  rm photondct.bin
  if [[ "$t" -eq 0 ]]; then
    echo "Country $1 has been set! Pass."
    return 0
  else
    echo "Country $1 was NOT set! Fail."
    return 1
  fi
}
configure_cli() {
  echo "CONFIGURING CLI FOR $CLOUD_NAME"
  particle config "$CLOUD_NAME"
  particle config apiUrl "$API_ENDPOINT"
  particle config username "$CLOUD_USERNAME"
}
run_cli_list_subcommand_and_confirm_device_shows_up_as_online() {
  sleep 20
  # particle list | grep "$DEVICE_ID" | grep online
  particle call $DEVICE_ID digitalwrite D7,LOW
  if [[ "$?" -eq 0 ]]; then
    echo "Device is online. Pass."
#    pass
    return 0
  else
    echo "Device is offline. Trying again in 20 seconds!"
    sleep 20
    # try again
    # particle list | grep "$DEVICE_ID" | grep online
    particle call $DEVICE_ID digitalwrite D7,LOW
    if [[ "$?" -eq 0 ]]; then
      echo "Device is online. Pass."
#      pass
      return 0
    else
      echo "Device is offline. Fail."
      fail
      return 1
    fi
  fi
}
run_cli_list_subcommand_and_confirm_device_shows_up_as_offline() {
  sleep 15
  # particle list | grep "$DEVICE_ID" | grep online
  particle call $DEVICE_ID digitalwrite D7,LOW
  if [[ "$?" -eq 0 ]]; then
    echo "Device is still online, waiting up to 1 minute for it to go offline..."
    tries_in_one_minute=6
    sleep 10
    while [ $tries_in_one_minute -gt 0 ]; do
      echo tries remaining: $tries_in_one_minute
      # particle list | grep "$DEVICE_ID" | grep online
      particle call $DEVICE_ID digitalwrite D7,LOW
      if [[ "$?" -eq 0 ]]; then
        let tries_in_one_minute-=1
        sleep 10
      else
        # break out
        let tries_in_one_minute=0
        echo "Device went offline. Pass."
        pass
        return 0
      fi
    done
    echo "Device remained online for 1 minute. Fail!"
    fail
    return 1
  else
    echo "Device is offline. Pass."
    echo "TODO: add check that connects over USB serial and checks for device id to test if in Listening Mode."
#    pass
    return 0
  fi
}
heading() {
  echo ""
  echo "+----+--------+------------+----------------------+--------------+--------------------+--------+"
  echo "|  # |  dev   |  version   | up/downgrade version |    order     |      flashed       | status |"
}
pass() {
  echo "8888888b.     d8888  .d8888b.   .d8888b."
  echo "888   Y88b   d88888 d88P  Y88b d88P  Y88b"
  echo "888    888  d88P888 Y88b.      Y88b."
  echo "888   d88P d88P 888  \"Y888b.    \"Y888b."
  echo "8888888P\" d88P  888     \"Y88b.     \"Y88b."
  echo "888      d88P   888       \"888       \"888"
  echo "888     d8888888888 Y88b  d88P Y88b  d88P"
  echo "888    d88P     888  \"Y8888P\"   \"Y8888P\""
}
fail() {
  echo "8888888888     d8888 8888888 888      888"
  echo "888           d88888   888   888      888"
  echo "888          d88P888   888   888      888"
  echo "8888888     d88P 888   888   888      888"
  echo "888        d88P  888   888   888      888"
  echo "888       d88P   888   888   888      Y8P"
  echo "888      d8888888888   888   888       \" "
  echo "888     d88P     888 8888888 88888888 888"
  exit 1;
}

main
