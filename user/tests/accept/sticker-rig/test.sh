#!/bin/bash 
# WiP for a bash implementation. Does not work, so gave up and coded in python which was much simpler.
serial=$1
if [ -z "$serial" ]; then
   echo "Please provide the serial port of the device to test"
   exit -1
fi

usbserial=$2

function readSerial() {
result=""
while read -r -t 2 line < $serial; do
  echo $line
  result=$result+$line
done
}

function writeSerial() {
	echo -en  "$1" > $serial	
}

function listeningMode() {
if [ ! -z "$usbserial" ]; then
   stty -f $usbserial speed 28800
   sleep 5
fi
}


listeningMode
stty -f $serial speed 9600
writeSerial "\xe1\x63\x57\x3f\xe7\x87\xc2\xa6\x85\x20\xa5\x6c\xe3\x04\x9e\xa0"
readSerial
writeSerial "INFO:;"
readSerial
