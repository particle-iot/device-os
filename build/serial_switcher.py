#!/usr/bin/env python

import serial
import sys

baudRate = 14400
neutralBaudRate = 9600
portName = "/dev/ttyACM0"

if len(sys.argv) > 1:
  baudRate = int(sys.argv[1])

if len(sys.argv) > 2:
  portName = sys.argv[2]

ser = serial.Serial(portName, baudRate)
ser.close()

ser = serial.Serial(portName, neutralBaudRate)
ser.close()


