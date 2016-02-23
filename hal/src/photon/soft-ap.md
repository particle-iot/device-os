# Soft-AP setup protocol, firmware and test

## Flashing Pre-built binary

- the soft-ap.elf is a prebuilt binary using the steps below
- to flash this to the device, run

./tools/OpenOCD/<OS>/openocd-all-brcm-libftdi.exe -f ./tools/OpenOCD/BCM9WCD1EVAL1.cfg -f ./tools/OpenOCD/stm32f2x.cfg -f ./tools/OpenOCD/stm32f2x-flash-app.cfg -c "flash write_image erase soft-ap.elf" -c shutdown

- Windows
.\tools\OpenOCD\Win32\openocd-all-brcm-libftdi.exe -f ./tools/OpenOCD/BCM9WCD1EVAL1.cfg -f ./tools/OpenOCD/stm32f2x.cfg -f ./tools/OpenOCD/stm32f2x-flash-app.cfg -c "flash write_image erase soft-ap.elf" -c shutdown


## Building/Flashing firmware

- connect the Broadcom EVB to your computer via a USB cable
- in the root directory of this repo run
```
./make demo.soft_ap-BCM943362WCD4-ThreadX-NetX-SDIO download run
```
- this will build the firmwawre and flash it to your board. (you don't need JTAG for this.)
- The firmare starts in soft-ap listening mode. It switches to the configured AP as part of the `connect-ap` command.
- Hitting the reset button (the white one) reverts to soft-ap listening mode. The real firmware will of course not always start in soft-ap mode, it's like this to make testing and development easier.
- To test, run the python test script, described below.

## Soft AP Credentials
- Open, unsecured AP
- SSID is "photon-xxxxx" where xxxxx is part of the unique device ID
- IP 192.168.0.1
- Later, product creators might change the default SSID?
- Later, SSID will be photon-N, where the device first scans for existing networks, and sets N to 1,2,3 etc.. make the name unique?

## Endpoints
- TCP/5609 : simple protocol
 - request is:
  - command name [a-zA-z0-9-_]+ LF
  - number of bytes in body in ascii, e.g. '5' '7' for 57 bytes. LF
  - LF (empty line delimiter), followed by the JSON representation of the request data (if the command request requires one)
 - response:
 	- line delimited header data (if any) followed by an empty line (\n\n)
 	- command result as JSON
 - one request/response pair per socket connection

- HTTP/80 :
 - HTTP server on port 80
 - command name is the URL path, e.g. /device-id,
 - The requests and responses are the same as for the TCP/5809 protocol
 - Commands with no data are sent as a GET.
 - Commands that require data (e.g. configure-ap) are sent as POST, with the data in the request body
 - response is sent as the response body


# Commands

## version
- desc: retrieves a version identifier
- request data: none
- response data: { "v": 1 }

The version number allows clients to know implicitly which commands are available.
The current version is 1 and will remain that way until release. Then as new commands are
added, the version will be incremented. Backwards compatible changes do not require a change
to the version number.

## device-id
- desc: retrieves the unique device ID as a 24-digit hex string
- request data: none
- response data: { id: "string", c: "1" }

- id: is the unique ID for the device
- c: is the claimed flag. "1" if the device has previously been claimed. "0" if the device has never been claimed before.
The device is flagged as claimed after a claim ID has been set (see the `set` command below) and
the device has successfully connected to the cloud.

## scan-ap
- request-data: None
- response-data: { "scans" : [ { "ssid":"ssid-name", "rssi":-30, "sec":value, "ch":value, "mdr":value }, ... ]  }
- desc: scan access points and return their details. Returns all access points in the order they are scanned by the device.

## configure-ap
- request-data: { "idx" : index, "ssid" : "ssid-value", "pwd":"hex-encoded secret", "sec":security, "ch":channel }
- response-data: { "r": responseCode }  // 0 ok, non zero problem with index/data
- desc: configure the access point details to connect to when connect-ap is called.
The ap doesn't have to be in the list from scan-ap, allowing manual entry of hidden networks.

### Encrypted 'pwd' attribute

The `pwd` attribute carries the wifi password entered by the user, if one was given.
(If the password is empty, the pwd attribute can be omitted.)

The binary data bytes to encrypt is the password (no greater than 64 characters - if the user enters more than 64 bytes it is truncated)
The data block is then RSA encrypted with PKCS#1 padding scheme using the device's public key (see the `public-key` command below).

The encrypted bytes are then ascii hex-encoded, with the most significant nibble (4-bits) first,
followed by the lest significant nibble. For example, the value `161` would be encoded as `A1`.

The entire hex-encoded string (128-bytes of data, 256-bytes ascii hex-encoded) is then sent as the
value of the `pwd` attribute.

### `sec` attribute

The sec attribute describes the security configuration of the scanned AP. It corresponds exactly with the
`sec` attribute from the `scan-ap` command.

It's an enum with these values:

```cpp
	enum Security {
	    SECURITY_OPEN           = 0;          /**< Unsecured                               */
	    SECURITY_WEP_PSK        = 1;     	  /**< WEP Security with open authentication   */
	    SECURITY_WEP_SHARED     = 0x8001;     /**< WEP Security with shared authentication */
	    SECURITY_WPA_TKIP_PSK   = 0x00200002; /**< WPA Security with TKIP                  */
	    SECURITY_WPA_AES_PSK    = 0x00200004; /**< WPA Security with AES                   */
	    SECURITY_WPA2_AES_PSK   = 0x00400004; /**< WPA2 Security with AES                  */
	    SECURITY_WPA2_TKIP_PSK  = 0x00400002; /**< WPA2 Security with TKIP                 */
	    SECURITY_WPA2_MIXED_PSK = 0x00400006; /**< WPA2 Security with AES & TKIP           */
	}
```

The `mdr` attribute is maximum data rate for the SSID in kbits/s.

## connect-ap
- request-data: { "idx" : index }
- response-data: { "r": responseCode }  // 0 ok, non zero problem with index/data
- desc: Connects to an AP previously configured with configure-ap. This disconnects the soft-ap
after the response code has been sent. Note that the response code doesn't indicate successful
connection to the AP, but only that the command was acknoweldged and the AP will be connected to after
the result is sent to the client.

If the AP connection is unsuccessful, the soft-AP will be reinstated so the user can enter
new credentials/try again.

## public-key

- request: public-key
- response: { "r": responseCode, "b":" ascii hex-encoded data" }
- desc: fetches the device's public key in DER format (hex encoded)
- the response code is 0 if successful.

## set

A generic setter that allows different values in the firmware to be altered.

- request: set
- request-data: { "k" : "key", "v" : "value" }
- response: { "r" : responseCode }
- desc: sets the named key to the given value. The format of the value is determined by the key. These keys are
supported:

 - cc: sets the claim code. The claim code is 48-bytes, base-64 encoded, as 64 bytes.







# Proposed Commands (not implemented)

## provision-keys
- request: provision-keys { "salt": number, "force": 0|1 )
- response: generation status (0 keys generated, 1 not generated)
- desc:
 - salt: random data for help in computing the keys
 - force: provision new keys even if keys already stored

(Ideally this would be a non-blocking operation that continued in the background, but initially it will block for ca 6s)


## signal
- desc: enables/disables signalling on the device for easier identification
- request data: SignalRequest
- response data: ResponseCode

# Python Test Script

## Setup Deps

use `pip` or easy_install to grab these dependencies

- json - included with python 2.7.x iirc
- pyhamcrest

The python test script is at `apps/demo/soft_ap/test/softap_test.py`.
The test script requires the device-id and AP connection details. The simplest way to get the device ID is to run the test and let it fail, or to run

```
echo -e "device-id\n0" | nc 192.168.0.1 5609
```

Here's how to setup and run the test:

1. before the first run, edit the device-id and configured AP details at the top of the python script.
2. start the soft-ap firmware on the broadcom evaluation board
3. Verify you can see the photon AP and join your machine to it.
4. Open a serial monitor (e.g. putty) to the evaluation board, baud 115200
5. Reset the evaluation board, and verify you see a startup message in the serial monitor, including notification that the IP address for the soft-ap is 192.168.0.1
6. Run the `softap_test.py` python script. This will print the device ID, scanned APs and then connect to the configured AP.
7. In the serial monitor, the connection status is shown. If connection to the configured AP was successful, the new IP address will be shown.
8. To help verify that the wifi connection is working, an HTTP server is started after successful connection to the AP. Browse/curl `http://<connected-IP>/hello`
9. On reset, the evaluation board starts in soft-AP mode again so the process can be repeated.



# Firmware Design

(Just some notes to help flesh out the design.)

## Key Interfaces/Classes

### Command (interface)
- name
- execute(requestStream, responseStream) -> response code
- requestStream contains encoded request params (e.g. a protobuf message)
- responseStream filled with command response (e.g. protobuf message)

### ProtobufCommand (abstract class)
- implements execute(requestStream, responseStream) by decoding the request via protobuf to an object
- invoking process(object) -> object
- serialize reponse object using protobuf into responseStream

### Dispatcher (interface)
- takes an array of Commands
- examines incoming request by name and looks up appropriate command
- forwards request stream to command and provides output stream

### SerialDispatcher (class)
- performs dispatcher duties over Serial
- request format is: command name <newline>, request data length (in ascii) <newline> followed by request data
- response format is: response code <newline>, response data length (in ascii) <newline> followed by response data

### HTTPDispatcher (class)
- performs dispatcher duties over HTTP request/responses
- http path mapped to command name
- request body mapped to request body stream
- response code mapped from command response code
- response body from command response stream

### CoAPDispatcher (class)
- performs dispatcher duties over a TCP socket using CoAP library
- will flesh out details after looking at CoAP libraries

# Implementation

##  useful wiced details

- configure soft-AP name + security: defined using DCT. Will read and compare dct soft-ap block with what is expected and write if different.see network/LwIP/WICED/wiced_network.c#174 wiced_network_up() WICED_AP_INTERFACE,
- ap scanning -> WICED/internal/config_http_content.c, process_scan() line 363
- also process_connect() for storing dct info on configured wifi network

## Testing

- serial dispatcher for manual testing, perhaps also automated testing from python script
- for soft-AP testing implement HTTP dispatcher and test via http client in python script.
- On windows `netsh wlan connect <AP name>` can be used to connect to a specific AP




