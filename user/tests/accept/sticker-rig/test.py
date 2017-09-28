# args:
# 1. hardware to USB serial port (connected to Serial1 on the device)
# 2. [optional] USB port corresponding to the Serial port on the device.
#  When specified, the device will be put in listening mode, otherwise the device should already be in listening
#  mode before running this script.


import serial
import sys
import time
import array

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

# Timeout changed from 1 to 20 seconds wait for slower responses from the device.
# Occasionally it takes more than 1-3 seconds for the modem to power up, and much
# longer if the modem is not present.
ser = serial.Serial(port, 9600, timeout=20)
code = bytearray([0xe1, 0x63, 0x57, 0x3f, 0xe7, 0x87, 0xc2, 0xa6, 0x85, 0x20, 0xa5, 0x6c, 0xe3, 0x04, 0x9e, 0xa0])
ser.flushInput()
ser.flushOutput()

# ser.write(code)
# writing the entire magic byte array was resulting in only the first byte
# being transmitted most of the time. Writing one byte at a time and flushing
# the serial device seems to be working every time now.
def writeBytes(data):
    for index, item in enumerate(data, start=0):
        ser.write(chr(item))
        ser.flush()
    # print(item)

def s2ba(data):
    return array.array('B',data)

def echolines():
    result = ""
    while True:
        line = ser.readline()
        result += line.decode('ascii')
        print(line)
        if len(line)==0:
            break
        # do not unconditionally wait for a serial timeout if we get the thing we're looking for
        if line.find("GOOD DAY, WIFI TESTER AT YOUR SERVICE!!!")==0:
            break
        if line.find("POWER IS ON")==0:
            break
        if line.find("POWER FAILED TO TURN ON!!!")==0:
            break
        if line.find("RSSI:")==0:
            break
    return result

# Write Magic Code to start the WiFi Tester
writeBytes(code)
echolines()

# Power on (required for Electron)
writeBytes(s2ba("POWER_ON:;"))
power = echolines()
if power.find("POWER IS ON")==-1:
    print("Expected POWER IS ON in output")
    print('Tests FAILED!!')
    sys.exit(1)

writeBytes(s2ba("INFO:;"))
info = echolines()

# wait a bit before closing port to ensure all data is sent
time.sleep(1)
ser.close()

failures = []

if info.find("RSSI:")==-1:
    failures += "Expected RSSI: in output"

if len(failures)>0:
    # print("%d failures" % (len(failures)))
    # for f in failures:
        # print(f)
    print('Tests FAILED!!')
    sys.exit(1)
else:
    print('Tests PASSED!!')
