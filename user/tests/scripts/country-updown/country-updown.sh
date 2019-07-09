# Usage: WIFI_NETWORK=myssid WIFI_SECURITY=WPA2_AES WIFI_PASSWORD=mypass DEVICE_ID=123412341234123412341234 PORT=$port4p ./country-updown.sh
# set globals
: ${DEVICE_ID:?"REQUIRED"}
: ${PORT:?"REQUIRED"}
: ${WIFI_NETWORK:?"REQUIRED"}
: ${WIFI_SECURITY:?"REQUIRED"}
: ${WIFI_PASSWORD:?"REQUIRED"}
# or hardcode it here
#export DEVICE_ID="123412341234123412341234"
#
# install
# requires Bash 4.x for associative arrays, see upgrade instructions here:
# http://clubmate.fi/upgrade-to-bash-4-in-mac-os-x/
#!/usr/bin/env bash
#
export PHOTON_TKR_049="https://github.com/particle-iot/device-os/releases/download/v0.4.9-rc.3/tinker-v0.4.9-photon.bin"
export PHOTON_BOOT_054="https://github.com/particle-iot/device-os/releases/download/v0.5.4/bootloader-0.5.4-photon.bin"
export PHOTON_SP1_050="https://github.com/particle-iot/device-os/releases/download/v0.5.0/system-part1-0.5.0-photon.bin"
export PHOTON_SP2_050="https://github.com/particle-iot/device-os/releases/download/v0.5.0/system-part2-0.5.0-photon.bin"
export PHOTON_SP1_051="https://github.com/particle-iot/device-os/releases/download/v0.5.1/system-part1-0.5.1-photon.bin"
export PHOTON_SP2_051="https://github.com/particle-iot/device-os/releases/download/v0.5.1/system-part2-0.5.1-photon.bin"
export PHOTON_SP1_052="https://github.com/particle-iot/device-os/releases/download/v0.5.2/system-part1-0.5.2-photon.bin"
export PHOTON_SP2_052="https://github.com/particle-iot/device-os/releases/download/v0.5.2/system-part2-0.5.2-photon.bin"
export PHOTON_SP1_053_RC1="https://github.com/particle-iot/device-os/releases/download/v0.5.3-rc.1/system-part1-0.5.3-rc.1-photon.bin"
export PHOTON_SP2_053_RC1="https://github.com/particle-iot/device-os/releases/download/v0.5.3-rc.1/system-part2-0.5.3-rc.1-photon.bin"
export PHOTON_SP1_053_RC2="https://github.com/particle-iot/device-os/releases/download/v0.5.3-rc.2/system-part1-0.5.3-rc.2-photon.bin"
export PHOTON_SP2_053_RC2="https://github.com/particle-iot/device-os/releases/download/v0.5.3-rc.2/system-part2-0.5.3-rc.2-photon.bin"
export PHOTON_SP1_053_RC3="https://github.com/particle-iot/device-os/releases/download/v0.5.3-rc.3/system-part1-0.5.3-rc.3-photon.bin"
export PHOTON_SP2_053_RC3="https://github.com/particle-iot/device-os/releases/download/v0.5.3-rc.3/system-part2-0.5.3-rc.3-photon.bin"
export PHOTON_SP1_055="https://github.com/particle-iot/device-os/releases/download/v0.5.5/system-part1-0.5.5-photon.bin"
export PHOTON_SP2_055="https://github.com/particle-iot/device-os/releases/download/v0.5.5/system-part2-0.5.5-photon.bin"
export PHOTON_SP1_055_MOD=23
export PHOTON_SP2_055_MOD=23
export PHOTON_SP1_060_RC1="https://github.com/particle-iot/device-os/releases/download/v0.6.0-rc.1/system-part1-0.6.0-rc.1-photon.bin"
export PHOTON_SP2_060_RC1="https://github.com/particle-iot/device-os/releases/download/v0.6.0-rc.1/system-part2-0.6.0-rc.1-photon.bin"
export PHOTON_SP1_060_RC2="https://github.com/particle-iot/device-os/releases/download/v0.6.0-rc.2/system-part1-0.6.0-rc.2-photon.bin"
export PHOTON_SP2_060_RC2="https://github.com/particle-iot/device-os/releases/download/v0.6.0-rc.2/system-part2-0.6.0-rc.2-photon.bin"
export PHOTON_SP1_061_RC1="https://github.com/particle-iot/device-os/releases/download/v0.6.1-rc.1/system-part1-0.6.1-rc.1-photon.bin"
export PHOTON_SP2_061_RC1="https://github.com/particle-iot/device-os/releases/download/v0.6.1-rc.1/system-part2-0.6.1-rc.1-photon.bin"
export PHOTON_SP1_061_RC2="https://github.com/particle-iot/device-os/releases/download/v0.6.1-rc.2/system-part1-0.6.1-rc.2-photon.bin"
export PHOTON_SP2_061_RC2="https://github.com/particle-iot/device-os/releases/download/v0.6.1-rc.2/system-part2-0.6.1-rc.2-photon.bin"
export PHOTON_SP1_061="https://github.com/particle-iot/device-os/releases/download/v0.6.1/system-part1-0.6.1-photon.bin"
export PHOTON_SP2_061="https://github.com/particle-iot/device-os/releases/download/v0.6.1/system-part2-0.6.1-photon.bin"
export PHOTON_SP1_062="https://github.com/particle-iot/device-os/releases/download/v0.6.2/system-part1-0.6.2-photon.bin"
export PHOTON_SP2_062="https://github.com/particle-iot/device-os/releases/download/v0.6.2/system-part2-0.6.2-photon.bin"
export PHOTON_SP1_063="https://github.com/particle-iot/device-os/releases/download/v0.6.3/system-part1-0.6.3-photon.bin"
export PHOTON_SP2_063="https://github.com/particle-iot/device-os/releases/download/v0.6.3/system-part2-0.6.3-photon.bin"
export PHOTON_SP1_070_RC2="https://github.com/particle-iot/device-os/releases/download/v0.7.0-rc.2/system-part1-0.7.0-rc.2-photon.bin"
export PHOTON_SP2_070_RC2="https://github.com/particle-iot/device-os/releases/download/v0.7.0-rc.2/system-part2-0.7.0-rc.2-photon.bin"
export PHOTON_BOOT_070="https://github.com/particle-iot/device-os/releases/download/v0.7.0/bootloader-0.7.0-photon.bin"
export PHOTON_SP1_070="https://github.com/particle-iot/device-os/releases/download/v0.7.0/system-part1-0.7.0-photon.bin"
export PHOTON_SP2_070="https://github.com/particle-iot/device-os/releases/download/v0.7.0/system-part2-0.7.0-photon.bin"
export PHOTON_SP1_101="https://github.com/particle-iot/device-os/releases/download/v1.0.1/system-part1-1.0.1-photon.bin"
export PHOTON_SP2_101="https://github.com/particle-iot/device-os/releases/download/v1.0.1/system-part2-1.0.1-photon.bin"
export PHOTON_SP1_110="https://github.com/particle-iot/device-os/releases/download/v1.1.0/photon-system-part1@1.1.0.bin"
export PHOTON_SP2_110="https://github.com/particle-iot/device-os/releases/download/v1.1.0/photon-system-part2@1.1.0.bin"
export PHOTON_SP1_121="https://github.com/particle-iot/device-os/releases/download/v1.2.1/photon-system-part1@1.2.1.bin"
export PHOTON_SP2_121="https://github.com/particle-iot/device-os/releases/download/v1.2.1/photon-system-part2@1.2.1.bin"

