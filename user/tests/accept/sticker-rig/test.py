# args:
# 1. hardware to USB serial port (connected to Serial1 on the device)
# 2. [optional] USB port corresponding to the Serial port on the device.
#  When specified, the device will be put in listening mode, otherwise the device should already be in listening
#  mode before running this script.


import serial
import sys
import time

port = None
if len(sys.argv) > 1:
    port = sys.argv[1]

listen = None
if len(sys.argv) > 2:
    listen = sys.argv[2]

if listen:
    l = serial.Serial(listen, 28800)
    l.close()
    time.sleep(5)

ser = serial.Serial(port, 9600, timeout=1)
code = bytearray([0xe1, 0x63, 0x57, 0x3f, 0xe7, 0x87, 0xc2, 0xa6, 0x85, 0x20, 0xa5, 0x6c, 0xe3, 0x04, 0x9e, 0xa0])
ser.write(code)

def echolines():
    result = ""
    while True:
        line = ser.readline()
        result += line.decode('ascii')
#        print(line)
        if len(line)==0:
            break
    return result

echolines()
ser.write(b"INFO:;")
info = echolines()
ser.close()

failures = []

if info.find("RSSI:")==-1:
    failures += "Expected RSSI: in output"


if len(failures)>0:
    print("%d failures" % (len(failures)))
    for f in failures:
        print(f)
    sys.exit(1)
else:
    print('Tests PASSED!!')