# instead of these set options,
#   set script options
#   set -e # fail on any error
#   set -x # be verbose as fack
# we are going for the 3 fingered claw approach (try <task>)
yell() { echo "$0: $*" >&2; }
die() { yell "$*"; exit 111; }
try() { "$@" || die "cannot $*"; }

# Finds and puts device in dfu mode automatically, must only have one device connected.
dfu() {
  #local d=`dfu-util -l | grep d00`;
  #if [ -z "${d}" ]; then
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
  #else
  #  echo "already in DFU mode!";
  #  return 1;
  #fi
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
set_wifi_credentials() {
  echo '{"network":"'${WIFI_NETWORK}'","security":"'${WIFI_SECURITY}'","password":"'${WIFI_PASSWORD}'"}' > POOPY_DIAPER.json
  result=$(particle serial wifi --port ${PORT} --file POOPY_DIAPER.json)
  rm -f POOPY_DIAPER.json
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
curl_all_required_system_parts() {
  if [ ! -e "bootloader-0.5.4-photon.bin" ]; then
    curl --fail -L "$PHOTON_BOOT_054" -o "bootloader-0.5.4-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.5.0-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_050" -o "system-part1-0.5.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.0-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_050" -o "system-part2-0.5.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.5.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_051" -o "system-part1-0.5.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_051" -o "system-part2-0.5.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.5.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_052" -o "system-part1-0.5.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_052" -o "system-part2-0.5.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.5.3-rc.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_053_RC1" -o "system-part1-0.5.3-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.3-rc.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_053_RC1" -o "system-part2-0.5.3-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.5.3-rc.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_053_RC2" -o "system-part1-0.5.3-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.3-rc.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_053_RC2" -o "system-part2-0.5.3-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.5.3-rc.3-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_053_RC3" -o "system-part1-0.5.3-rc.3-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.3-rc.3-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_053_RC3" -o "system-part2-0.5.3-rc.3-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.5.5-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_055" -o "system-part1-0.5.5-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.5.5-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_055" -o "system-part2-0.5.5-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.6.0-rc.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_060_RC1" -o "system-part1-0.6.0-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.6.0-rc.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_060_RC1" -o "system-part2-0.6.0-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.6.0-rc.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_060_RC2" -o "system-part1-0.6.0-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.6.0-rc.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_060_RC2" -o "system-part2-0.6.0-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part1-0.6.1-rc.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_061_RC1" -o "system-part1-0.6.1-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.6.1-rc.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_061_RC1" -o "system-part2-0.6.1-rc.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "system-part1-0.6.1-rc.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_061_RC2" -o "system-part1-0.6.1-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.6.1-rc.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_061_RC2" -o "system-part2-0.6.1-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "system-part1-0.6.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_061" -o "system-part1-0.6.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.6.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_061" -o "system-part2-0.6.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "system-part1-0.6.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_062" -o "system-part1-0.6.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.6.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_062" -o "system-part2-0.6.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "system-part1-0.6.3-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_063" -o "system-part1-0.6.3-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.6.3-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_063" -o "system-part2-0.6.3-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "system-part1-0.7.0-rc.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_070_RC2" -o "system-part1-0.7.0-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.7.0-rc.2-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_070_RC2" -o "system-part2-0.7.0-rc.2-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "system-part1-0.7.0-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_070" -o "system-part1-0.7.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-0.7.0-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_070" -o "system-part2-0.7.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "bootloader-0.7.0-photon.bin" ]; then
    curl --fail -L "$PHOTON_BOOT_070" -o "bootloader-0.7.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "system-part1-1.0.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_101" -o "system-part1-1.0.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-1.0.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_101" -o "system-part2-1.0.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "system-part1-1.1.0-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_110" -o "system-part1-1.1.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-1.1.0-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_110" -o "system-part2-1.1.0-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "system-part1-1.2.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP1_121" -o "system-part1-1.2.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
  if [ ! -e "system-part2-1.2.1-photon.bin" ]; then
    curl --fail -L "$PHOTON_SP2_121" -o "system-part2-1.2.1-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi

  if [ ! -e "tinker-v0.4.9-photon.bin" ]; then
    curl --fail -L "$PHOTON_TKR_049" -o "tinker-v0.4.9-photon.bin"; if [[ "$?" -ne 0 ]]; then return 1; fi
  fi
}
get_country() {
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
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  # | country | program | country |  upgrade   | country | dngrade | result                 |"
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

# ----+-----------------------------+-------------------------------------------------------
#     |   reset w/o losing creds    |
# ----+---------+---------+---------+------------+---------+---------+----------------------
#   # | country | program | country |  upgrade   | country | dngrade | result
# ----+---------+---------+---------+------------+---------+---------+----------------------
#   1 |   JP2   |   050   | DEFAULT |   053rc1   |   N/A   |   050   | online
#   2 |   JP2   |   050   | DEFAULT | 051/053rc1 |   N/A   |   050   | online
#   3 |   JP2   |   050   | DEFAULT |   053rc1   |   N/A   |   051   | online
#   4 |   JP2   |   050   | DEFAULT |   053rc1   |   JP2   |   050   | online
#   5 |   JP2   |   050   | DEFAULT | 051/053rc1 |   JP2   |   050   | online
#   6 |   JP2   |   050   | DEFAULT |   053rc1   |   JP2   |   051   | online
#   7 |   JP2   |   050   | DEFAULT |   053rc1   |   GB0   |   050   | wiped creds, re-enter
#   8 |   JP2   |   050   | DEFAULT |   053rc1   | DEFAULT |   050   | re-test, online
# ----+---------+---------+---------+------------+---------+---------+----------------------
#   9 |   JP2   |   050   | DEFAULT |     052    |   N/A   |   050   | online
#  10 |   JP2   |   050   | DEFAULT |   051/052  |   N/A   |   050   | online
#  11 |   JP2   |   050   | DEFAULT |     052    |   N/A   |   051   | online
#  12 |   JP2   |   050   | DEFAULT |     052    |   JP2   |   050   | online
#  13 |   JP2   |   050   | DEFAULT |   051/052  |   JP2   |   050   | online
#  14 |   JP2   |   050   | DEFAULT |     052    |   JP2   |   051   | online
#  15 |   JP2   |   050   | DEFAULT |     052    |   GB0   |   050   | wiped creds, re-enter
#  16 |   JP2   |   050   | DEFAULT |     052    | DEFAULT |   050   | re-test, online
# ----+---------+---------+---------+------------+---------+---------+----------------------
#  17 |   JP2   |   050   | DEFAULT |   060rc1   |   N/A   |   050   | online
#  18 |   JP2   |   050   | DEFAULT | 051/060rc1 |   N/A   |   050   | online
#  19 |   JP2   |   050   | DEFAULT |   060rc1   |   N/A   |   051   | online
#  20 |   JP2   |   050   | DEFAULT |   060rc1   |   JP2   |   050   | online
#  21 |   JP2   |   050   | DEFAULT | 051/060rc1 |   JP2   |   050   | online
#  22 |   JP2   |   050   | DEFAULT |   060rc1   |   JP2   |   051   | online
#  23 |   JP2   |   050   | DEFAULT |   060rc1   |   GB0   |   050   | wiped creds, re-enter
#  24 |   JP2   |   050   | DEFAULT |   060rc1   | DEFAULT |   050   | re-test, online
# ----+---------+---------+---------+------------+---------+---------+----------------------
#  25 |   JP2   |   050   | DEFAULT |   060rc2   |   N/A   |   050   | online
#  26 |   JP2   |   050   | DEFAULT | 051/060rc2 |   N/A   |   050   | online
#  27 |   JP2   |   050   | DEFAULT |   060rc2   |   N/A   |   051   | online
#  28 |   JP2   |   050   | DEFAULT |   060rc2   |   JP2   |   050   | online
#  29 |   JP2   |   050   | DEFAULT | 051/060rc2 |   JP2   |   050   | online
#  30 |   JP2   |   050   | DEFAULT |   060rc2   |   JP2   |   051   | online
#  31 |   JP2   |   050   | DEFAULT |   060rc2   |   GB0   |   050   | wiped creds, re-enter
#  32 |   JP2   |   050   | DEFAULT |   060rc2   | DEFAULT |   050   | re-test, online
# ----+---------+---------+---------+------------+---------+---------+----------------------
#  33 |   JP2   |   050   | DEFAULT |   061rc1   |   N/A   |   050   | online
#  34 |   JP2   |   050   | DEFAULT | 051/061rc1 |   N/A   |   050   | online
#  35 |   JP2   |   050   | DEFAULT |   061rc1   |   N/A   |   051   | online
#  36 |   JP2   |   050   | DEFAULT |   061rc1   |   JP2   |   050   | online
#  37 |   JP2   |   050   | DEFAULT | 051/061rc1 |   JP2   |   050   | online
#  38 |   JP2   |   050   | DEFAULT |   061rc1   |   JP2   |   051   | online
#  39 |   JP2   |   050   | DEFAULT |   061rc1   |   GB0   |   050   | wiped creds, re-enter
#  40 |   JP2   |   050   | DEFAULT |   061rc1   | DEFAULT |   050   | re-test, online
# ----+---------+---------+---------+------------+---------+---------+----------------------
#  41 |   JP2   |   050   | DEFAULT |   061rc2   |   N/A   |   050   | online
#  42 |   JP2   |   050   | DEFAULT | 051/061rc2 |   N/A   |   050   | online
#  43 |   JP2   |   050   | DEFAULT |   061rc2   |   N/A   |   051   | online
#  44 |   JP2   |   050   | DEFAULT |   061rc2   |   JP2   |   050   | online
#  45 |   JP2   |   050   | DEFAULT | 051/061rc2 |   JP2   |   050   | online
#  46 |   JP2   |   050   | DEFAULT |   061rc2   |   JP2   |   051   | online
#  47 |   JP2   |   050   | DEFAULT |   061rc2   |   GB0   |   050   | wiped creds, re-enter
#  48 |   JP2   |   050   | DEFAULT |   061rc2   | DEFAULT |   050   | re-test, online
# ----+---------+---------+---------+------------+---------+---------+----------------------
#  49 |   JP2   |   050   | DEFAULT |   061      |   N/A   |   050   | online
#  50 |   JP2   |   050   | DEFAULT | 051/061    |   N/A   |   050   | online
#  51 |   JP2   |   050   | DEFAULT |   061      |   N/A   |   051   | online
#  52 |   JP2   |   050   | DEFAULT |   061      |   JP2   |   050   | online
#  53 |   JP2   |   050   | DEFAULT | 051/061    |   JP2   |   050   | online
#  54 |   JP2   |   050   | DEFAULT |   061      |   JP2   |   051   | online
#  55 |   JP2   |   050   | DEFAULT |   061      |   GB0   |   050   | wiped creds, re-enter
#  56 |   JP2   |   050   | DEFAULT |   061      | DEFAULT |   050   | re-test, online
# ----+---------+---------+---------+------------+---------+---------+----------------------
#  57 |   JP2   |   050   | DEFAULT |   062      |   N/A   |   050   | online
#  58 |   JP2   |   050   | DEFAULT | 051/062    |   N/A   |   050   | online
#  59 |   JP2   |   050   | DEFAULT |   062      |   N/A   |   051   | online
#  60 |   JP2   |   050   | DEFAULT |   062      |   JP2   |   050   | online
#  61 |   JP2   |   050   | DEFAULT | 051/062    |   JP2   |   050   | online
#  62 |   JP2   |   050   | DEFAULT |   062      |   JP2   |   051   | online
#  63 |   JP2   |   050   | DEFAULT |   062      |   GB0   |   050   | wiped creds, re-enter
#  64 |   JP2   |   050   | DEFAULT |   062      | DEFAULT |   050   | re-test, online
# ----+---------+---------+---------+------------+---------+---------+----------------------
#  65 |   JP2   |   050   | DEFAULT |   070rc2   |   N/A   |   050   | online
#  66 |   JP2   |   050   | DEFAULT | 051/070rc2 |   N/A   |   050   | online
#  67 |   JP2   |   050   | DEFAULT |   070rc2   |   N/A   |   051   | online
#  68 |   JP2   |   050   | DEFAULT |   070rc2   |   JP2   |   050   | online
#  69 |   JP2   |   050   | DEFAULT | 051/070rc2 |   JP2   |   050   | online
#  70 |   JP2   |   050   | DEFAULT |   070rc2   |   JP2   |   051   | online
#  71 |   JP2   |   050   | DEFAULT |   070rc2   |   GB0   |   050   | wiped creds, re-enter
#  72 |   JP2   |   050   | DEFAULT |   070rc2   | DEFAULT |   050   | re-test, online
# ----+---------+---------+---------+------------+---------+---------+----------------------
#  73 |   JP2   |   050   | DEFAULT |   070      |   N/A   |   063/050   | online
#  74 |   JP2   |   050   | DEFAULT | 051/070    |   N/A   |   063/050   | online
#  75 |   JP2   |   050   | DEFAULT |   070      |   N/A   |   063/051   | online
#  76 |   JP2   |   050   | DEFAULT |   070      |   JP2   |   063/050   | online
#  77 |   JP2   |   050   | DEFAULT | 051/070    |   JP2   |   063/050   | online
#  78 |   JP2   |   050   | DEFAULT |   070      |   JP2   |   063/051   | online
#  79 |   JP2   |   050   | DEFAULT |   070      |   GB0   |   063/050   | wiped creds, re-enter
#  80 |   JP2   |   050   | DEFAULT |   070      | DEFAULT |   063/050   | re-test, online
# ----+---------+---------+---------+------------+---------+-------------+----------------------
#  81 |   JP2   |   050   | DEFAULT |   101      |   N/A   |   070/063/050   | online
#  82 |   JP2   |   050   | DEFAULT | 051/101    |   N/A   |   070/063/050   | online
#  83 |   JP2   |   050   | DEFAULT |   101      |   N/A   |   070/063/051   | online
#  84 |   JP2   |   050   | DEFAULT |   101      |   JP2   |   070/063/050   | online
#  85 |   JP2   |   050   | DEFAULT | 051/101    |   JP2   |   070/063/050   | online
#  86 |   JP2   |   050   | DEFAULT |   101      |   JP2   |   070/063/051   | online
#  87 |   JP2   |   050   | DEFAULT |   101      |   GB0   |   070/063/050   | wiped creds, re-enter
#  88 |   JP2   |   050   | DEFAULT |   101      | DEFAULT |   070/063/050   | re-test, online
# ----+---------+---------+---------+------------+---------+-----------------+----------------------
#  89 |   JP2   |   050   | DEFAULT |   121      |   N/A   |   070/063/050   | online
#  90 |   JP2   |   050   | DEFAULT | 051/121    |   N/A   |   070/063/050   | online
#  91 |   JP2   |   050   | DEFAULT |   121      |   N/A   |   070/063/051   | online
#  92 |   JP2   |   050   | DEFAULT |   121      |   JP2   |   070/063/050   | online
#  93 |   JP2   |   050   | DEFAULT | 051/121    |   JP2   |   070/063/050   | online
#  94 |   JP2   |   050   | DEFAULT |   121      |   JP2   |   070/063/051   | online
#  95 |   JP2   |   050   | DEFAULT |   121      |   GB0   |   070/063/050   | wiped creds, re-enter
#  96 |   JP2   |   050   | DEFAULT |   121      | DEFAULT |   070/063/050   | re-test, online
# ----+---------+---------+---------+------------+---------+-----------------+----------------------
# LAST TEST | ALL PASSED
# -------------------------

# define the program in terms of helpers (main gets called at the end of the script)
main() {
  try check_bash_version
  try curl_all_required_system_parts

if true; then
  # ensure an old compatible version of tinker is on the device
  enter_dfu_mode
  try dfu6user "tinker-v0.4.9-photon.bin"
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.5 photon
  try exit_dfu_mode

  sleep 10

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  # # enter_ymodem
  # try ymodem_boot 0.5.4 photon

  enter_dfu_mode
  set_country JP2
  try exit_dfu_mode
  sleep 10

  enter_dfu_mode
  set_country DEFAULT
  try exit_dfu_mode
  sleep 10

  # MAKE SURE THE DEVICE HAS CREDENTIALS WHEN STARTING THE TEST!!!!!
  # IT CAN GET SCREWED UP IF YOU RESET THE DEVICE DURING THE SETUP, OR TEST PROCESS.
  # put credentials back
  enter_ymodem
  set_wifi_credentials
fi

if false; then
  # ------
  # 052
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  1 |   JP2   |   050   | DEFAULT |    052     |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  2 |   JP2   |   050   | DEFAULT |  051/052   |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  3 |   JP2   |   050   | DEFAULT |    052     |   N/A   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  4 |   JP2   |   050   | DEFAULT |    052     |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  5 |   JP2   |   050   | DEFAULT |  051/052   |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  6 |   JP2   |   050   | DEFAULT |    052     |   JP2   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  7 |   JP2   |   050   | DEFAULT |    052     |   GB0   |   050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  8 |   JP2   |   050   | DEFAULT |    052     | DEFAULT |   050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 053rc1
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "|  9 |   JP2   |   050   | DEFAULT |  053rc1    |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.3-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 10 |   JP2   |   050   | DEFAULT | 051/053rc1 |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.3-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 11 |   JP2   |   050   | DEFAULT |   053rc1   |   N/A   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.3-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 12 |   JP2   |   050   | DEFAULT |   053rc1   |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.3-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 13 |   JP2   |   050   | DEFAULT | 051/053rc1 |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.3-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 14 |   JP2   |   050   | DEFAULT |   053rc1   |   JP2   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.3-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 15 |   JP2   |   050   | DEFAULT |   053rc1   |   GB0   |   050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.3-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 16 |   JP2   |   050   | DEFAULT |   053rc1   | DEFAULT |   050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.3-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 060rc1
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 17 |   JP2   |   050   | DEFAULT |   060rc1   |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 18 |   JP2   |   050   | DEFAULT | 051/060rc1 |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 19 |   JP2   |   050   | DEFAULT |   060rc1   |   N/A   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 20 |   JP2   |   050   | DEFAULT |   060rc1   |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 21 |   JP2   |   050   | DEFAULT | 051/060rc1 |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 22 |   JP2   |   050   | DEFAULT |   060rc1   |   JP2   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 23 |   JP2   |   050   | DEFAULT |   060rc1   |   GB0   |   050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 24 |   JP2   |   050   | DEFAULT |   060rc1   | DEFAULT |   050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 060rc2
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 25 |   JP2   |   050   | DEFAULT |   060rc2   |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 26 |   JP2   |   050   | DEFAULT | 051/060rc2 |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 27 |   JP2   |   050   | DEFAULT |   060rc2   |   N/A   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 28 |   JP2   |   050   | DEFAULT |   060rc2   |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 29 |   JP2   |   050   | DEFAULT | 051/060rc2 |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 30 |   JP2   |   050   | DEFAULT |   060rc2   |   JP2   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 31 |   JP2   |   050   | DEFAULT |   060rc2   |   GB0   |   050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 32 |   JP2   |   050   | DEFAULT |   060rc2   | DEFAULT |   050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 061rc1
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 33 |   JP2   |   050   | DEFAULT |   061rc1   |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 34 |   JP2   |   050   | DEFAULT | 051/061rc1 |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 35 |   JP2   |   050   | DEFAULT |   061rc1   |   N/A   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 36 |   JP2   |   050   | DEFAULT |   061rc1   |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 37 |   JP2   |   050   | DEFAULT | 051/061rc1 |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 38 |   JP2   |   050   | DEFAULT |   061rc1   |   JP2   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 39 |   JP2   |   050   | DEFAULT |   061rc1   |   GB0   |   050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 40 |   JP2   |   050   | DEFAULT |   061rc1   | DEFAULT |   050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 061rc2
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 41 |   JP2   |   050   | DEFAULT |   061rc2   |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 42 |   JP2   |   050   | DEFAULT | 051/061rc2 |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 43 |   JP2   |   050   | DEFAULT |   061rc2   |   N/A   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 44 |   JP2   |   050   | DEFAULT |   061rc2   |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 45 |   JP2   |   050   | DEFAULT | 051/061rc2 |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 46 |   JP2   |   050   | DEFAULT |   061rc2   |   JP2   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 47 |   JP2   |   050   | DEFAULT |   061rc2   |   GB0   |   050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 48 |   JP2   |   050   | DEFAULT |   061rc2   | DEFAULT |   050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 061
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 49 |   JP2   |   050   | DEFAULT |   061      |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 50 |   JP2   |   050   | DEFAULT | 051/061    |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 51 |   JP2   |   050   | DEFAULT |   061      |   N/A   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 52 |   JP2   |   050   | DEFAULT |   061      |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 53 |   JP2   |   050   | DEFAULT | 051/061    |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 54 |   JP2   |   050   | DEFAULT |   061      |   JP2   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 55 |   JP2   |   050   | DEFAULT |   061      |   GB0   |   050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 56 |   JP2   |   050   | DEFAULT |   061      | DEFAULT |   050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 062
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 57 |   JP2   |   050   | DEFAULT |   062      |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 58 |   JP2   |   050   | DEFAULT | 051/062    |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 59 |   JP2   |   050   | DEFAULT |   062      |   N/A   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 60 |   JP2   |   050   | DEFAULT |   062      |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 61 |   JP2   |   050   | DEFAULT | 051/062    |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 62 |   JP2   |   050   | DEFAULT |   062      |   JP2   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 63 |   JP2   |   050   | DEFAULT |   062      |   GB0   |   050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 64 |   JP2   |   050   | DEFAULT |   062      | DEFAULT |   050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 070rc2
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 65 |   JP2   |   050   | DEFAULT |   070rc2   |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 66 |   JP2   |   050   | DEFAULT | 051/070rc2 |   N/A   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 67 |   JP2   |   050   | DEFAULT |   070rc2   |   N/A   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 68 |   JP2   |   050   | DEFAULT |   070rc2   |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 69 |   JP2   |   050   | DEFAULT | 051/070rc2 |   JP2   |   050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 70 |   JP2   |   050   | DEFAULT |   070rc2   |   JP2   |   051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 71 |   JP2   |   050   | DEFAULT |   070rc2   |   GB0   |   050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  echo "| 72 |   JP2   |   050   | DEFAULT |   070rc2   | DEFAULT |   050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+---------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0-rc.2 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 070
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  echo "| 73 |   JP2   |   050   | DEFAULT |   070      |   N/A   |   063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  # wait for 0.7.0 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  echo "| 74 |   JP2   |   050   | DEFAULT | 051/070    |   N/A   |   063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  # wait for 0.7.0 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  echo "| 75 |   JP2   |   050   | DEFAULT |   070      |   N/A   |   063/051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  # wait for 0.7.0 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  echo "| 76 |   JP2   |   050   | DEFAULT |   070      |   JP2   |   063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  # wait for 0.7.0 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  echo "| 77 |   JP2   |   050   | DEFAULT | 051/070    |   JP2   |   063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  # wait for 0.7.0 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  echo "| 78 |   JP2   |   050   | DEFAULT |   070      |   JP2   |   063/051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  # wait for 0.7.0 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  echo "| 79 |   JP2   |   050   | DEFAULT |   070      |   GB0   |   063/050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  # wait for 0.7.0 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  echo "| 80 |   JP2   |   050   | DEFAULT |   070      | DEFAULT |   063/050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+-------------+------------------------+"
  particle serial wifi

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  # wait for 0.7.0 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online
fi

if false; then
  # ------
  # 101
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 81 |   JP2   |   050   | DEFAULT |   101      |   N/A   |   070/063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.0.1 photon
  try exit_dfu_mode

  # wait for 1.0.1 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 82 |   JP2   |   050   | DEFAULT | 051/101    |   N/A   |   070/063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.0.1 photon
  try exit_dfu_mode

  # wait for 1.0.1 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 83 |   JP2   |   050   | DEFAULT |   101      |   N/A   |   070/063/051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.0.1 photon
  try exit_dfu_mode

  # wait for 1.0.1 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 84 |   JP2   |   050   | DEFAULT |   101      |   JP2   |   070/063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.0.1 photon
  try exit_dfu_mode

  # wait for 1.0.1 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 85 |   JP2   |   050   | DEFAULT | 051/101    |   JP2   |   070/063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.0.1 photon
  try exit_dfu_mode

  # wait for 1.0.1 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 86 |   JP2   |   050   | DEFAULT |   101      |   JP2   |   070/063/051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.0.1 photon
  try exit_dfu_mode

  # wait for 1.0.1 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 87 |   JP2   |   050   | DEFAULT |   101      |   GB0   |   070/063/050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.0.1 photon
  try exit_dfu_mode

  # wait for 1.0.1 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 88 |   JP2   |   050   | DEFAULT |   101      | DEFAULT |   070/063/050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"

  enter_ymodem
  set_wifi_credentials

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.0.1 photon
  try exit_dfu_mode

  # wait for 1.0.1 bootloader from SMH
  sleep 30

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

fi

if true; then
  # ------
  # 121
  # ------

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 89 |   JP2   |   050   | DEFAULT |   121      |   N/A   |   070/063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.2.1 photon
  try exit_dfu_mode

  # wait for 1.2.1 bootloader from SMH
  sleep 10
  try ymodem_boot 1.2.1 photon
  sleep 10

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon
  sleep 20

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 90 |   JP2   |   050   | DEFAULT | 051/121    |   N/A   |   070/063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.2.1 photon
  try exit_dfu_mode

  # wait for 1.2.1 bootloader from SMH
  sleep 10
  try ymodem_boot 1.2.1 photon
  sleep 10

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon
  sleep 20

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 91 |   JP2   |   050   | DEFAULT |   121      |   N/A   |   070/063/051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.2.1 photon
  try exit_dfu_mode

  # wait for 1.2.1 bootloader from SMH
  sleep 10
  try ymodem_boot 1.2.1 photon
  sleep 10

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon
  sleep 20

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 92 |   JP2   |   050   | DEFAULT |   121      |   JP2   |   070/063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.2.1 photon
  try exit_dfu_mode

  # wait for 1.2.1 bootloader from SMH
  sleep 10
  try ymodem_boot 1.2.1 photon
  sleep 10

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon
  sleep 20

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 93 |   JP2   |   050   | DEFAULT | 051/121    |   JP2   |   070/063/050   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.2.1 photon
  try exit_dfu_mode

  # wait for 1.2.1 bootloader from SMH
  sleep 10
  try ymodem_boot 1.2.1 photon
  sleep 10

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon
  sleep 20

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 94 |   JP2   |   050   | DEFAULT |   121      |   JP2   |   070/063/051   | online                 |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.2.1 photon
  try exit_dfu_mode

  # wait for 1.2.1 bootloader from SMH
  sleep 10
  try ymodem_boot 1.2.1 photon
  sleep 10

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country JP2
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.1 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon
  sleep 20

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 95 |   JP2   |   050   | DEFAULT |   121      |   GB0   |   070/063/050   | wiped creds, re-enter  |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.2.1 photon
  try exit_dfu_mode

  # wait for 1.2.1 bootloader from SMH
  sleep 10
  try ymodem_boot 1.2.1 photon
  sleep 10

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try set_country GB0
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon
  sleep 20

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_offline

  heading
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"
  echo "| 96 |   JP2   |   050   | DEFAULT |   121      | DEFAULT |   070/063/050   | re-test, online        |"
  echo "+----+---------+---------+---------+------------+---------+-----------------+------------------------+"

  enter_ymodem
  set_wifi_credentials

  enter_dfu_mode
  try set_country JP2
  exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try set_country DEFAULT
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 1.2.1 photon
  try exit_dfu_mode

  # wait for 1.2.1 bootloader from SMH
  sleep 10
  try ymodem_boot 1.2.1 photon
  sleep 10

  enter_dfu_mode
  try dfu_system 0.7.0 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.6.3 photon
  try exit_dfu_mode

  enter_dfu_mode
  try dfu_system 0.5.0 photon
  try exit_dfu_mode

  enter_ymodem
  try ymodem_boot 0.5.4 photon
  sleep 20

  try run_cli_list_subcommand_and_confirm_device_shows_up_as_online

fi





  echo "+-----------+------------+"
  echo "| LAST TEST | ALL PASSED |"
  echo "+-----------+------------+"
  pass
  exit 0;

  # # -----------------------------------------------------------------------------
  # # Test Matrix Iterator
  # # -----------------------------------------------------------------------------
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

main
