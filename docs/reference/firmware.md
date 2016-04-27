---
title: Firmware
template: reference.hbs
columns: three
devices: [photon,electron,core]
order: 1
---

Particle Device Firmware
==========

## Cloud Functions

### Particle.variable()

Expose a *variable* through the Cloud so that it can be called with `GET /v1/devices/{DEVICE_ID}/{VARIABLE}`.
Returns a success value - `true` when the variable was registered.


```C++
// EXAMPLE USAGE

int analogvalue = 0;
double tempC = 0;
char *message = "my name is particle";
String aString;

void setup()
{
  // variable name max length is 12 characters long
  Particle.variable("analogvalue", analogvalue);
  Particle.variable("temp", tempC);
  if (Particle.variable("mess", message)==false)
  {
      // variable not registered!
  }
  Particle.variable("mess2", aString);

  pinMode(A0, INPUT);
}

void loop()
{
  // Read the analog value of the sensor (TMP36)
  analogvalue = analogRead(A0);
  //Convert the reading into degree celcius
  tempC = (((analogvalue * 3.3)/4095) - 0.5) * 100;
  delay(200);
}
```

Currently, up to 10 cloud variables may be defined and each variable name is limited to a maximum of 12 characters.

It is fine to call this function when the cloud is disconnected - the variable
will be registered next time the cloud is connected.

Prior to 0.4.7 firmware, variables were defined with an additional 3rd parameter
to specify the data type of the variable. From 0.4.7 onwards, the system can
infer the type from the actual variable. Additionally, the variable address
was passed via the address-of operator (`&`). With 0.4.7 and newer, this is no longer required.

This is the pre-0.4.7 syntax:

```
int analogvalue = 0;
double tempC = 0;
char *message = "my name is particle";

void setup()
{
  // variable name max length is 12 characters long
  Particle.variable("analogvalue", &analogvalue, INT);
  Particle.variable("temp", &tempC, DOUBLE);
  if (Particle.variable("mess", message, STRING)==false)
      // variable not registered!
  pinMode(A0, INPUT);
}
```

There are three supported data types:

 * `INT`
 * `DOUBLE`
 * `STRING`   (maximum string length is 622 bytes)




```json
# EXAMPLE REQUEST IN TERMINAL
# Device ID is 0123456789abcdef
# Your access token is 123412341234
curl "https://api.particle.io/v1/devices/0123456789abcdef/analogvalue?access_token=123412341234"
curl "https://api.particle.io/v1/devices/0123456789abcdef/temp?access_token=123412341234"
curl "https://api.particle.io/v1/devices/0123456789abcdef/mess?access_token=123412341234"

# In return you'll get something like this:
960
27.44322344322344
my name is particle

```

### Particle.function()

Expose a *function* through the Cloud so that it can be called with `POST /v1/devices/{DEVICE_ID}/{FUNCTION}`.

```cpp
// SYNTAX TO REGISTER A CLOUD FUNCTION
bool success = Particle.function("funcKey", funcName);
//                                  ^
//                                  |
//                     (max of 12 characters long)
```

Currently the application supports the creation of up to 4 different cloud functions.

In order to register a cloud  function, the user provides the `funcKey`, which is the string name used to make a POST request and a `funcName`, which is the actual name of the function that gets called in your app. The cloud function can return any integer; `-1` is commonly used for a failed function call.

The length of the `funcKey` is limited to a max of 12 characters. If you declare a function name longer than 12 characters the function will not be registered.

A cloud function is set up to take one argument of the [String](#string-class) datatype. This argument length is limited to a max of 63 characters.

```cpp
// EXAMPLE USAGE

int brewCoffee(String command);

void setup()
{
  // register the cloud function
  Particle.function("brew", brewCoffee);
}

void loop()
{
  // this loops forever
}

// this function automagically gets called upon a matching POST request
int brewCoffee(String command)
{
  // look for the matching argument "coffee" <-- max of 64 characters long
  if(command == "coffee")
  {
    // some example functions you might have
    //activateWaterHeater();
    //activateWaterPump();
    return 1;
  }
  else return -1;
}
```

---

You can expose a method on a C++ object to the Cloud.

```C++
// EXAMPLE USAGE WITH C++ OBJECT

class CoffeeMaker {
  public:
    CoffeeMaker() {
      Particle.function("brew", &CoffeeMaker::brew, this);
    }

    int brew(String command) {
      // do stuff
      return 1;
    }
};

CoffeeMaker myCoffeeMaker;
// nothing else needed in setup() or loop()
```

---

The API request will be routed to the device and will run your brew function. The response will have a return_value key containing the integer returned by brew.

```json
COMPLEMENTARY API CALL
POST /v1/devices/{DEVICE_ID}/{FUNCTION}

# EXAMPLE REQUEST
curl https://api.particle.io/v1/devices/0123456789abcdef/brew \
     -d access_token=123412341234 \
     -d "args=coffee"
```

### Particle.publish()

Publish an *event* through the Particle Cloud that will be forwarded to all registered listeners, such as callbacks, subscribed streams of Server-Sent Events, and other devices listening via `Particle.subscribe()`.

This feature allows the device to generate an event based on a condition. For example, you could connect a motion sensor to the device and have the device generate an event whenever motion is detected.

Cloud events have the following properties:

* name (1–63 ASCII characters)
* public/private (default public)
* ttl (time to live, 0–16777215 seconds, default 60)
  !! NOTE: The user-specified ttl value is not yet implemented, so changing this property will not currently have any impact.
* optional data (up to 255 bytes)

Anyone may subscribe to public events; think of them like tweets.
Only the owner of the device will be able to subscribe to private events.

A device may not publish events beginning with a case-insensitive match for "spark".
Such events are reserved for officially curated data originating from the Cloud.

Calling `Particle.publish()` when the device is not connected to the cloud will not
result in an event being published. This is indicated by the return success code
of `false`.

For the time being there exists no way to access a previously published but TTL-unexpired event.

**NOTE:** Currently, a device can publish at rate of about 1 event/sec, with bursts of up to 4 allowed in 1 second. Back to back burst of 4 messages will take 4 seconds to recover.

---

Publish a public event with the given name, no data, and the default TTL of 60 seconds.

```C++
// SYNTAX
Particle.publish(const char *eventName);
Particle.publish(String eventName);

RETURNS
boolean (true or false)

// EXAMPLE USAGE
bool success;
success = Particle.publish("motion-detected");
if (!success) {
  // get here if event publish did not work
}
```

---

Publish a public event with the given name and data, with the default TTL of 60 seconds.

```C++
// SYNTAX
Particle.publish(const char *eventName, const char *data);
Particle.publish(String eventName, String data);

// EXAMPLE USAGE
Particle.publish("temperature", "19 F");
```

---

Publish a public event with the given name, data, and TTL.

```C++
// SYNTAX
Particle.publish(const char *eventName, const char *data, int ttl);
Particle.publish(String eventName, String data, int ttl);

// EXAMPLE USAGE
Particle.publish("lake-depth/1", "28m", 21600);
```

---

Publish a private event with the given name, data, and TTL.
In order to publish a private event, you must pass all four parameters.

```C++
// SYNTAX
Particle.publish(const char *eventName, const char *data, int ttl, PRIVATE);
Particle.publish(String eventName, String data, int ttl, PRIVATE);

// EXAMPLE USAGE
Particle.publish("front-door-unlocked", NULL, 60, PRIVATE);
```

Publish a private event with the given name.

```C++
// SYNTAX
Particle.publish(const char *eventName, PRIVATE);
Particle.publish(String eventName, PRIVATE);

// EXAMPLE USAGE
Particle.publish("front-door-unlocked", PRIVATE);
```


```json
COMPLEMENTARY API CALL
GET /v1/events/{EVENT_NAME}

# EXAMPLE REQUEST
curl -H "Authorization: Bearer {ACCESS_TOKEN_GOES_HERE}" \
    https://api.particle.io/v1/events/motion-detected

# Will return a stream that echoes text when your event is published
event: motion-detected
data: {"data":"23:23:44","ttl":"60","published_at":"2014-05-28T19:20:34.638Z","deviceid":"0123456789abcdef"}
```

{{#if electron}}
*`NO_ACK` flag*

Unless specified otherwise, events sent to the cloud are sent as a reliable message. The Electoron waits for
acknowledgement from the cloud that the event has been recieved, resending the event in the background up to 3 times before giving up.

The `NO_ACK` flag disables this acknoweldge/retry behavior and sends the event only once.  This reduces data consumption per event, with the possibility that the event may not reach the cloud. 

For example, the `NO_ACK` flag could be useful when many events are sent (such as sensor readings) and the occaisonal lost event can be tolerated. 

```C++
// SYNTAX

int temperature = sensor.readTemperature();  // by way of example, not part of the API 
Particle.publish("t", temperature, NO_ACK);
Particle.publish("t", temperature, PRIVATE, NO_ACK);
Particle.publish("t", temperature, ttl, PRIVATE, NO_ACK);
```

{{/if}}


### Particle.subscribe()

Subscribe to events published by devices.

This allows devices to talk to each other very easily.  For example, one device could publish events when a motion sensor is triggered and another could subscribe to these events and respond by sounding an alarm.

```cpp
int i = 0;

void myHandler(const char *event, const char *data)
{
  i++;
  Serial.print(i);
  Serial.print(event);
  Serial.print(", data: ");
  if (data)
    Serial.println(data);
  else
    Serial.println("NULL");
}

void setup()
{
  Particle.subscribe("temperature", myHandler);
  Serial.begin(9600);
}
```

To use `Particle.subscribe()`, define a handler function and register it in `setup()`.


---

You can listen to events published only by your own devices by adding a `MY_DEVICES` constant.

```cpp
// only events from my devices
Particle.subscribe("the_event_prefix", theHandler, MY_DEVICES);
```

---

You can register a method in a C++ object as a subscription handler.

```cpp
class Subscriber {
  public:
   Subscriber() {
      Particle.subscribe("some_event", &Subscriber::handler, this);
    }
    void handler(const char *eventName, const char *data) {
      Serial.println(data);
    }
};

Subscriber mySubscriber;
// now nothing else is needed in setup() or loop()
```

---

A subscription works like a prefix filter.  If you subscribe to "foo", you will receive any event whose name begins with "foo", including "foo", "fool", "foobar", and "food/indian/sweet-curry-beans".

Received events will be passed to a handler function similar to `Particle.function()`.
A _subscription handler_ (like `myHandler` above) must return `void` and take two arguments, both of which are C strings (`const char *`).

- The first argument is the full name of the published event.
- The second argument (which may be NULL) is any data that came along with the event.

`Particle.subscribe()` returns a `bool` indicating success. It is ok to register a subscription when
the device is not connected to the cloud - the subscription is automatically registered
with the cloud next time the device connects.

**NOTE:** A device can register up to 4 event handlers. This means you can call `Particle.subscribe()` a maximum of 4 times; after that it will return `false`.

### Particle.unsubscribe()

Removes all subscription handlers previously registered with `Particle.subscribe()`.

```cpp
// SYNTAX
Particle.unsubscribe();
```

### Particle.connect()

`Particle.connect()` connects the device to the Cloud. This will automatically activate the Wi-Fi module and attempt to connect to a Wi-Fi network if the device is not already connected to a network.

```cpp
void setup() {}

void loop() {
  if (Particle.connected() == false) {
    Particle.connect();
  }
}
```

After you call `Particle.connect()`, your loop will not be called again until the device finishes connecting to the Cloud. Typically, you can expect a delay of approximately one second.

In most cases, you do not need to call `Particle.connect()`; it is called automatically when the device turns on. Typically you only need to call `Particle.connect()` after disconnecting with [`Particle.disconnect()`](#particle-disconnect-) or when you change the [system mode](#system-modes).


### Particle.disconnect()

`Particle.disconnect()` disconnects the device from the Cloud.

```C++
int counter = 10000;

void doConnectedWork() {
  digitalWrite(D7, HIGH);
  Serial.println("Working online");
}

void doOfflineWork() {
  digitalWrite(D7, LOW);
  Serial.println("Working offline");
}

bool needConnection() {
  --counter;
  if (0 == counter)
    counter = 10000;
  return (2000 > counter);
}

void setup() {
  pinMode(D7, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  if (needConnection()) {
    if (!Particle.connected())
      Particle.connect();
    doConnectedWork();
  } else {
    if (Particle.connected())
      Particle.disconnect();
    doOfflineWork();
  }
}
```

While this function will disconnect from the Cloud, it will keep the connection to the {{#unless electron}}Wi-Fi network. If you would like to completely deactivate the Wi-Fi module, use [`WiFi.off()`](#off-).{{/unless}}{{#if electron}}Cellular network. If you would like to completely deactivate the Cellular module, use [`Cellular.off()`](#off-).{{/if}}


**NOTE:* When the device is disconnected, many features are not possible, including over-the-air updates, reading Particle.variables, and calling Particle.functions.

*If you disconnect from the Cloud, you will NOT BE ABLE to flash new firmware over the air. A factory reset should resolve the issue.*

### Particle.connected()

Returns `true` when connected to the Cloud, and `false` when disconnected from the Cloud.

```C++
// SYNTAX
Particle.connected();

RETURNS
boolean (true or false)

// EXAMPLE USAGE
void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Particle.connected()) {
    Serial.println("Connected!");
  }
  delay(1000);
}
```

{{#if electron}}
### Particle.keepAlive()

Sets the duration between keep alive messages used to maintain the connection to the cloud.

```C++
// SYNTAX
Particle.keepAlive(23 * 60);	// send a ping every 23 minutes
```

A keep alive is used to implement "UDP hole punching" which helps maintain the connection from the cloud to the device.
Should a device becomes unreachable from the cloud (such as a timed out function call or variable get), 
one possible cause of this is that the keep alives have not been sent often enough.

The keep alive duration varies by mobile network operator. The default keepalive is set to 23 minutes, which is sufficient to maintain the connection on Particle SIM cards. 3rd party SIM cards will need to determine the appropriate keep alive value.


{{/if}}


### Particle.process()

Runs the background loop. This is the public API for the former internal function
`SPARK_WLAN_Loop()`.

`Particle.process()` checks the Wi-Fi module for incoming messages from the Cloud,
and processes any messages that have come in. It also sends keep-alive pings to the Cloud,
so if it's not called frequently, the connection to the Cloud may be lost.

Even in non-cloud-bound applications it can still be advisable to call `Particle.process()` to explicitly provide some processor time to the {{#unless electron}}Wi-Fi module (e.g. immediately after `WiFi.ready()` to update system variables).{{/unless}}{{#if electron}}Cellular module (e.g. immediately after `Cellular.ready()` to update system variables).{{/if}}

```cpp
void setup() {
  Serial.begin(9600);
}

void loop() {
  while (1) {
    Particle.process();
    redundantLoop();
  }
}

void redundantLoop() {
  Serial.println("Well that was unnecessary.");
}
```

`Particle.process()` is a blocking call, and blocks for a few milliseconds. `Particle.process()` is called automatically after every `loop()` and during delays. Typically you will not need to call `Particle.process()` unless you block in some other way and need to maintain the connection to the Cloud, or you change the [system mode](#system-modes). If the user puts the device into `MANUAL` mode, the user is responsible for calling `Particle.process()`. The more frequently this function is called, the more responsive the device will be to incoming messages, the more likely the Cloud connection will stay open, and the less likely that the Wi-Fi module's buffer will overrun.

### Particle.syncTime()

Synchronize the time with the Particle Cloud.
This happens automatically when the device connects to the Cloud.
However, if your device runs continuously for a long time,
you may want to synchronize once per day or so.

```C++
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long lastSync = millis();

void loop() {
  if (millis() - lastSync > ONE_DAY_MILLIS) {
    // Request time synchronization from the Particle Cloud
    Particle.syncTime();
    lastSync = millis();
  }
}
```

Note that this function sends a request message to the Cloud and then returns.
The time on the device will not be synchronized until some milliseconds later
when the Cloud responds with the current time between calls to your loop.

### Get Public IP

Using this feature, the device can programmatically know its own public IP address.

```cpp
// Open a serial terminal and see the IP address printed out
void handler(const char *topic, const char *data) {
    Serial.println("received " + String(topic) + ": " + String(data));
}

void setup() {
    Serial.begin(115200);
    for(int i=0;i<5;i++) {
        Serial.println("waiting... " + String(5 - i));
        delay(1000);
    }

    Particle.subscribe("spark/", handler);
    Particle.publish("spark/device/ip");
}
```


### Get Device name

This gives you the device name that is stored in the cloud,

```cpp
// Open a serial terminal and see the device name printed out
void handler(const char *topic, const char *data) {
    Serial.println("received " + String(topic) + ": " + String(data));
}

void setup() {
    Serial.begin(115200);
    for(int i=0;i<5;i++) {
        Serial.println("waiting... " + String(5 - i));
        delay(1000);
    }

    Particle.subscribe("spark/", handler);
    Particle.publish("spark/device/name");
}
```

### Get Random seed

Grab 40 bytes of randomness from the cloud and {e}n{c}r{y}p{t} away!

```cpp
void handler(const char *topic, const char *data) {
    Serial.println("received " + String(topic) + ": " + String(data));
}

void setup() {
    Serial.begin(115200);
    for(int i=0;i<5;i++) {
        Serial.println("waiting... " + String(5 - i));
        delay(1000);
    }

    Particle.subscribe("spark/", handler);
    Particle.publish("spark/device/random");
}
```

{{#unless electron}}
## WiFi

### on()

`WiFi.on()` turns on the Wi-Fi module. Useful when you've turned it off, and you changed your mind.

Note that `WiFi.on()` does not need to be called unless you have changed the [system mode](#system-modes) or you have previously turned the Wi-Fi module off.

### off()

`WiFi.off()` turns off the Wi-Fi module. Useful for saving power, since most of the power draw of the device is the Wi-Fi module.

### connect()

Attempts to connect to the Wi-Fi network. If there are no credentials stored, this will enter listening mode (see below for how to avoid this.). If there are credentials stored, this will try the available credentials until connection is successful. When this function returns, the device may not have an IP address on the LAN; use `WiFi.ready()` to determine the connection status.

```cpp
// SYNTAX
WiFi.connect();
```

_Since 0.4.5_
It's possible to call `WiFi.connect()` without entering listening mode in the case where no credentials are stored:

```cpp
// SYNTAX
WiFi.connect(WIFI_CONNECT_NO_LISTEN);
```

If there are no credentials then the call does nothing other than turn on the WiFi module. 


### disconnect()

Disconnects from the Wi-Fi network, but leaves the Wi-Fi module on.

```cpp
// SYNTAX
WiFi.disconnect();
```

### connecting()

This function will return `true` once the device is attempting to connect using stored Wi-Fi credentials, and will return `false` once the device has successfully connected to the Wi-Fi network.

```cpp
// SYNTAX
WiFi.connecting();
```

### ready()

This function will return `true` once the device is connected to the network and has been assigned an IP address, which means that it's ready to open TCP sockets and send UDP datagrams. Otherwise it will return `false`.

```cpp
// SYNTAX
WiFi.ready();
```

{{#if photon}}
### selectAntenna()

Selects which antenna the device should connect to Wi-Fi with and remembers that
setting until it is changed.

```cpp
// SYNTAX
STARTUP(WiFi.selectAntenna(ANT_INTERNAL)); // selects the CHIP antenna
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL)); // selects the u.FL antenna
STARTUP(WiFi.selectAntenna(ANT_AUTO)); // continually switches at high speed between antennas
```

`WiFi.selectAntenna()` selects one of three antenna modes on your Photon or P1.  It takes one argument: `ANT_AUTO`, `ANT_INTERNAL` or `ANT_EXTERNAL`.
`WiFi.selectAntenna()` must be used inside another function like STARTUP(), setup(), or loop() to compile.

You may specify in code which antenna to use as the default at boot time using the STARTUP() macro.

> Note that the antenna selection is remembered even after power off or when entering safe mode.
This is to allow your device to be configured once and then continue to function with the
selected antenna when applications are flashed that don't specify which antenna to use.

This ensures that devices which must use the external antenna continue to use the external
antenna in all cases even when the application code isn't being executed (e.g. safe mode.)

If no antenna has been previously selected, the `ANT_INTERNAL` antenna will be chosen by default.

`WiFi.selectAntenna()` returns 0 on success, or -1005 if the antenna choice was not found.
Other errors that may appear will all be negative values.

```cpp
// Use the STARTUP() macro to set the default antenna
// to use system boot time.
// In this case it would be set to the chip antenna
STARTUP(WiFi.selectAntenna(ANT_INTERNAL));

void setup() {
  // your setup code
}

void loop() {
  // your loop code
}
```




{{/if}}

### listen()

This will enter or exit listening mode, which opens a Serial connection to get Wi-Fi credentials over USB, and also listens for credentials over
{{#if core}}Smart Config{{/if}}{{#if photon}}Soft AP{{/if}}.

```cpp
// SYNTAX - enter listening mode
WiFi.listen();
```

Listening mode blocks application code. Advanced cases that use multithreading, interrupts, or system events
have the ability to continue to execute application code while in listening mode, and may wish to then exit listening
mode, such as after a timeout. Listening mode is stopped using this syntax:

```cpp

// SYNTAX - exit listening mode
WiFi.listen(false);

```



### listening()

```cpp
// SYNTAX
WiFi.listening();
```

{{#if core}}
Because listening mode blocks your application code on the Core, this command is not useful on the Core.
It will always return `false`.
{{/if}}
{{#if photon}}
Right now, this command is not useful, always returning `false`, because listening mode blocks application code.

This command becomes useful on the Photon and Electron when system code runs as a separate RTOS task from application code.
We estimate that firmware feature will be released for the Photon in September 2015.

Once system code does not block application code,
`WiFi.listening()` will return `true` once `WiFi.listen()` has been called
or the setup button has been held for 3 seconds,
when the RGB LED should be blinking blue.
It will return `false` when the device is not in listening mode.
{{/if}}


### setCredentials()

Allows the application to set credentials for the Wi-Fi network from within the code. These credentials will be added to the device's memory, and the device will automatically attempt to connect to this network in the future.

Your device can remember more than one set of credentials:
- Core: remembers the 7 most recently set credentials
- Photon: remembers the 5 most recently set credentials

```cpp
// Connects to an unsecured network.
WiFi.setCredentials(SSID);
WiFi.setCredentials("My_Router_Is_Big");

// Connects to a network secured with WPA2 credentials.
WiFi.setCredentials(SSID, PASSWORD);
WiFi.setCredentials("My_Router", "mypasswordishuge");

// Connects to a network with a specified authentication procedure.
// Options are WPA2, WPA, or WEP.
WiFi.setCredentials(SSID, PASSWORD, AUTH);
WiFi.setCredentials("My_Router", "wepistheworst", WEP);

```

{{#if photon}}
When the Photon used with hidden or offline networks, the security cipher is also required.

```cpp

// for hidden and offline networks on the Photon, the security cipher is also needed
// Cipher options are WLAN_CIPHER_AES, WLAN_CIPHER_TKIP and WLAN_CIPHER_AES_TKIP
WiFi.setCredentials("SSID", "PASSWORD", WPA2, WLAN_CIPHER_AES));
```

{{/if}}

**Note:** In order for `WiFi.setCredentials()` to work, the WiFi module needs to be on (if switched off or disabled via non_AUTOMATIC SYSTEM_MODEs call `WiFi.on()`).

### getCredentials()

*Since 0.4.9.*

Lists the Wi-Fi networks with credentials stored on the device. Returns the number of stored networks.

Note that this returns details about the Wi-Fi networks, but not the actual password.

{{#if core}}

*Core: always returns 0 since Wi-Fi credentials cannot be read back from the CC3000 Wi-Fi module.*

{{else}}

```cpp
// EXAMPLE
WiFiAccessPoint ap[5];
int found = WiFi.getCredentials(ap, 5);
for (int i = 0; i < found; i++) {
    Serial.print("ssid: ");
    Serial.println(ap[i].ssid);
    // security is one of WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA, WLAN_SEC_WPA2
    Serial.print("security: ");
    Serial.println(ap[i].security);
    // cipher is one of WLAN_CIPHER_AES, WLAN_CIPHER_TKIP
    Serial.print("cipher: ");
    Serial.println(ap[i].cipher);
}
```

{{/if}}

### clearCredentials()

This will clear all saved credentials from the Wi-Fi module's memory. This will return `true` on success and `false` if the Wi-Fi module has an error.

```cpp
// SYNTAX
WiFi.clearCredentials();
```

### hasCredentials()

Will return `true` if there are Wi-Fi credentials stored in the Wi-Fi module's memory.

```cpp
// SYNTAX
WiFi.hasCredentials();
```

### macAddress()

`WiFi.macAddress()` returns the MAC address of the device.

```cpp
// EXAMPLE USAGE

byte mac[6];

void setup() {
  WiFi.on();
  Serial.begin(9600);
  while (!Serial.available()) Particle.process();

  WiFi.macAddress(mac);

  for (int i=0; i<6; i++) {
    if (i) Serial.print(":");
    Serial.print(mac[i], HEX);
  }
}
```

```cpp
// EXAMPLE USAGE

// Only for Spark Core using firmware < 0.4.0
// Mac address is in the reversed order and
// is fixed from V0.4.0 onwards

byte mac[6];

void setup() {
  Serial.begin(9600);
  while (!Serial.available()) Particle.process();

  WiFi.macAddress(mac);

  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
}

void loop() {}
```

### SSID()

`WiFi.SSID()` returns the SSID of the network the device is currently connected to as a `char*`.

### BSSID()

`WiFi.BSSID()` retrives the 6-byte MAC address of the access point the device is currently connected to.

byte bssid[6];

void setup() {
  Serial.begin(9600);
  while (!Serial.available()) Particle.process();

  WiFi.BSSID(bssid);
  Serial.printlnf("%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
}


### RSSI()

`WiFi.RSSI()` returns the signal strength of a Wi-Fi network from from -127 (weak) to -1dB (strong) as an `int`. Positive return values indicate an error with 1 indicating a Wi-Fi chip error and 2 indicating a time-out error.

```cpp
// SYNTAX
WiFi.RSSI();
```

### ping()

`WiFi.ping()` allows you to ping an IP address and returns the number of packets received as an `int`. It takes two forms:

`WiFi.ping(IPAddress remoteIP)` takes an `IPAddress` and pings that address.

`WiFi.ping(IPAddress remoteIP, uint8_t nTries)` and pings that address a specified number of times.

### scan()

Returns information about access points within range of the device.

The first form is the simplest, but also least flexible. You provide a
array of `WiFiAccessPoint` instances, and the call to `WiFi.scan()` fills out the array.
If there are more APs detected than will fit in the array, they are dropped.
Returns the number of access points written to the array.

```cpp
// EXAMPLE - retrieve up to 20 Wi-Fi APs

WiFiAccessPoint aps[20];
int found = WiFi.scan(aps, 20);
for (int i=0; i<found; i++) {
    WiFiAccessPoint& ap = aps[i];
    Serial.print("SSID: ");
    Serial.println(ap.ssid);
    Serial.print("Security: ");
    Serial.println(ap.security);
    Serial.print("Channel: ");
    Serial.println(ap.channel);
    Serial.print("RSSI: ");
    Serial.println(ap.rssi);
}
```

The more advanced call to `WiFi.scan()` uses a callback function that receives
each scanned access point.

```
// EXAMPLE using a callback
void wifi_scan_callback(WiFiAccessPoint* wap, void* data)
{
    WiFiAccessPoint& ap = *wap;
    Serial.print("SSID: ");
    Serial.println(ap.ssid);
    Serial.print("Security: ");
    Serial.println(ap.security);
    Serial.print("Channel: ");
    Serial.println(ap.channel);
    Serial.print("RSSI: ");
    Serial.println(ap.rssi);
}

void loop()
{
    int result_count = WiFi.scan(wifi_scan_callback);
    Serial.print(result_count);
    Serial.println(" APs scanned.");
}
```

The main reason for doing this is that you gain access to all access points available
without having to know in advance how many there might be.

You can also pass a 2nd parameter to `WiFi.scan()` after the callback, which allows
object-oriented code to be used.

```
// EXAMPLE - class to find the strongest AP

class FindStrongestSSID
{
    char strongest_ssid[33];
    int strongest_rssi;

    // This is the callback passed to WiFi.scan()
    // It makes the call on the `self` instance - to go from a static
    // member function to an instance member function.
    static void handle_ap(WiFiAccessPoint* wap, FindStrongestSSID* self)
    {
        self->next(*wap);
    }

    // determine if this AP is stronger than the strongest seen so far
    void next(WiFiAccessPoint& ap)
    {
        if ((ap.rssi < 0) && (ap.rssi > strongest_rssi)) {
            strongest_rssi = ap.rssi;
            strcpy(strongest_ssid, ap.ssid);
        }
    }

public:

    /**
     * Scan Wi-Fi Access Points and retrieve the strongest one.
     */
    const char* scan()
    {
        // initialize data
        strongest_rssi = 0;
        strongest_ssid[0] = 0;
        // perform the scan
        WiFi.scan(handle_ap, this);
        return strongest_ssid;
    }
};

// Now use the class
FindStrongestSSID strongestFinder;
const char* ssid = strongestFinder.scan();

}
```

### resolve()

`WiFi.resolve()` finds the IP address for a domain name.

```cpp
// SYNTAX
ip = WiFi.resolve(name);
```

Parameters:

- `name`: the domain name to resolve (string)

It returns the IP address if the domain name is found, otherwise a blank IP address.

```cpp
// EXAMPLE USAGE

IPAddress ip;
void setup() {
   ip = WiFi.resolve("www.google.com");
   if(ip) {
     // IP address was resolved
   } else {
     // name resolution failed
   }
}
```

### localIP()

`WiFi.localIP()` returns the local IP address assigned to the device as an `IPAddress`.

```cpp

void setup() {
  Serial.begin(9600);
  while(!Serial.available()) Particle.process();

  // Prints out the local IP over Serial.
  Serial.println(WiFi.localIP());
}
```

### subnetMask()

`WiFi.subnetMask()` returns the subnet mask of the network as an `IPAddress`.

```cpp

void setup() {
  Serial.begin(9600);
  while(!Serial.available()) Particle.process();

  // Prints out the subnet mask over Serial.
  Serial.println(WiFi.subnetMask());
}
```

### gatewayIP()

`WiFi.gatewayIP()` returns the gateway IP address of the network as an `IPAddress`.

```cpp

void setup() {
  Serial.begin(9600);
  while(!Serial.available()) Particle.process();

  // Prints out the gateway IP over Serial.
  Serial.println(WiFi.gatewayIP());
}
```

### dnsServerIP()

`WiFi.dnsServerIP()` retrieves the IP address of the DNS server that resolves
DNS requests for the device's network connection.

Note that for this value to be available requires calling `Particle.process()` after Wi-Fi
has connected.


### dhcpServerIP()

`WiFi.dhcpServerIP()` retrieves the IP address of the DHCP server that manages
the IP address used by the device's network connection.

Note that for this value to be available requires calling `Particle.process()` after Wi-Fi
has connected.


{{#if photon}}

### setStaticIP()

Defines the static IP addresses used by the system to connect to the network when static IP is activated.

```cpp
// SYNTAX

void setup() {
    IPAddress myAddress(192,168,1,100);
    IPAddress netmask(255,255,255,0);
    IPAddress gateway(192,168,1,1);
    IPAddress dns(192,168,1,1);
    WiFi.setStaticIP(myAddress, netmask, gateway, dns);

    // now let's use the configured IP
    WiFi.useStaticIP();
}

```

The addresses are stored persistently so that they are available in all subsequent
application and also in safe mode.


### useStaticIP()

Instructs the system to connect to the network using the IP addresses provided to
`WiFi.setStaticIP()`

The setting is persistent and is remembered until `WiFi.useDynamicIP()` is called.

### useDynamicIP()

Instructs the system to connect to the network using a dynamically allocated IP
address from the router.

A note on switching between static and dynamic IP. If static IP addresses have been previously configured using `WiFi.setStaticIP()`, they continue to be remembered
by the system after calling `WiFi.useDynamicIP()`, and so are available for use next time `WiFi.useStaticIP()`
is called, without needing to be reconfigured using `WiFi.setStaticIP()`

{{/if}}
{{/unless}}


{{#if photon}}
## SoftAP HTTP Pages

_Since 0.5.0_

When the device is in listening mode, it creates a temporary access point (AP) and a HTTP server on port 80. The HTTP server is used to configure the Wi-Fi access points the device attempts to connect to. As well as the system providing HTTP URLs, applications can add their own pages to the
SoftAP HTTP server.

SoftAP HTTP Pages is presently an advanced feature, requiring moderate C++ knowledge.  To being using the feature:

- add `#pragma SPARK_NO_PREPROCESSOR` to the top of your sketch
- add `#include "Particle.h"` below that, then
- add `#include "softap_http.h"` below that still


```cpp
// SYNTAX

void myPages(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result, void* reserved);

STARTUP(softap_set_application_page_handler(myPages, nullptr));
``` 

The `softap_set_application_page_handler` is set during startup. When the system is in setup mode, and a request is made for an unknown URL, the system
calls the page handler function provided by the application (here, `myPages`.)

The page handler function is called whenver an unknown URL is requested. It is called with these parameters:

- `url`: the path of the file requested by the client. It doesn't include the server name or port. Examples: `/index`,  `/someimage.jpg`.
- `cb`: a response callback - this is used by the application to indicate the type of HTTP response, such as 200 (OK) or 404 (not found). More on this below.
- `cbArg`: data that should be passed as the first parameter to the callback function `cb`. 
- `body`: a reader object that the page handler uses to retrieve the HTTP request body
- `result`: a writer object that the page handler uses to write the HTTP response body
- `reserved`: reserved for future expansion. Will be equal to `nullptr` and can be ignored.

The application MUST call the page callback function `cb` to provide a response for the requested page. If the requested page url isn't recognized by the application, then a 404 response should be sent, as described below. 

### The page callback function

When your page handler function is called, the system passes a result callback function as the `cb` parameter. 
The callback function takes these parameters:

- `cbArg`: this is the `cbArg` parameter passed to your page callback function. It's internal state used by the HTTP server. 
- `flags`: presently unused. Set to 0.
- `status`: the HTTP status code, as an integer, such as 200 for `OK`, or 404 for `page not found`.
- `mime-type`: the mime-type of the response as a string, such as `text/html` or `application/javascript`. 
- `header`: an optional pointer to a `Header` that is added to the response sent to the client. 

For example, to send a "not found" error for a page that is not recognized, your application code would call

```cpp
//  EXAMPLE - send a 404 response for an unknown page
cb(cbArg, 0, 404, "text/plain", nullptr);
```


### Retrieving the request data

When the HTTP request contains a request body (such as with a POST request), the `Reader` object provided by the `body` parameter can be used
to retrieve the request data. 

```cpp
// EXAMPLE

if (body->bytes_left) {
	char* data = body->read_as_string();
	// handle the body data
 	dostuff(data);
 	// free the data! IMPORTANT!
 	free(data);
}

```

### Sending a response

When sending a page, the page function responds with a HTTP 200 code, meaning the content was found, followed by the page data. 

```
// EXAMPLE - send a page

if (!stricmp(url, '/helloworld') {
	// send the response code 200, the mime type "text/html"
	cb(cbArg, 0, 200, "text/html", nullptr);
	// send the page content
	result->write("<h2>hello world!</h2>");
}
```

### The default page

When a browser requests the default page (`http://192.168.0.1/`) the system internally redirects this to `/index` so that it can be handled
by the application. 

The application may provide an actual page at `/index` or redirect to another page if the application pefers to have another page as its launch page.

### Sending a Redirect

The application can send a redirect response for a given page in order to manage the URL namespace, such as providing aliases for some resources.

The code below sends a redirect from the default page `/index` to `/index.html`

```cpp
// EXAMPLE - redirect from /index to /index.html
// add this code in the page hanler function

if (strcmp(url,"/index")==0) {
    Header h("Location: /index.html\r\n");
    cb(cbArg, 0, 301, "text/plain", &h);
    return;
}

```

### Complete Example

Here's a complete example providing a Web UI for setting up WiFi via HTTP. Credit for the HTTP pages goes to Github user @mebrunet!



```
#pragma SPARK_NO_PREPROCESSOR

#include "Particle.h"
#include "softap_http.h"

struct Page
{
    const char* url;
    const char* mime_type;
    const char* data;
};


const char index_html[] = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Setup your device</title><link rel='stylesheet' type='text/css' href='style.css'></head><body><h1>Connect me to your WiFi!</h1><h3>My device ID:</h3><input type=text id='device-id' size='25' value='' disabled/><button type='button' class='input-helper' id='copy-button'>Copy</button><div id='scan-div'><h3>Scan for visible WiFi networks</h3><button id='scan-button' type='button'>Scan</button></div><div id='networks-div'></div><div id='connect-div' style='display: none'><p>Don't see your network? Move me closer to your router, then re-scan.</p><form id='connect-form'><input type='password' id='password' size='25' placeholder='password'/><button type='button' class='input-helper' id='show-button'>Show</button><button type='submit' id='connect-button'>Connect</button></form></div><script src='rsa-utils/jsbn_1.js'></script><script src='rsa-utils/jsbn_2.js'></script><script src='rsa-utils/prng4.js'></script><script src='rsa-utils/rng.js'></script><script src='rsa-utils/rsa.js'></script><script src='script.js'></script></body></html>";

const char rsa_js[] = "function parseBigInt(a,b){return new BigInteger(a,b);}function linebrk(a,b){var c='';var d=0;while(d+b<a.length){c+=a.substring(d,d+b)+'\\n';d+=b;}return c+a.substring(d,a.length);}function byte2Hex(a){if(a<0x10)return '0'+a.toString(16);else return a.toString(16);}function pkcs1pad2(a,b){if(b<a.length+11){alert('Message too long for RSA');return null;}var c=new Array();var d=a.length-1;while(d>=0&&b>0){var e=a.charCodeAt(d--);if(e<128)c[--b]=e;else if((e>127)&&(e<2048)){c[--b]=(e&63)|128;c[--b]=(e>>6)|192;}else{c[--b]=(e&63)|128;c[--b]=((e>>6)&63)|128;c[--b]=(e>>12)|224;}}c[--b]=0;var f=new SecureRandom();var g=new Array();while(b>2){g[0]=0;while(g[0]==0)f.nextBytes(g);c[--b]=g[0];}c[--b]=2;c[--b]=0;return new BigInteger(c);}function RSAKey(){this.n=null;this.e=0;this.d=null;this.p=null;this.q=null;this.dmp1=null;this.dmq1=null;this.coeff=null;}function RSASetPublic(a,b){if(a!=null&&b!=null&&a.length>0&&b.length>0){this.n=parseBigInt(a,16);this.e=parseInt(b,16);}else alert('Invalid RSA public key');}function RSADoPublic(a){return a.modPowInt(this.e,this.n);}function RSAEncrypt(a){var b=pkcs1pad2(a,(this.n.bitLength()+7)>>3);if(b==null)return null;var c=this.doPublic(b);if(c==null)return null;var d=c.toString(16);if((d.length&1)==0)return d;else return '0'+d;}RSAKey.prototype.doPublic=RSADoPublic;RSAKey.prototype.setPublic=RSASetPublic;RSAKey.prototype.encrypt=RSAEncrypt;";

const char style_css[] = "html{height:100%;margin:auto;background-color:white}body{box-sizing:border-box;min-height:100%;padding:20px;background-color:#1aabe0;font-family:'Lucida Sans Unicode','Lucida Grande',sans-serif;font-weight:normal;color:white;margin-top:0;margin-left:auto;margin-right:auto;margin-bottom:0;max-width:400px;text-align:center;border:1px solid #6e6e70;border-radius:4px}div{margin-top:25px;margin-bottom:25px}h1{margin-top:25px;margin-bottom:25px}button{border-color:#1c75be;background-color:#1c75be;color:white;border-radius:5px;height:30px;font-size:15px;font-weight:bold}button.input-helper{background-color:#bebebe;border-color:#bebebe;color:#6e6e70;margin-left:3px}button:disabled{background-color:#bebebe;border-color:#bebebe;color:white}input[type='text'],input[type='password']{background-color:white;color:#6e6e70;border-color:white;border-radius:5px;height:25px;text-align:center;font-size:15px}input:disabled{background-color:#bebebe;border-color:#bebebe}input[type='radio']{position:relative;bottom:-0.33em;margin:0;border:0;height:1.5em;width:15%}label{padding-top:7px;padding-bottom:7px;padding-left:5%;display:inline-block;width:80%;text-align:left}input[type='radio']:checked+label{font-weight:bold;color:#1c75be}.scanning-error{font-weight:bold;text-align:center}.radio-div{box-sizing:border-box;margin:2px;margin-left:auto;margin-right:auto;background-color:white;color:#6e6e70;border:1px solid #6e6e70;border-radius:3px;width:100%;padding:5px}#networks-div{margin-left:auto;margin-right:auto;text-align:left}#device-id{text-align:center}#scan-button{min-width:100px}#connect-button{display:block;min-width:100px;margin-top:10px;margin-left:auto;margin-right:auto;margin-bottom:20px}#password{margin-top:20px;margin-bottom:10px}";

const char rng_js[] = "var rng_state;var rng_pool;var rng_pptr;function rng_seed_int(a){rng_pool[rng_pptr++]^=a&255;rng_pool[rng_pptr++]^=(a>>8)&255;rng_pool[rng_pptr++]^=(a>>16)&255;rng_pool[rng_pptr++]^=(a>>24)&255;if(rng_pptr>=rng_psize)rng_pptr-=rng_psize;}function rng_seed_time(){rng_seed_int(new Date().getTime());}if(rng_pool==null){rng_pool=new Array();rng_pptr=0;var t;if(window.crypto&&window.crypto.getRandomValues){var ua=new Uint8Array(32);window.crypto.getRandomValues(ua);for(t=0;t<32;++t)rng_pool[rng_pptr++]=ua[t];}if(navigator.appName=='Netscape'&&navigator.appVersion<'5'&&window.crypto){var z=window.crypto.random(32);for(t=0;t<z.length;++t)rng_pool[rng_pptr++]=z.charCodeAt(t)&255;}while(rng_pptr<rng_psize){t=Math.floor(65536*Math.random());rng_pool[rng_pptr++]=t>>>8;rng_pool[rng_pptr++]=t&255;}rng_pptr=0;rng_seed_time();}function rng_get_byte(){if(rng_state==null){rng_seed_time();rng_state=prng_newstate();rng_state.init(rng_pool);for(rng_pptr=0;rng_pptr<rng_pool.length;++rng_pptr)rng_pool[rng_pptr]=0;rng_pptr=0;}return rng_state.next();}function rng_get_bytes(a){var b;for(b=0;b<a.length;++b)a[b]=rng_get_byte();}function SecureRandom(){}SecureRandom.prototype.nextBytes=rng_get_bytes;";

const char jsbn_2_js[] = "function bnpRShiftTo(a,b){b.s=this.s;var c=Math.floor(a/this.DB);if(c>=this.t){b.t=0;return;}var d=a%this.DB;var e=this.DB-d;var f=(1<<d)-1;b[0]=this[c]>>d;for(var g=c+1;g<this.t;++g){b[g-c-1]|=(this[g]&f)<<e;b[g-c]=this[g]>>d;}if(d>0)b[this.t-c-1]|=(this.s&f)<<e;b.t=this.t-c;b.clamp();}function bnpSubTo(a,b){var c=0,d=0,e=Math.min(a.t,this.t);while(c<e){d+=this[c]-a[c];b[c++]=d&this.DM;d>>=this.DB;}if(a.t<this.t){d-=a.s;while(c<this.t){d+=this[c];b[c++]=d&this.DM;d>>=this.DB;}d+=this.s;}else{d+=this.s;while(c<a.t){d-=a[c];b[c++]=d&this.DM;d>>=this.DB;}d-=a.s;}b.s=(d<0)?-1:0;if(d<-1)b[c++]=this.DV+d;else if(d>0)b[c++]=d;b.t=c;b.clamp();}function bnpMultiplyTo(a,b){var c=this.abs(),d=a.abs();var e=c.t;b.t=e+d.t;while(--e>=0)b[e]=0;for(e=0;e<d.t;++e)b[e+c.t]=c.am(0,d[e],b,e,0,c.t);b.s=0;b.clamp();if(this.s!=a.s)BigInteger.ZERO.subTo(b,b);}function bnpSquareTo(a){var b=this.abs();var c=a.t=2*b.t;while(--c>=0)a[c]=0;for(c=0;c<b.t-1;++c){var d=b.am(c,b[c],a,2*c,0,1);if((a[c+b.t]+=b.am(c+1,2*b[c],a,2*c+1,d,b.t-c-1))>=b.DV){a[c+b.t]-=b.DV;a[c+b.t+1]=1;}}if(a.t>0)a[a.t-1]+=b.am(c,b[c],a,2*c,0,1);a.s=0;a.clamp();}function bnpDivRemTo(a,b,c){var d=a.abs();if(d.t<=0)return;var e=this.abs();if(e.t<d.t){if(b!=null)b.fromInt(0);if(c!=null)this.copyTo(c);return;}if(c==null)c=nbi();var f=nbi(),g=this.s,h=a.s;var i=this.DB-nbits(d[d.t-1]);if(i>0){d.lShiftTo(i,f);e.lShiftTo(i,c);}else{d.copyTo(f);e.copyTo(c);}var j=f.t;var k=f[j-1];if(k==0)return;var l=k*(1<<this.F1)+((j>1)?f[j-2]>>this.F2:0);var m=this.FV/l,n=(1<<this.F1)/l,o=1<<this.F2;var p=c.t,q=p-j,r=(b==null)?nbi():b;f.dlShiftTo(q,r);if(c.compareTo(r)>=0){c[c.t++]=1;c.subTo(r,c);}BigInteger.ONE.dlShiftTo(j,r);r.subTo(f,f);while(f.t<j)f[f.t++]=0;while(--q>=0){var s=(c[--p]==k)?this.DM:Math.floor(c[p]*m+(c[p-1]+o)*n);if((c[p]+=f.am(0,s,c,q,0,j))<s){f.dlShiftTo(q,r);c.subTo(r,c);while(c[p]<--s)c.subTo(r,c);}}if(b!=null){c.drShiftTo(j,b);if(g!=h)BigInteger.ZERO.subTo(b,b);}c.t=j;c.clamp();if(i>0)c.rShiftTo(i,c);if(g<0)BigInteger.ZERO.subTo(c,c);}function bnMod(a){var b=nbi();this.abs().divRemTo(a,null,b);if(this.s<0&&b.compareTo(BigInteger.ZERO)>0)a.subTo(b,b);return b;}function Classic(a){this.m=a;}function cConvert(a){if(a.s<0||a.compareTo(this.m)>=0)return a.mod(this.m);else return a;}function cRevert(a){return a;}function cReduce(a){a.divRemTo(this.m,null,a);}function cMulTo(a,b,c){a.multiplyTo(b,c);this.reduce(c);}function cSqrTo(a,b){a.squareTo(b);this.reduce(b);}Classic.prototype.convert=cConvert;Classic.prototype.revert=cRevert;Classic.prototype.reduce=cReduce;Classic.prototype.mulTo=cMulTo;Classic.prototype.sqrTo=cSqrTo;function bnpInvDigit(){if(this.t<1)return 0;var a=this[0];if((a&1)==0)return 0;var b=a&3;b=(b*(2-(a&0xf)*b))&0xf;b=(b*(2-(a&0xff)*b))&0xff;b=(b*(2-(((a&0xffff)*b)&0xffff)))&0xffff;b=(b*(2-a*b%this.DV))%this.DV;return(b>0)?this.DV-b:-b;}function Montgomery(a){this.m=a;this.mp=a.invDigit();this.mpl=this.mp&0x7fff;this.mph=this.mp>>15;this.um=(1<<(a.DB-15))-1;this.mt2=2*a.t;}function montConvert(a){var b=nbi();a.abs().dlShiftTo(this.m.t,b);b.divRemTo(this.m,null,b);if(a.s<0&&b.compareTo(BigInteger.ZERO)>0)this.m.subTo(b,b);return b;}function montRevert(a){var b=nbi();a.copyTo(b);this.reduce(b);return b;}function montReduce(a){while(a.t<=this.mt2)a[a.t++]=0;for(var b=0;b<this.m.t;++b){var c=a[b]&0x7fff;var d=(c*this.mpl+(((c*this.mph+(a[b]>>15)*this.mpl)&this.um)<<15))&a.DM;c=b+this.m.t;a[c]+=this.m.am(0,d,a,b,0,this.m.t);while(a[c]>=a.DV){a[c]-=a.DV;a[++c]++;}}a.clamp();a.drShiftTo(this.m.t,a);if(a.compareTo(this.m)>=0)a.subTo(this.m,a);}function montSqrTo(a,b){a.squareTo(b);this.reduce(b);}function montMulTo(a,b,c){a.multiplyTo(b,c);this.reduce(c);}Montgomery.prototype.convert=montConvert;Montgomery.prototype.revert=montRevert;Montgomery.prototype.reduce=montReduce;Montgomery.prototype.mulTo=montMulTo;Montgomery.prototype.sqrTo=montSqrTo;function bnpIsEven(){return((this.t>0)?(this[0]&1):this.s)==0;}function bnpExp(a,b){if(a>0xffffffff||a<1)return BigInteger.ONE;var c=nbi(),d=nbi(),e=b.convert(this),f=nbits(a)-1;e.copyTo(c);while(--f>=0){b.sqrTo(c,d);if((a&(1<<f))>0)b.mulTo(d,e,c);else{var g=c;c=d;d=g;}}return b.revert(c);}function bnModPowInt(a,b){var c;if(a<256||b.isEven())c=new Classic(b);else c=new Montgomery(b);return this.exp(a,c);}BigInteger.prototype.copyTo=bnpCopyTo;BigInteger.prototype.fromInt=bnpFromInt;BigInteger.prototype.fromString=bnpFromString;BigInteger.prototype.clamp=bnpClamp;BigInteger.prototype.dlShiftTo=bnpDLShiftTo;BigInteger.prototype.drShiftTo=bnpDRShiftTo;BigInteger.prototype.lShiftTo=bnpLShiftTo;BigInteger.prototype.rShiftTo=bnpRShiftTo;BigInteger.prototype.subTo=bnpSubTo;BigInteger.prototype.multiplyTo=bnpMultiplyTo;BigInteger.prototype.squareTo=bnpSquareTo;BigInteger.prototype.divRemTo=bnpDivRemTo;BigInteger.prototype.invDigit=bnpInvDigit;BigInteger.prototype.isEven=bnpIsEven;BigInteger.prototype.exp=bnpExp;BigInteger.prototype.toString=bnToString;BigInteger.prototype.negate=bnNegate;BigInteger.prototype.abs=bnAbs;BigInteger.prototype.compareTo=bnCompareTo;BigInteger.prototype.bitLength=bnBitLength;BigInteger.prototype.mod=bnMod;BigInteger.prototype.modPowInt=bnModPowInt;BigInteger.ZERO=nbv(0);BigInteger.ONE=nbv(1);";

const char jsbn_1_js[] = "var dbits;var canary=0xdeadbeefcafe;var j_lm=((canary&0xffffff)==0xefcafe);function BigInteger(a,b,c){if(a!=null)if('number'==typeof a)this.fromNumber(a,b,c);else if(b==null&&'string'!=typeof a)this.fromString(a,256);else this.fromString(a,b);}function nbi(){return new BigInteger(null);}function am1(a,b,c,d,e,f){while(--f>=0){var g=b*this[a++]+c[d]+e;e=Math.floor(g/0x4000000);c[d++]=g&0x3ffffff;}return e;}function am2(a,b,c,d,e,f){var g=b&0x7fff,h=b>>15;while(--f>=0){var i=this[a]&0x7fff;var j=this[a++]>>15;var k=h*i+j*g;i=g*i+((k&0x7fff)<<15)+c[d]+(e&0x3fffffff);e=(i>>>30)+(k>>>15)+h*j+(e>>>30);c[d++]=i&0x3fffffff;}return e;}function am3(a,b,c,d,e,f){var g=b&0x3fff,h=b>>14;while(--f>=0){var i=this[a]&0x3fff;var j=this[a++]>>14;var k=h*i+j*g;i=g*i+((k&0x3fff)<<14)+c[d]+e;e=(i>>28)+(k>>14)+h*j;c[d++]=i&0xfffffff;}return e;}if(j_lm&&(navigator.appName=='Microsoft Internet Explorer')){BigInteger.prototype.am=am2;dbits=30;}else if(j_lm&&(navigator.appName!='Netscape')){BigInteger.prototype.am=am1;dbits=26;}else{BigInteger.prototype.am=am3;dbits=28;}BigInteger.prototype.DB=dbits;BigInteger.prototype.DM=((1<<dbits)-1);BigInteger.prototype.DV=(1<<dbits);var BI_FP=52;BigInteger.prototype.FV=Math.pow(2,BI_FP);BigInteger.prototype.F1=BI_FP-dbits;BigInteger.prototype.F2=2*dbits-BI_FP;var BI_RM='0123456789abcdefghijklmnopqrstuvwxyz';var BI_RC=new Array();var rr,vv;rr='0'.charCodeAt(0);for(vv=0;vv<=9;++vv)BI_RC[rr++]=vv;rr='a'.charCodeAt(0);for(vv=10;vv<36;++vv)BI_RC[rr++]=vv;rr='A'.charCodeAt(0);for(vv=10;vv<36;++vv)BI_RC[rr++]=vv;function int2char(a){return BI_RM.charAt(a);}function intAt(a,b){var c=BI_RC[a.charCodeAt(b)];return(c==null)?-1:c;}function bnpCopyTo(a){for(var b=this.t-1;b>=0;--b)a[b]=this[b];a.t=this.t;a.s=this.s;}function bnpFromInt(a){this.t=1;this.s=(a<0)?-1:0;if(a>0)this[0]=a;else if(a<-1)this[0]=a+this.DV;else this.t=0;}function nbv(a){var b=nbi();b.fromInt(a);return b;}function bnpFromString(a,b){var c;if(b==16)c=4;else if(b==8)c=3;else if(b==256)c=8;else if(b==2)c=1;else if(b==32)c=5;else if(b==4)c=2;else{this.fromRadix(a,b);return;}this.t=0;this.s=0;var d=a.length,e=false,f=0;while(--d>=0){var g=(c==8)?a[d]&0xff:intAt(a,d);if(g<0){if(a.charAt(d)=='-')e=true;continue;}e=false;if(f==0)this[this.t++]=g;else if(f+c>this.DB){this[this.t-1]|=(g&((1<<(this.DB-f))-1))<<f;this[this.t++]=(g>>(this.DB-f));}else this[this.t-1]|=g<<f;f+=c;if(f>=this.DB)f-=this.DB;}if(c==8&&(a[0]&0x80)!=0){this.s=-1;if(f>0)this[this.t-1]|=((1<<(this.DB-f))-1)<<f;}this.clamp();if(e)BigInteger.ZERO.subTo(this,this);}function bnpClamp(){var a=this.s&this.DM;while(this.t>0&&this[this.t-1]==a)--this.t;}function bnToString(a){if(this.s<0)return '-'+this.negate().toString(a);var b;if(a==16)b=4;else if(a==8)b=3;else if(a==2)b=1;else if(a==32)b=5;else if(a==4)b=2;else return this.toRadix(a);var c=(1<<b)-1,d,e=false,f='',g=this.t;var h=this.DB-(g*this.DB)%b;if(g-->0){if(h<this.DB&&(d=this[g]>>h)>0){e=true;f=int2char(d);}while(g>=0){if(h<b){d=(this[g]&((1<<h)-1))<<(b-h);d|=this[--g]>>(h+=this.DB-b);}else{d=(this[g]>>(h-=b))&c;if(h<=0){h+=this.DB;--g;}}if(d>0)e=true;if(e)f+=int2char(d);}}return e?f:'0';}function bnNegate(){var a=nbi();BigInteger.ZERO.subTo(this,a);return a;}function bnAbs(){return(this.s<0)?this.negate():this;}function bnCompareTo(a){var b=this.s-a.s;if(b!=0)return b;var c=this.t;b=c-a.t;if(b!=0)return(this.s<0)?-b:b;while(--c>=0)if((b=this[c]-a[c])!=0)return b;return 0;}function nbits(a){var b=1,c;if((c=a>>>16)!=0){a=c;b+=16;}if((c=a>>8)!=0){a=c;b+=8;}if((c=a>>4)!=0){a=c;b+=4;}if((c=a>>2)!=0){a=c;b+=2;}if((c=a>>1)!=0){a=c;b+=1;}return b;}function bnBitLength(){if(this.t<=0)return 0;return this.DB*(this.t-1)+nbits(this[this.t-1]^(this.s&this.DM));}function bnpDLShiftTo(a,b){var c;for(c=this.t-1;c>=0;--c)b[c+a]=this[c];for(c=a-1;c>=0;--c)b[c]=0;b.t=this.t+a;b.s=this.s;}function bnpDRShiftTo(a,b){for(var c=a;c<this.t;++c)b[c-a]=this[c];b.t=Math.max(this.t-a,0);b.s=this.s;}function bnpLShiftTo(a,b){var c=a%this.DB;var d=this.DB-c;var e=(1<<d)-1;var f=Math.floor(a/this.DB),g=(this.s<<c)&this.DM,h;for(h=this.t-1;h>=0;--h){b[h+f+1]=(this[h]>>d)|g;g=(this[h]&e)<<c;}for(h=f-1;h>=0;--h)b[h]=0;b[f]=g;b.t=this.t+f+1;b.s=this.s;b.clamp();}";

const char script_js[] = "var base_url='http://192.168.0.1/';var network_list;var public_key;var rsa=new RSAKey();var scanButton=document.getElementById('scan-button');var connectButton=document.getElementById('connect-button');var copyButton=document.getElementById('copy-button');var showButton=document.getElementById('show-button');var deviceID=document.getElementById('device-id');var connectForm=document.getElementById('connect-form');var public_key_callback={success:function(a){console.log('Public key: '+a.b);public_key=a.b;rsa.setPublic(public_key.substring(58,58+256),public_key.substring(318,318+6));},error:function(a,b){console.log(a);window.alert('There was a problem fetching important information from your device. Please verify your connection, then reload this page.');}};var device_id_callback={success:function(a){var b=a.id;deviceID.value=b;},error:function(a,b){console.log(a);var c='COMMUNICATION_ERROR';deviceID.value=c;}};var scan=function(){console.log('Scanning...!');disableButtons();scanButton.innerHTML='Scanning...';connectButton.innerHTML='Connect';document.getElementById('connect-div').style.display='none';document.getElementById('networks-div').style.display='none';getRequest(base_url+'scan-ap',scan_callback);};var scan_callback={success:function(a){network_list=a.scans;console.log('I found:');var b=document.getElementById('networks-div');b.innerHTML='';if(network_list.length>0)for(var c=0;c<network_list.length;c++){ssid=network_list[c].ssid;console.log(network_list[c]);add_wifi_option(b,ssid);document.getElementById('connect-div').style.display='block';}else b.innerHTML='<p class=\\'scanning-error\\'>No networks found.</p>';},error:function(a){console.log('Scanning error:'+a);document.getElementById('networks-div').innerHTML='<p class=\\'scanning-error\\'>Scanning error.</p>';},regardless:function(){scanButton.innerHTML='Re-Scan';enableButtons();document.getElementById('networks-div').style.display='block';}};var configure=function(a){a.preventDefault();var b=get_selected_network();var c=document.getElementById('password').value;if(!b){window.alert('Please select a network!');return false;}var d={idx:0,ssid:b.ssid,pwd:rsa.encrypt(c),sec:b.sec,ch:b.ch};connectButton.innerHTML='Sending credentials...';disableButtons();console.log('Sending credentials: '+JSON.stringify(d));postRequest(base_url+'configure-ap',d,configure_callback);};var configure_callback={success:function(a){console.log('Credentials received.');connectButton.innerHTML='Credentials received...';postRequest(base_url+'connect-ap',{idx:0},connect_callback);},error:function(a,b){console.log('Configure error: '+a);window.alert('The configuration command failed, check that you are still well connected to the device\\'s WiFi hotspot and retry.');connectButton.innerHTML='Retry';enableButtons();}};var connect_callback={success:function(a){console.log('Attempting to connect to the cloud.');connectButton.innerHTML='Attempting to connect...';window.alert('Your device should now start flashing green and attempt to connect to the cloud. This usually takes about 20 seconds, after which it will begin slowly blinking cyan. \\n\\n\\nIf this process fails because you entered the wrong password, the device will flash green indefinitely. In this case, hold the setup button for 6 seconds until the device starts blinking blue again. Then reconnect to the WiFi hotspot it generates and reload this page to try again.');},error:function(a,b){console.log('Connect error: '+a);window.alert('The connect command failed, check that you are still well connected to the device\\'s WiFi hotspot and retry.');connectButton.innerHTML='Retry';enableButtons();}};var disableButtons=function(){connectButton.disabled=true;scanButton.disabled=true;};var enableButtons=function(){connectButton.disabled=false;scanButton.disabled=false;};var add_wifi_option=function(a,b){var c=document.createElement('INPUT');c.type='radio';c.value=b;c.id=b;c.name='ssid';c.className='radio';var d=document.createElement('DIV');d.className='radio-div';d.appendChild(c);var e=document.createElement('label');e.htmlFor=b;e.innerHTML=b;d.appendChild(e);a.appendChild(d);};var get_selected_network=function(){for(var a=0;a<network_list.length;a++){ssid=network_list[a].ssid;if(document.getElementById(ssid).checked)return network_list[a];}};var copy=function(){window.prompt('Copy to clipboard: Ctrl + C, Enter',deviceID.value);};var toggleShow=function(){var a=document.getElementById('password');inputType=a.type;if(inputType==='password'){showButton.innerHTML='Hide';a.type='text';}else{showButton.innerHTML='Show';a.type='password';}};var getRequest=function(a,b){var c=new XMLHttpRequest();c.open('GET',a,true);c.timeout=8000;c.send();c.onreadystatechange=function(){if(c.readyState==4)if(b){if(c.status==200){if(b.success)b.success(JSON.parse(c.responseText));}else if(b.error)b.error(c.status,c.responseText);if(b.regardless)b.regardless();}};};var postRequest=function(a,b,c){var d=JSON.stringify(b);var e=new XMLHttpRequest();e.open('POST',a,true);e.timeout=4000;e.setRequestHeader('Content-Type','multipart/form-data');e.send(d);e.onreadystatechange=function(){if(e.readyState==4)if(c){if(e.status==200){if(c.success)c.success(JSON.parse(e.responseText));}else if(c.error)c.error(e.status,e.responseText);if(c.regardless)c.regardless();}};};if(scanButton.addEventListener){copyButton.addEventListener('click',copy);showButton.addEventListener('click',toggleShow);scanButton.addEventListener('click',scan);connectForm.addEventListener('submit',configure);}else if(scanButton.attachEvent){copyButton.attachEvent('onclick',copy);showButton.attachEvent('onclick',toggleShow);scanButton.attachEvent('onclick',scan);connectForm.attachEvent('onsubmit',configure);}getRequest(base_url+'device-id',device_id_callback);getRequest(base_url+'public-key',public_key_callback);";

const char prng4_js[] = "function Arcfour(){this.i=0;this.j=0;this.S=new Array();}function ARC4init(a){var b,c,d;for(b=0;b<256;++b)this.S[b]=b;c=0;for(b=0;b<256;++b){c=(c+this.S[b]+a[b%a.length])&255;d=this.S[b];this.S[b]=this.S[c];this.S[c]=d;}this.i=0;this.j=0;}function ARC4next(){var a;this.i=(this.i+1)&255;this.j=(this.j+this.S[this.i])&255;a=this.S[this.i];this.S[this.i]=this.S[this.j];this.S[this.j]=a;return this.S[(a+this.S[this.i])&255];}Arcfour.prototype.init=ARC4init;Arcfour.prototype.next=ARC4next;function prng_newstate(){return new Arcfour();}var rng_psize=256;";

Page myPages[] = {
     { "/index.html", "text/html", index_html },
     { "/rsa-utils/rsa.js", "application/javascript", rsa_js },
     { "/style.css", "text/css", style_css },
     { "/rsa-utils/rng.js", "application/javascript", rng_js },
     { "/rsa-utils/jsbn_2.js", "application/javascript", jsbn_2_js },
     { "/rsa-utils/jsbn_1.js", "application/javascript", jsbn_1_js },
     { "/script.js", "application/javascript", script_js },
     { "/rsa-utils/prng4.js", "application/javascript", prng4_js },
     { nullptr }
};

void myPage(const char* url, ResponseCallback* cb, void* cbArg, Reader* body, Writer* result, void* reserved)
{
    Serial.printlnf("handling page %s", url);

    if (strcmp(url,"/index")==0) {
        Serial.println("sending redirect");
        Header h("Location: /index.html\r\n");
        cb(cbArg, 0, 301, "text/plain", &h);
        return;
    }

    int8_t idx = 0;
    for (;;idx++) {
        Page& p = myPages[idx];
        if (!p.url) {
            idx = -1;
            break;
        }
        else if (strcmp(url, p.url)==0) {
            break;
        }
    }

    if (idx==-1) {
        cb(cbArg, 0, 404, nullptr, nullptr);
    }
    else {
        cb(cbArg, 0, 200, myPages[idx].mime_type, nullptr);
        result->write(myPages[idx].data);
    }
}

STARTUP(softap_set_application_page_handler(myPage, nullptr));
```

{{/if}}


{{#if electron}}
## Cellular

### on()

`Cellular.on()` turns on the Cellular module. Useful when you've turned it off, and you changed your mind.

Note that `Cellular.on()` does not need to be called unless you have changed the [system mode](#system-modes) or you have previously turned the Cellular module off.  When turning on the Cellular module, it will go through a full re-connect to the Cellular network which will take anywhere from 30 to 60 seconds in most situations.

```cpp
// SYNTAX
Cellular.on();
```

### off()

`Cellular.off()` turns off the Cellular module. Useful for saving power, since most of the power draw of the device is the Cellular module.  Note: turning off the Cellular module will force it to go through a full re-connect to the Cellular network the next time it is turned on.

```cpp
// SYNTAX
Cellular.off();
```

### connect()

Attempts to connect to the Cellular network. If there are no credentials entered, the default Particle APN for Particle SIM cards will be used.  If no SIM card is inserted, the Electron will enter listening mode. If a 3rd party APN is set, these credentials must match the inserted SIM card for the Electron to connect to the cellular network. When this function returns, the device may not have a local (private) IP address; use `Cellular.ready()` to determine the connection status.

```cpp
// SYNTAX
Cellular.connect();
```

### disconnect()

Disconnects from the Cellular network, but leaves the Cellular module on.

```cpp
// SYNTAX
Cellular.disconnect();
```

### connecting()

This function will return `true` once the device is attempting to connect using the default Particle APN or 3rd party APN cellular credentials, and will return `false` once the device has successfully connected to the cellular network.

```cpp
// SYNTAX
Cellular.connecting();
```

### ready()

This function will return `true` once the device is connected to the cellular network and has been assigned an IP address, which means it has an activated PDP context and is ready to open TCP/UDP sockets. Otherwise it will return `false`.

```cpp
// SYNTAX
Cellular.ready();
```

### listen()

This will enter or exit listening mode, which opens a Serial connection to get Cellular information such as the IMEI or CCID over USB.

```cpp
// SYNTAX - enter listening mode
Cellular.listen();
```

Listening mode blocks application code. Advanced cases that use multithreading, interrupts, or system events
have the ability to continue to execute application code while in listening mode, and may wish to then exit listening
mode, such as after a timeout. Listening mode is stopped using this syntax:

```cpp

// SYNTAX - exit listening mode
Cellular.listen(false);

```

### listening()

```cpp
// SYNTAX
Cellular.listening();
```

Without multithreading enabled, this command is not useful (always returning `false`) because listening mode blocks application code.

This command becomes useful on the Electron when system code runs as a separate RTOS task from application code.

Once system code does not block application code,
`Cellular.listening()` will return `true` once `Cellular.listen()` has been called
or the setup button has been held for 3 seconds, when the RGB LED should be blinking blue.
It will return `false` when the device is not in listening mode.


### setCredentials()

**Note**: `Cellular.setCredentials()` is not currently enabled, however read on to find out how to use the cellular_hal to do the same thing with `cellular_credentials_set()`.

Sets 3rd party credentials for the Cellular network from within the user application. These credentials are currently not added to the device's non-volatile memory and need to be set every time from the user application.  You may set credentials in 3 different ways:

- APN only
- USERNAME & PASSWORD only
- APN, USERNAME & PASSWORD

**Note**: When using the default `SYSTEM_MODE(AUTOMATIC)` connection behavior, it is necessary to call `cellular_credentials_set()` with the `STARTUP()` macro outside of `setup()` and `loop()` so that the system will have the correct credentials before it tries to connect to the cellular network (see EXAMPLE).

The following examples can be copied to a file called `setcreds.ino` and compiled and flashed to your Electron over USB via the [Particle CLI](/guide/tools-and-features/cli).  With your Electron in [DFU mode](/guide/getting-started/modes/electron/#dfu-mode-device-firmware-upgrade-), the command for this is:

`particle compile electron setcreds.ino --saveTo firmware.bin && particle flash --usb firmware.bin`

**Note**: Your Electron only uses one set of credentials, and they must be correctly matched to the SIM card that's used.  If using a Particle SIM, using `cellular_credentials_set()` is not necessary as the default APN of "spark.telefonica.com" with no username or password will be used by system firmware. To switch back to using a Particle SIM after successfully connecting with a 3rd Party SIM, just flash any app that does not include cellular_credentials_set().  Then ensure you completely power cycle the Electron to remove the settings from the modem’s volatile memory.

```C++
// SYNTAX
// Connects to a cellular network by APN only
STARTUP(cellular_credentials_set(APN, "", "", NULL));

// Connects to a cellular network with USERNAME and PASSWORD only
STARTUP(cellular_credentials_set("", USERNAME, PASSWORD, NULL));

// Connects to a cellular network with a specified APN, USERNAME and PASSWORD
#include “cellular_hal.h”
STARTUP(cellular_credentials_set(APN, USERNAME, PASSWORD, NULL));
```

```C++
// EXAMPLE - an AT&T APN with no username or password in AUTOMATIC mode

#include "cellular_hal.h"
STARTUP(cellular_credentials_set("broadband", "", "", NULL));

void setup() {
  // your setup code
}

void loop() {
  // your loop code
}
```

### getDataUsage()

A software implementation of Data Usage that pulls sent and received session and total bytes from the cellular modem's internal counters.  The sent / received bytes are the gross payload evaluated by the protocol stack, therefore they comprise the TCP and IP header bytes and the packets used to open and close the TCP connection.  I.e., these counters account for all overhead in cellular communications.

**Note**: Cellular.getDataUsage() should only be used for relative measurements on data at runtime.  Do not rely upon these figures for absolute and total data usage of your SIM card.

**Note**: There is a known issue with Sara U260/U270 modem firmware not being able to set/reset the internal counters (which does work on the Sara G350).  Because of this limitation, the set/reset function is implemented in software, using the modem's current count as the baseline.

**Note**: The internal modem counters are typically reset when the modem is power cycled (complete power removal, soft power down or Cellular.off()) or if the PDP context is deactivated and reactivated which can happen asynchronously during runtime. If the Cellular.getDataUsage() API has been read, reset or set, and then the modem's counters are reset for any reason, the next call to Cellular.getDataUsage() for a read will detect that the new reading would be less than the previous reading.  When this is detected, the current reading will remain the same, and the now lower modem count will be used as the new baseline.  Because of this mechanism, it is generally more accurate to read the getDataUsage() count often. This catches the instances when the modem count is reset, before the count starts to increase again.

To use the data usage API, an instance of the `CellularData` type needs to be created to read or set counters.  All data usage API functions and the CellularData object itself return `bool` - `true` indicating the last operation was successful and the CellularData object was updated. For set and get functions, `CellularData` is passed by reference `Cellular.dataUsage(CellularData&);` and updated by the function.  There are 5 integers and 1 boolean within the CellularData object:

- **ok**: (bool) a boolean value `false` when the CellularData object is initially created, and `true` after the object has been successfully updated by the API. If the last reading failed and the counters were not changed from their previous value, this value is set to `false`.
- **cid**: (int) a value from 0-255 that is the index of the current PDP context that the data usage counters are valid for.  If this number is -1, the data usage counters have either never been initially set, or the last reading failed and the counters were not changed from their previous value.
- **tx_session**: (int) number of bytes sent for the current session
- **rx_session**: (int) number of bytes sent for the current session
- **tx_total**: (int) number of bytes sent total (typical equals the session numbers)
- **rx_total**: (int) number of bytes sent total (typical equals the session numbers)

CellularData is a Printable object, so using it directly with `Serial.println(data);` will be output as follows:

```
cid,tx_session,rx_session,tx_total,rx_total
31,1000,300,1000,300
```

```c++
// SYNTAX
// Read Data Usage
CellularData data;
Cellular.getDataUsage(data);
```

```c++
// EXAMPLE

void setup()
{
  Serial.begin(9600);
}

void loop()
{
    if (Serial.available() > 0)
    {
        char c = Serial.read();
        if (c == '1') {
            Serial.println("Read counters of sent or received PSD data!");
            CellularData data;
            if (!Cellular.getDataUsage(data)) {
                Serial.print("Error! Not able to get data.");
            }
            else {
                Serial.printlnf("CID: %d SESSION TX: %d RX: %d TOTAL TX: %d RX: %d",
                    data.cid,
                    data.tx_session, data.rx_session,
                    data.tx_total, data.rx_total);
                Serial.println(data); // Printable
            }
        }
        else if (c == '2') {
            Serial.println("Set all sent/received PSD data counters to 1000!");
            CellularData data;
            data.tx_session = 1000;
            data.rx_session = 1000;
            data.tx_total = 1000;
            data.rx_total = 1000;
            if (!Cellular.setDataUsage(data)) {
                Serial.print("Error! Not able to set data.");
            }
            else {
                Serial.printlnf("CID: %d SESSION TX: %d RX: %d TOTAL TX: %d RX: %d",
                    data.cid,
                    data.tx_session, data.rx_session,
                    data.tx_total, data.rx_total);
                Serial.println(data); // Printable
            }
        }
        else if (c == '3') {
            Serial.println("Reset counter of sent/received PSD data!");
            if (!Cellular.resetDataUsage()) {
                Serial.print("Error! Not able to reset data.");
            }
        }
        else if (c == 'p') {
            Serial.println("Publish some data!");
            Particle.publish("1","a");
        }
        while (Serial.available()) Serial.read(); // Flush the input buffer
    }

}
```

### setDataUsage()

Sets the Data Usage counters to the values indicated in the supplied CellularData object.

Returns `bool` - `true` indicating this operation was successful and the CellularData object was updated.

```c++
// SYNTAX
// Set Data Usage
CellularData data;
Cellular.setDataUsage(data);
```

### resetDataUsage()

Resets the Data Usage counters to all zero.  No CellularData object is required.  This is handy to call just before an operation where you'd like to measure data usage.

Returns `bool` - `true` indicating this operation was successful and the internally stored software offset has been reset to zero. If getDataUsage() was called immediately after without any data being used, the CellularData object would indicate zero data used.

```c++
// SYNTAX
// Reset Data Usage
Cellular.dataUsageReset();
```

### RSSI()

`Cellular.RSSI()` returns a struct of type `CellularSignal` that contains two integers: the signal strength (`rssi`) and signal quality (`qual`) of the currently connected Cellular network.

`CellularSignal`
- `rssi`: (`int`) is the signal strength with range -113dBm to -51dBm (in 2dBm steps). This variable also doubles as an error response for the entire struct; positive return values indicate an error with:
    - 1: indicating a Cellular module or time-out error
    - 2: the module responded that the rssi value is not known, not detectable or currently not available.
- `qual`: (`int`) is a number in UMTS RAT indicating the Energy per Chip/Noise ratio in dB levels of the current cell. This value ranges from 0 to 49, higher numbers indicate higher signal quality.

**Note**: `qual` is not supported on 2G Electrons (Model G350) and will return 0.

```C++
// SYNTAX

CellularSignal sig = Cellular.RSSI();
Serial.println(sig.rssi);
Serial.println(sig.qual);
Serial.println(sig); // Complete structure also printable

// Output
-95    // RSSI (-dBm)
19     // QUAL (dB)
-95,19 // RSSI,QUAL

// EXAMPLE
uint32_t lastUpdate;
#define now millis()

void setup()
{
  Serial.begin(9600);
  lastUpdate = now;
}

void loop()
{
  // Print two methods of signal strength and signal quality every 20 seconds
  if (now - lastUpdate > 20000UL) {
    lastUpdate = now;
    CellularSignal sig = Cellular.RSSI();
    String s = String(sig.rssi) + String(",") + String(sig.qual);
    Serial.print(s);
    Serial.print(" is the same as ");
    Serial.println(sig);
  }
}
```


bool Cellular.getBandSelect(CellularBand &data_get);

### getBandAvailable()
*Since 0.5.0.*

Gets the cellular bands currently available in the modem.  `Bands` are the carrier frequncies used to communicate with the cellular network.  Some modems have 2 bands available (U260/U270) and others have 4 bands (G350).

To use the band select API, an instance of the `CellularBand` type needs to be created to read or set bands.  All band select API functions and the CellularBand object itself return `bool` - `true` indicating the last operation was successful and the CellularBand object was updated. For set and get functions, `CellularBand` is passed by reference `Cellular.getBandSelect(CellularBand&);` and updated by the function.  There is 1 array, 1 integer, 1 boolean and 1 helper function within the CellularBand object:

- **ok**: (bool) a boolean value `false` when the CellularBand object is initially created, and `true` after the object has been successfully updated by the API. If the last reading failed and the bands were not changed from their previous value, this value is set to `false`.
- **count**: (int) a value from 0-5 that is the number of currently selected bands retrieved from the modem after getBandAvailble() or getBandSelect() is called successfully.
- **band[5]**: (MDM_Band[]) array of up to 5 MDM_Band enumerated types.  Available enums are: `BAND_DEFAULT, BAND_0, BAND_700, BAND_800, BAND_850, BAND_900, BAND_1500, BAND_1700, BAND_1800, BAND_1900, BAND_2100, BAND_2600`.  All elements set to 0 when CellularBand object is first created, but after getBandSelect() is called successfully the currently selected bands will be populated started with index 0, i.e., (`.band[0]`). Can be 5 values when getBandAvailable() is called on a G350 modem, as it will return factory default value of 0 as an available option, i.e., `0,850,900,1800,1900`.
- **bool isBand(int)**: helper function built into the CellularBand type that can be used to check if an integer is a valid band.  This is helpful if you would like to test a value before manually setting a band in the .band[] array.

CellularBand is a Printable object, so using it directly with `Serial.println(CellularBand);` will print the number of bands that are retreived from the modem.  This will be output as follows:

```
// EXAMPLE PRINTABLE
CellularBand band_sel;
// populate band object with fake data
band_sel.band[0] = BAND_850;
band_sel.band[1] = BAND_1900;
band_sel.count = 2;
Serial.println(band_sel);

// OUTPUT: band[0],band[1]
850,1900
```

Here's a full example using all of the functions in the <a href="https://gist.github.com/technobly/b0e2f160b9d969d978337c0561999998" target="_blank">Cellular Band Select API</a>

There is one supported function for getting available bands using the CellularBand object:

`bool Cellular.getBandAvailable(CellularBand &band_avail);`

**Note:** getBandAvailable() always sets the first band array element `.band[0]` as 0 which indicates the first option available is to set the bands back to factory defaults, which includes all bands.

```
// SYNTAX
CellularBand band_avail;
Cellular.getBandAvailable(band_avail);
```

```
// EXAMPLE
CellularBand band_avail;
if (Cellular.getBandSelect(band_avail)) {
    Serial.print("Available bands: ");
    for (int x=0; x<band_avail.count; x++) {
        Serial.printf("%d", band_avail.band[x]);
        if (x+1 < band_avail.count) Serial.printf(",");
    }
    Serial.println();
}
else {
    Serial.printlnf("Bands available not retrieved from the modem!");
}
```

### getBandSelect()
*Since 0.5.0.*

Gets the cellular bands currently set in the modem.  `Bands` are the carrier frequncies used to communicate with the cellular network.

There is one supported function for getting selected bands using the CellularBand object:

`bool Cellular.getBandSelect(CellularBand &band_sel);`

```
// SYNTAX
CellularBand band_sel;
Cellular.getBandSelect(band_sel);
```

```
// EXAMPLE
CellularBand band_sel;
if (Cellular.getBandSelect(band_sel)) {
    Serial.print("Selected bands: ");
    for (int x=0; x<band_sel.count; x++) {
        Serial.printf("%d", band_sel.band[x]);
        if (x+1 < band_sel.count) Serial.printf(",");
    }
    Serial.println();
}
else {
    Serial.printlnf("Bands selected not retrieved from the modem!");
}
```

### setBandSelect()
*Since 0.5.0.*

Sets the cellular bands currently set in the modem.  `Bands` are the carrier frequncies used to communicate with the cellular network.

**Caution:** The Band Select API is an advanced feature designed to give users selective frequency control over their Electrons. When changing location or between cell towers, you may experience connectivity issues if you have only set one specific frequency for use. Because these settings are permanently saved in non-volatile memory, it is recommended to keep the factory default value of including all frequencies with mobile applications.  Only use the selective frequency control for stationary applications, or for special use cases.

- Make sure to set the `count` to the appropriate number of bands set in the CellularBand object before calling `setBandSelect()`.
- Use the `.isBand(int)` helper function to determine if an integer value is a valid band.  It still may not be valid for the particular modem you are using, in which case `setBandSelect()` will return `false` and `.ok` will also be set to `false`.
- When trying to set bands that are already set, they will not be written to Non-Volatile Memory (NVM) again.
- When updating the bands in the modem, they will be saved to NVM and will be remain as set when the modem power cycles.
- Setting `.band[0]` to `BAND_0` or `BAND_DEFAULT` and `.count` to 1 will restore factory defaults.

There are two supported functions for setting bands, one uses the CellularBand object, and the second allow you to pass in a comma delimited string of bands:

`bool Cellular.setBandSelect(const char* band_set);`

`bool Cellular.setBandSelect(CellularBand &band_set);`

```
// SYNTAX
CellularBand band_set;
Cellular.setBandSelect(band_set);

// or

Cellular.setBandSelect("850,1900"); // set two bands
Cellular.setBandSelect("0"); // factory defaults
```

```
// EXAMPLE
Serial.println("Setting bands to 850 only");
CellularBand band_set;
band_set.band[0] = BAND_850;
band_set.band[1] = BAND_1900;
band_set.count = 2;
if (Cellular.setBandSelect(band_set)) {
    Serial.print(band_set);
    Serial.println(" band(s) set! Value will be saved in NVM when powering off modem.");
}
else {
    Serial.print(band_set);
    Serial.println(" band(s) not valid! Use getBandAvailable() to query for valid bands.");
}
```

```
// EXAMPLE
Serial.println("Restoring factory defaults with the CellularBand object");
CellularBand band_set;
band_set.band[0] = BAND_DEFAULT;
band_set.count = 1;
if (Cellular.setBandSelect(band_set)) {
    Serial.println("Factory defaults set! Value will be saved in NVM when powering off modem.");
}
else {
    Serial.println("Restoring factory defaults failed!");
}
```

```
// EXAMPLE
Serial.println("Restoring factory defaults with strings");
if (Cellular.setBandSelect("0")) {
    Serial.println("Factory defaults set! Value will be saved in NVM when powering off modem.");
}
else {
    Serial.println("Restoring factory defaults failed!");
}
```

### localIP()
*Since 0.5.0.*

`Cellular.localIP()` returns the local (private) IP address assigned to the device as an `IPAddress`.

```C++
// EXAMPLE
void setup() {
  Serial.begin(9600);

  // Prints out the local (private) IP over Serial
  Serial.println(Cellular.localIP());
}
```

### command()

`Cellular.command()` is a powerful API that allows users access to directly send AT commands to, and parse responses returned from, the Cellular module.  Commands may be sent with and without printf style formatting. The API also includes the ability pass a callback function pointer which is used to parse the response returned from the cellular module.

You can download the latest <a href="https://www.u-blox.com/en/product-resources/2432?f[0]=field_file_category%3A210" target="_blank">u-blox AT Commands Manual</a>.

Another good resource is the <a href="https://www.u-blox.com/sites/default/files/AT-CommandsExamples_AppNote_%28UBX-13001820%29.pdf" target="_blank">u-blox AT Command Examples Application Note</a>.

The prototype definition is as follows:

`int Cellular.command(_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout, const char* format, ...);`

`Cellular.command()` takes one or more arguments in 4 basic types of signatures.

```C++
// SYNTAX (4 basic signatures with/without printf style formatting)
int ret = Cellular.command(cb, param, timeout, format, ...);
int ret = Cellular.command(cb, param, timeout, format);
int ret = Cellular.command(cb, param, format, ...);
int ret = Cellular.command(cb, param, format);
int ret = Cellular.command(timeout, format, ...);
int ret = Cellular.command(timeout, format);
int ret = Cellular.command(format, ...)
int ret = Cellular.command(format);
```

- `cb`: callback function pointer to a user specified function that parses the results of the AT command response.
- `param`: (`void*`) a pointer to the variable that will be updated by the callback function.
- `timeout`: (`system_tick_t`) the timeout value specified in milliseconds (default is 10000 milliseconds).
- `format`: (`const char*`) contains your AT command and any desired [format specifiers](http://www.cplusplus.com/reference/cstdio/printf/), followed by `\r\n`.
- `...`: additional arguments optionally required as input to their respective `format` string format specifiers.

`Cellular.command()` returns an `int` with one of the following 6 enumerated AT command responses:

- `NOT_FOUND`    =  0
- `WAIT`         = -1 // TIMEOUT
- `RESP_OK`      = -2
- `RESP_ERROR`   = -3
- `RESP_PROMPT`  = -4
- `RESP_ABORTED` = -5

```C++
// EXAMPLE - Get the ICCID number of the inserted SIM card
int callbackICCID(int type, const char* buf, int len, char* iccid)
{
  if ((type == TYPE_PLUS) && iccid) {
    if (sscanf(buf, "\r\n+CCID: %[^\r]\r\n", iccid) == 1)
      /*nothing*/;
  }
  return WAIT;
}

void setup()
{
  Serial.begin(9600);
  char iccid[32] = "";
  if ((RESP_OK == Cellular.command(callbackICCID, iccid, 10000, "AT+CCID\r\n"))
    && (strcmp(iccid,"") != 0))
  {
    Serial.printlnf("SIM ICCID = %s\r\n", iccid);
  }
  else
  {
    Serial.println("SIM ICCID NOT FOUND!");
  }
}

void loop()
{
  // your loop code
}
```
The `cb` function prototype is defined as:

`int callback(int type, const char* buf, int len, void* param);`

The four Cellular.command() callback arguments are defined as follows:

- `type`: (`int`) one of 13 different enumerated AT command response types.
- `buf`: (`const char*`) a pointer to the character array containing the AT command response.
- `len`: (`int`) length of the AT command response `buf`.
- `param`: (`void*`) a pointer to the variable or structure being updated by the callback function.
- returns an `int` - user specified callback return value, default is `return WAIT;` which keeps the system invoking the callback again as more of the AT command response is received.  Optionally any one of the 6 enumerated AT command responses previously described can be used as a return type which will force the Cellular.command() to return the same value. All AT commands typically end with an `OK` response, so even after a response is parsed, it is recommended to wait for the final `OK` response to be parsed and returned by the system. Then after testing `if(Cellular.command() == RESP_OK)` and any other optional qualifiers, should you act upon the results contained in the variable/structure passed into the callback.

There are 13 different enumerated AT command responses passed by the system into the Cellular.command() callback as `type`. These are used to help qualify which type of response has already been parsed by the system.

- `TYPE_UNKNOWN`    = 0x000000
- `TYPE_OK`         = 0x110000
- `TYPE_ERROR`      = 0x120000
- `TYPE_RING`       = 0x210000
- `TYPE_CONNECT`    = 0x220000
- `TYPE_NOCARRIER`  = 0x230000
- `TYPE_NODIALTONE` = 0x240000
- `TYPE_BUSY`       = 0x250000
- `TYPE_NOANSWER`   = 0x260000
- `TYPE_PROMPT`     = 0x300000
- `TYPE_PLUS`       = 0x400000
- `TYPE_TEXT`       = 0x500000
- `TYPE_ABORTED`    = 0x600000

{{/if}}


{{#if electron}}
## FuelGauge
The on-board Fuel Gauge allows you to monitor the battery voltage, state of charge and set low voltage battery thresholds. Use an instance of the `FuelGauge` library to call the various fuel gauge functions.

```C++
// EXAMPLE
FuelGauge fuel;
```

### getVCell()
Returns the battery voltage as a `float`.

```C++
// EXAMPLE
FuelGauge fuel;
Serial.println( fuel.getVCell() );
```

### getSoC()
Returns the State of Charge in percentage from 0-100% as a `float`.

```C++
// EXAMPLE
FuelGauge fuel;
Serial.println( fuel.getSoC() );
```

### getVersion()
`int getVersion();`

### getCompensateValue()
`byte getCompensateValue();`

### getAlertThreshold()
`byte getAlertThreshold();`

### setAlertThreshold()
`void setAlertThreshold(byte threshold);`

### getAlert()
`boolean getAlert();`

### clearAlert()
`void clearAlert();`

### reset()
`void reset();`

### quickStart()
`void quickStart();`

### sleep()
`void sleep();`

### wakeup()
`void wakeup();`
{{/if}}

## Input/Output

### pinMode()

`pinMode()` configures the specified pin to behave either as an input (with or without an internal weak pull-up or pull-down resistor), or an output.

```C++
// SYNTAX
pinMode(pin,mode);
```

`pinMode()` takes two arguments, `pin`: the number of the pin whose mode you wish to set and `mode`: `INPUT, INPUT_PULLUP, INPUT_PULLDOWN or OUTPUT.`

`pinMode()` does not return anything.

```C++
// EXAMPLE USAGE
int button = D0;                      // button is connected to D0
int LED = D1;                         // LED is connected to D1

void setup()
{
  pinMode(LED, OUTPUT);               // sets pin as output
  pinMode(button, INPUT_PULLDOWN);    // sets pin as input
}

void loop()
{
  // blink the LED as long as the button is pressed
  while(digitalRead(button) == HIGH) {
    digitalWrite(LED, HIGH);          // sets the LED on
    delay(200);                       // waits for 200mS
    digitalWrite(LED, LOW);           // sets the LED off
    delay(200);                       // waits for 200mS
  }
}
```

### getPinMode(pin)

Retrieves the current pin mode.

```cpp
// EXAMPLE

if (getPinMode(D0)==INPUT) {
  // D0 is an input pin
}
```

### digitalWrite()

Write a `HIGH` or a `LOW` value to a digital pin.

```C++
// SYNTAX
digitalWrite(pin, value);
```

If the pin has been configured as an `OUTPUT` with `pinMode()` or if previously used with `analogWrite()`, its voltage will be set to the corresponding value: 3.3V for HIGH, 0V (ground) for LOW.

`digitalWrite()` takes two arguments, `pin`: the number of the pin whose value you wish to set and `value`: `HIGH` or `LOW`.

`digitalWrite()` does not return anything.

```C++
// EXAMPLE USAGE
int LED = D1;              // LED connected to D1

void setup()
{
  pinMode(LED, OUTPUT);    // sets pin as output
}

void loop()
{
  digitalWrite(LED, HIGH); // sets the LED on
  delay(200);              // waits for 200mS
  digitalWrite(LED, LOW);  // sets the LED off
  delay(200);              // waits for 200mS
}
```

**Note:** All GPIO pins (`D0`..`D7`, `A0`..`A7`, `DAC`, `WKP`, `RX`, `TX`) can be used as long they are not used otherwise (e.g. as `Serial1` `RX`/`TX`).

### digitalRead()

Reads the value from a specified digital `pin`, either `HIGH` or `LOW`.

```C++
// SYNTAX
digitalRead(pin);
```

`digitalRead()` takes one argument, `pin`: the number of the digital pin you want to read.

`digitalRead()` returns `HIGH` or `LOW`.

```C++
// EXAMPLE USAGE
int button = D0;                   // button is connected to D0
int LED = D1;                      // LED is connected to D1
int val = 0;                       // variable to store the read value

void setup()
{
  pinMode(LED, OUTPUT);            // sets pin as output
  pinMode(button, INPUT_PULLDOWN); // sets pin as input
}

void loop()
{
  val = digitalRead(button);       // read the input pin
  digitalWrite(LED, val);          // sets the LED to the button's value
}

```
**Note:** All GPIO pins (`D0`..`D7`, `A0`..`A7`, `DAC`, `WKP`, `RX`, `TX`) can be used as long they are not used otherwise (e.g. as `Serial1` `RX`/`TX`).

### analogWrite() (PWM)

Writes an analog value to a pin as a digital PWM (pulse-width modulated) signal. The default frequency of the PWM signal is 500 Hz.

Can be used to light a LED at varying brightnesses or drive a motor at various speeds. After a call to analogWrite(), the pin will generate a steady square wave of the specified duty cycle until the next call to `analogWrite()` (or a call to `digitalRead()` or `digitalWrite()` on the same pin).

```C++
// SYNTAX
analogWrite(pin, value);
analogWrite(pin, value, frequency);
```

`analogWrite()` takes two or three arguments:

- `pin`: the number of the pin whose value you wish to set
- `value`: the duty cycle: between 0 (always off) and 255 (always on).
- `frequency`: the PWM frequency: between 1 Hz and 65535 Hz (default 500 Hz).

**NOTE:** `pinMode(pin, OUTPUT);` is required before calling `analogWrite(pin, value);` or else the `pin` will not be initialized as a PWM output and set to the desired duty cycle.

`analogWrite()` does not return anything.

```C++
// EXAMPLE USAGE

int ledPin = D1;               // LED connected to digital pin D1
int analogPin = A0;            // potentiometer connected to analog pin A0
int val = 0;                   // variable to store the read value

void setup()
{
  pinMode(ledPin, OUTPUT);     // sets the pin as output
}

void loop()
{
  val = analogRead(analogPin); // read the input pin
  analogWrite(ledPin, val/16); // analogRead values go from 0 to 4095,
                               // analogWrite values from 0 to 255.
  delay(10);
}
```

- On the Core, this function works on pins D0, D1, A0, A1, A4, A5, A6, A7, RX and TX.
- On the Photon and Electron, this function works on pins D0, D1, D2, D3, A4, A5, WKP, RX and TX with a caveat: PWM timer peripheral is duplicated on two pins (A5/D2) and (A4/D3) for 7 total independent PWM outputs. For example: PWM may be used on A5 while D2 is used as a GPIO, or D2 as a PWM while A5 is used as an analog input. However A5 and D2 cannot be used as independently controlled PWM outputs at the same time.
- Additionally on the Electron, this function works on pins B0, B1, B2, B3, C4, C5.

The PWM frequency must be the same for pins in the same timer group.

- On the Core, the timer groups are D0/D1, A0/A1/RX/TX, A4/A5/A6/A7.
- On the Photon, the timer groups are D0/D1, D2/D3/A4/A5, WKP, RX/TX.
- On the P1, the timer groups are D0/D1, D2/D3/A4/A5/P1S0/P1S1, WKP, RX/TX.
- On the Electron, the timer groups are D0/D1/C4/C5, D2/D3/A4/A5/B2/B3, WKP, RX/TX, B0/B1.

**NOTE:** When used with PWM capable pins, the `analogWrite()` function sets up these pins as PWM only.  {{#unless core}}This function operates differently when used with the [`Analog Output (DAC)`](#analog-output-dac-) pins.{{/unless}}

{{#unless core}}
### Analog Output (DAC)

The Photon and Electron support true analog output on pins DAC (`DAC1` or `A6` in code) and A3 (`DAC2` or `A3` in code). Using `analogWrite(pin, value)`
with these pins, the output of the pin is set to an analog voltage from 0V to 3.3V that corresponds to values
from 0-4095.

**NOTE:** This output is buffered inside the STM32 to allow for more output current at the cost of not being able to acheive rail-to-rail performance, i.e., the output will be about 50mV when the DAC is set to 0, and approx 50mV less than the 3V3 voltage when DAC output is set to 4095.

**NOTE:** System firmware version 0.4.6 and 0.4.7 only - not applicable to versions from 0.4.9 onwards: While for PWM pins one single call to `pinMode(pin, OUTPUT);` sets the pin mode for multiple `analogWrite(pin, value);` calls, for DAC pins you need to set `pinMode(DAC, OUTPUT);` each time you want to perform an `analogWrite()`.

```C++
// SYNTAX
pinMode(DAC1, OUTPUT);
analogWrite(DAC1, 1024);
// sets DAC pin to an output voltage of 1024/4095 * 3.3V = 0.825V.
```
{{/unless}}

### analogRead() (ADC)

Reads the value from the specified analog pin. The device has 8 channels (A0 to A7) with a 12-bit resolution. This means that it will map input voltages between 0 and 3.3 volts into integer values between 0 and 4095. This yields a resolution between readings of: 3.3 volts / 4096 units or, 0.0008 volts (0.8 mV) per unit.

**Note**: do *not* set the pinMode() with `analogRead()`. The pinMode() is automatically set to AN_INPUT the first time analogRead() is called for a particular analog pin. If you explicitly set a pin to INPUT or OUTPUT after that first use of analogRead(), it will not attempt to switch it back to AN_INPUT the next time you call analogRead() for the same analog pin. This will create incorrect analog readings.

```C++
// SYNTAX
analogRead(pin);
```

`analogRead()` takes one argument `pin`: the number of the analog input pin to read from ('A0 to A7'.)

`analogRead()` returns an integer value ranging from 0 to 4095.

```C++
// EXAMPLE USAGE
int ledPin = D1;                // LED connected to digital pin D1
int analogPin = A0;             // potentiometer connected to analog pin A0
int val = 0;                    // variable to store the read value

void setup()
{
  // Note: analogPin pin does not require pinMode()

  pinMode(ledPin, OUTPUT);      // sets the ledPin as output
}

void loop()
{
  val = analogRead(analogPin);  // read the analogPin
  analogWrite(ledPin, val/16);  // analogRead values go from 0 to 4095, analogWrite values from 0 to 255
  delay(10);
}
```

### setADCSampleTime()

The function `setADCSampleTime(duration)` is used to change the default sample time for `analogRead()`.

On the Core, this parameter can be one of the following values:

 * ADC_SampleTime_1Cycles5: Sample time equal to 1.5 cycles
 * ADC_SampleTime_7Cycles5: Sample time equal to 7.5 cycles
 * ADC_SampleTime_13Cycles5: Sample time equal to 13.5 cycles
 * ADC_SampleTime_28Cycles5: Sample time equal to 28.5 cycles
 * ADC_SampleTime_41Cycles5: Sample time equal to 41.5 cycles
 * ADC_SampleTime_55Cycles5: Sample time equal to 55.5 cycles
 * ADC_SampleTime_71Cycles5: Sample time equal to 71.5 cycles
 * ADC_SampleTime_239Cycles5: Sample time equal to 239.5 cycles

 On the Photon and Electron, this parameter can be one of the following values:

 * ADC_SampleTime_3Cycles: Sample time equal to 3 cycles
 * ADC_SampleTime_15Cycles: Sample time equal to 15 cycles
 * ADC_SampleTime_28Cycles: Sample time equal to 28 cycles
 * ADC_SampleTime_56Cycles: Sample time equal to 56 cycles
 * ADC_SampleTime_84Cycles: Sample time equal to 84 cycles
 * ADC_SampleTime_112Cycles: Sample time equal to 112 cycles
 * ADC_SampleTime_144Cycles: Sample time equal to 144 cycles
 * ADC_SampleTime_480Cycles: Sample time equal to 480 cycles

## Low Level Input/Output

The Input/Ouput functions include safety checks such as making sure a pin is set to OUTPUT when doing a digitalWrite() or that the pin is not being used for a timer function.  These safety measures represent good coding and system design practice.

There are times when the fastest possible input/output operations are crucial to an applications performance. The SPI, UART (Serial) or I2C hardware are examples of low level performance-oriented devices.  There are, however, times when these devices may not be suitable or available.  For example, One-wire support is done in software, not hardware.

In order to provide the fastest possible bit-oriented I/O, the normal safety checks must be skipped.  As such, please be aware that the programmer is responsible for proper planning and use of the low level I/O functions.

Prior to using the following low-level functions, `pinMode()` must be used to configure the target pin.


### pinSetFast()

Write a `HIGH` value to a digital pin.

```C++
// SYNTAX
pinSetFast(pin);
```

`pinSetFast()` takes one argument, `pin`: the number of the pin whose value you wish to set `HIGH`.

`pinSetFast()` does not return anything.

```C++
// EXAMPLE USAGE
int LED = D7;              // LED connected to D7

void setup()
{
  pinMode(LED, OUTPUT);    // sets pin as output
}

void loop()
{
  pinSetFast(LED);		   // set the LED on
  delay(500);
  pinResetFast(LED);	   // set the LED off
  delay(500);
}
```

### pinResetFast()

Write a `LOW` value to a digital pin.

```C++
// SYNTAX
pinResetFast(pin);
```

`pinResetFast()` takes one argument, `pin`: the number of the pin whose value you wish to set `LOW`.

`pinResetFast()` does not return anything.

```C++
// EXAMPLE USAGE
int LED = D7;              // LED connected to D7

void setup()
{
  pinMode(LED, OUTPUT);    // sets pin as output
}

void loop()
{
  pinSetFast(LED);		   // set the LED on
  delay(500);
  pinResetFast(LED);	   // set the LED off
  delay(500);
}
```

### digitalWriteFast()

Write a `HIGH` or `LOW` value to a digital pin.  This function will call pinSetFast() or pinResetFast() based on `value` and is useful when `value` is calculated. As such, this imposes a slight time overhead.

```C++
// SYNTAX
digitalWriteFast(pin, value);
```

`digitalWriteFast()` `pin`: the number of the pin whose value you wish to set and `value`: `HIGH` or `LOW`.

`digitalWriteFast()` does not return anything.

```C++
// EXAMPLE USAGE
int LED = D7;              // LED connected to D7

void setup()
{
  pinMode(LED, OUTPUT);    // sets pin as output
}

void loop()
{
  digitalWriteFast(LED, HIGH);		   // set the LED on
  delay(500);
  digitalWriteFast(LED, LOW);	      // set the LED off
  delay(500);
}
```

### pinReadFast()

Reads the value from a specified digital `pin`, either `HIGH` or `LOW`.

```C++
// SYNTAX
pinReadFast(pin);
```

`pinReadFast()` takes one argument, `pin`: the number of the digital pin you want to read.

`pinReadFast()` returns `HIGH` or `LOW`.

```C++
// EXAMPLE USAGE
int button = D0;                   // button is connected to D0
int LED = D1;                      // LED is connected to D1
int val = 0;                       // variable to store the read value

void setup()
{
  pinMode(LED, OUTPUT);            // sets pin as output
  pinMode(button, INPUT_PULLDOWN); // sets pin as input
}

void loop()
{
  val = pinReadFast(button);       // read the input pin
  digitalWriteFast(LED, val);      // sets the LED to the button's value
}

```

## Advanced I/O

### tone()

Generates a square wave of the specified frequency and duration (and 50% duty cycle) on a timer channel pin which supports PWM. Use of the tone() function will interfere with PWM output on the selected pin.

- On the Core, this function works on pins D0, D1, A0, A1, A4, A5, A6, A7, RX and TX.

- On the Photon and Electron, this function works on pins D0, D1, D2, D3, A4, A5, WKP, RX and TX with a caveat: Tone timer peripheral is duplicated on two pins (A5/D2) and (A4/D3) for 7 total independent Tone outputs. For example: Tone may be used on A5 while D2 is used as a GPIO, or D2 for Tone while A5 is used as an analog input. However A5 and D2 cannot be used as independent Tone outputs at the same time.

- Additionally on the Electron, this function works on pins B0, B1, B2, B3, C4, C5.

```C++
// SYNTAX
tone(pin, frequency, duration)
```

`tone()` takes three arguments, `pin`: the pin on which to generate the tone, `frequency`: the frequency of the tone in hertz and `duration`: the duration of the tone in milliseconds (a zero value = continuous tone).

The frequency range is from 20Hz to 20kHz. Frequencies outside this range will not be played.

`tone()` does not return anything.

**NOTE:** the Photon's PWM pins / timer channels are allocated as per the following table. If multiple, simultaneous tone() calls are needed (for example, to generate DTMF tones), use pins allocated to separate timers to avoid stuttering on the output:

Pin  | TMR3 | TMR4 | TMR5
:--- | :--: | :--: | :--:
D0   |      |  x   |  
D1   |      |  x   |  
D2   |  x   |      |  
D3   |  x   |      |  
A4   |  x   |      |  
A5   |  x   |      |  
WKP  |      |      |  x


```C++
// EXAMPLE USAGE
// Plays a melody - Connect small speaker to analog pin A0

int speakerPin = A0;

// notes in the melody:
int melody[] = {1908,2551,2551,2273,2551,0,2024,1908}; //C4,G3,G3,A3,G3,0,B3,C4

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {4,8,8,4,4,4,4,4 };

void setup() {
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000/noteDurations[thisNote];
    tone(speakerPin, melody[thisNote],noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(speakerPin);
  }
}
```

### noTone()

Stops the generation of a square wave triggered by tone() on a specified pin (D0, D1, A0, A1, A4, A5, A6, A7, RX, TX). Has no effect if no tone is being generated.


```C++
// SYNTAX
noTone(pin)
```

`noTone()` takes one argument, `pin`: the pin on which to stop generating the tone.

`noTone()` does not return anything.

```C++
//See the tone() example
```

### shiftOut()

Shifts out a byte of data one bit at a time on a specified pin. Starts from either the most (i.e. the leftmost) or least (rightmost) significant bit. Each bit is written in turn to a data pin, after which a clock pin is pulsed (taken high, then low) to indicate that the bit is available.

**NOTE:** if you're interfacing with a device that's clocked by rising edges, you'll need to make sure that the clock pin is low before the call to `shiftOut()`, e.g. with a call to `digitalWrite(clockPin, LOW)`.

This is a software implementation; see also the SPI function, which provides a hardware implementation that is faster but works only on specific pins.


```C++
// SYNTAX
shiftOut(dataPin, clockPin, bitOrder, value)
```
```C++
// EXAMPLE USAGE

// Use digital pins D0 for data and D1 for clock
int dataPin = D0;
int clock = D1;

uint8_t data = 50;

setup() {
	// Set data and clock pins as OUTPUT pins before using shiftOut()
	pinMode(dataPin, OUTPUT);
	pinMode(clock, OUTPUT);

	// shift out data using MSB first
	shiftOut(dataPin, clock, MSBFIRST, data);

	// Or do this for LSBFIRST serial
	shiftOut(dataPin, clock, LSBFIRST, data);
}

loop() {
	// nothing to do
}
```

`shiftOut()` takes four arguments, 'dataPin': the pin on which to output each bit, `clockPin`: the pin to toggle once the dataPin has been set to the correct value, `bitOrder`: which order to shift out the bits; either MSBFIRST or LSBFIRST (Most Significant Bit First, or, Least Significant Bit First) and `value`: the data (byte) to shift out.

`shiftOut()` does not return anything.



### shiftIn()

Shifts in a byte of data one bit at a time. Starts from either the most (i.e. the leftmost) or least (rightmost) significant bit. For each bit, the clock pin is pulled high, the next bit is read from the data line, and then the clock pin is taken low.

**NOTE:** if you're interfacing with a device that's clocked by rising edges, you'll need to make sure that the clock pin is low before the call to shiftOut(), e.g. with a call to `digitalWrite(clockPin, LOW)`.

This is a software implementation; see also the SPI function, which provides a hardware implementation that is faster but works only on specific pins.


```C++
// SYNTAX
shiftIn(dataPin, clockPin, bitOrder)
```
```C++
// EXAMPLE USAGE

// Use digital pins D0 for data and D1 for clock
int dataPin = D0;
int clock = D1;

uint8_t data;

setup() {
	// Set data as INPUT and clock pin as OUTPUT before using shiftIn()
	pinMode(dataPin, INPUT);
	pinMode(clock, OUTPUT);

	// shift in data using MSB first
	data = shiftIn(dataPin, clock, MSBFIRST);

	// Or do this for LSBFIRST serial
	data = shiftIn(dataPin, clock, LSBFIRST);
}

loop() {
	// nothing to do
}
```

`shiftIn()` takes three arguments, 'dataPin': the pin on which to input each bit, `clockPin`: the pin to toggle to signal a read from dataPin, `bitOrder`: which order to shift in the bits; either MSBFIRST or LSBFIRST (Most Significant Bit First, or, Least Significant Bit First).

`shiftIn()` returns the byte value read.


### pulseIn()

*Since 0.4.7.*

Reads a pulse (either HIGH or LOW) on a pin. For example, if value is HIGH, pulseIn() waits for the pin to go HIGH, starts timing, then waits for the pin to go LOW and stops timing. Returns the length of the pulse in microseconds or 0 if no complete pulse was received within the timeout.

The timing of this function is based on an internal hardware counter derived from the system tick clock.  Resolution is 1/Fosc (1/72MHz for Core, 1/120MHz for Photon/P1/Electron). Works on pulses from 10 microseconds to 3 seconds in length. Please note that if the pin is already reading the desired `value` when the function is called, it will wait for the pin to be the opposite state of the desired `value`, and then finally measure the duration of the desired `value`. This routine is blocking and does not use interrupts.  The `pulseIn()` routine will time out and return 0 after 3 seconds.

```C++
// SYNTAX
pulseIn(pin, value)
```

`pulseIn()` takes two arguments, `pin`: the pin on which you want to read the pulse (this can be any GPIO, e.g. D1, A2, C0, B3, etc..), `value`: type of pulse to read: either HIGH or LOW. `pin` should be set to one of three [pinMode()](#pinmode-)'s prior to using pulseIn(), `INPUT`, `INPUT_PULLUP` or `INPUT_PULLDOWN`.

`pulseIn()` returns the length of the pulse (in microseconds) or 0 if no pulse is completed before the 3 second timeout (unsigned long)

```C++
// EXAMPLE
unsigned long duration;

void setup()
{
    Serial.begin(9600);
    pinMode(D0, INPUT);

    // Pulse generator, connect D1 to D0 with a jumper
    // PWM output is 500Hz at 50% duty cycle
    // 1000us HIGH, 1000us LOW
    pinMode(D1, OUTPUT);
    analogWrite(D1, 128);
}

void loop()
{
    duration = pulseIn(D0, HIGH);
    Serial.printlnf("%d us", duration);
    delay(1000);
}

/* OUTPUT
 * 1003 us
 * 1003 us
 * 1003 us
 * 1003 us
 */
```

{{#if electron}}
## PMIC (Power Managment IC)

*Note*: This is advanced IO and for experienced users. This
controls the LiPo battery management system and is handled automatically
by the system firmware.

### begin()
`bool begin();`

### getVersion()
`byte getVersion();`

### getSystemStatus()
`byte getSystemStatus();`

### getFault()
`byte getFault();`

---

### Input source control register

#### readInputSourceRegister()
`byte readInputSourceRegister(void);`

#### enableBuck()
`bool enableBuck(void);`

#### disableBuck()
`bool disableBuck(void);`

#### setInputCurrentLimit()
`bool setInputCurrentLimit(uint16_t current);`

#### getInputCurrentLimit()
`byte getInputCurrentLimit(void);`

#### setInputVoltageLimit()
`bool setInputVoltageLimit(uint16_t voltage);`

#### getInputVoltageLimit()
`byte getInputVoltageLimit(void);`

---

### Power ON Configuration Reg

#### enableCharging()
`bool enableCharging(void);`

#### disableCharging()
`bool disableCharging(void);`

#### enableOTG()
`bool enableOTG(void);`

#### disableOTG()
`bool disableOTG(void);`

#### resetWatchdog()
`bool resetWatchdog(void);`

#### setMinimumSystemVoltage()
`bool setMinimumSystemVoltage(uint16_t voltage);`

#### getMinimumSystemVoltage()
`uint16_t getMinimumSystemVoltage();`

#### readPowerONRegister()
`byte readPowerONRegister(void);`

---

### Charge Current Control Reg

#### setChargeCurrent()
`bool setChargeCurrent(bool bit7, bool bit6, bool bit5, bool bit4, bool bit3, bool bit2);`

#### getChargeCurrent()
`byte getChargeCurrent(void);`

---

### PreCharge/Termination Current Control Reg

#### setPreChargeCurrent()
`bool setPreChargeCurrent();`

#### getPreChargeCurrent()
`byte getPreChargeCurrent();`

#### setTermChargeCurrent()
`bool setTermChargeCurrent();`

#### getTermChargeCurrent()
`byte getTermChargeCurrent();`

---

### Charge Voltage Control Reg

#### setChargeVoltage()
`bool setChargeVoltage(uint16_t voltage);`

#### getChargeVoltage()
`byte getChargeVoltage();`

---

### Charge Timer Control Reg

#### readChargeTermRegister()
`byte readChargeTermRegister();`

#### disableWatchdog()
`bool disableWatchdog(void);`

#### setWatchdog()
`bool setWatchdog(byte time);`

---

### Thermal Regulation Control Reg

#### setThermalRegulation()
`bool setThermalRegulation();`

#### getThermalRegulation()
`byte getThermalRegulation();`

---

### Misc Operation Control Reg

#### readOpControlRegister()
`byte readOpControlRegister();`

#### enableDPDM()
`bool enableDPDM(void);`

#### disableDPDM()
`bool disableDPDM(void);`

#### enableBATFET()
`bool enableBATFET(void);`

#### disableBATFET()
`bool disableBATFET(void);`

#### safetyTimer()
`bool safetyTimer();`

#### enableChargeFaultINT()
`bool enableChargeFaultINT();`

#### disableChargeFaultINT()
`bool disableChargeFaultINT();`

#### enableBatFaultINT()
`bool enableBatFaultINT();`

#### disableBatFaultINT()
`bool disableBatFaultINT();`

---

### System Status Register

#### getVbusStat()
`byte getVbusStat();`

#### getChargingStat()
`byte getChargingStat();`

#### getDPMStat()
`bool getDPMStat();`

#### isPowerGood()
`bool isPowerGood(void);`

#### isHot()
`bool isHot(void);`

#### getVsysStat()
`bool getVsysStat();`

---

### Fault Register

#### isWatchdogFault()
`bool isWatchdogFault();`

#### getChargeFault()
`byte getChargeFault();`

#### isBatFault()
`bool isBatFault();`

#### getNTCFault()
`byte getNTCFault();`

{{/if}}

## Serial

Used for communication between the device and a computer or other devices. The device has two serial channels:

`Serial:` This channel communicates through the USB port and when connected to a computer, will show up as a virtual COM port.

`Serial1:` This channel is available via the device's TX and RX pins.

{{#if core}}
`Serial2:` This channel is optionally available via the device's D1(TX) and D0(RX) pins. To use Serial2, add `#include "Serial2/Serial2.h"` near the top of your app's main code file.

To use the TX/RX (Serial1) or D1/D0 (Serial2) pins to communicate with your personal computer, you will need an additional USB-to-serial adapter. To use them to communicate with an external TTL serial device, connect the TX pin to your device's RX pin, the RX to your device's TX pin, and the ground of your Core to your device's ground.
{{/if}}

{{#unless core}}
`Serial2:` This channel is optionally available via the device's RGB Green (TX) and Blue (RX) LED pins. The Blue and Green current limiting resistors should be removed.  To use Serial2, add #include "Serial2/Serial2.h" near the top of your app's main code file.

If the user enables Serial2, they should also consider using RGB.onChange() to move the RGB functionality to an external RGB LED on some PWM pins.
{{/unless}}

{{#if electron}}
`Serial4:` This channel is optionally available via the Electron's C3(TX) and C2(RX) pins. To use Serial4, add `#include "Serial4/Serial4.h"` near the top of your app's main code file.

`Serial5:` This channel is optionally available via the Electron's C1(TX) and C0(RX) pins. To use Serial5, add `#include "Serial5/Serial5.h"` near the top of your app's main code file.
{{/if}}

```C++
// EXAMPLE USAGE
// Include the appropriate header file for Serial2{{#if electron}}, Serial4, or Serial5{{/if}}
#include "Serial2/Serial2.h"
{{#if electron}}
#include "Serial4/Serial4.h"
#include "Serial5/Serial5.h"
{{/if}}

void setup()
{
  Serial2.begin(9600);
{{#if electron}}
  Serial4.begin(9600);
  Serial5.begin(9600);
{{/if}}

  Serial2.println("Hello World!");
{{#if electron}}
  Serial4.println("Hello World!");
  Serial5.println("Hello World!");
{{/if}}
}
```

To use the hardware serial pins of (Serial1/2{{#if electron}}/4/5{{/if}}) to communicate with your personal computer, you will need an additional USB-to-serial adapter. To use them to communicate with an external TTL serial device, connect the TX pin to your device's RX pin, the RX to your device's TX pin, and the ground of your Core/Photon/Electron to your device's ground.

**NOTE:** Please take into account that the voltage levels on these pins operate at 0V to 3.3V and should not be connected directly to a computer's RS232 serial port which operates at +/- 12V and will damage the Core/Photon/Electron.

### begin()

Sets the data rate in bits per second (baud) for serial data transmission. For communicating with the computer, use one of these rates: 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, or 115200. You can, however, specify other rates - for example, to communicate over pins TX and RX with a component that requires a particular baud rate.

**NOTE:** The data rate for the USB device `Serial` is ignored, as USB has its own negotiated speed. Setting speed to 9600 is safe for the USB device. Setting the port on the Host computer to 14400 baud will cause the Photon or Electron to go into DFU mode while 28800 will allow a YMODEM download of firmware.

_Since 0.5.0_

As of 0.5.0 firmware, 28800 baud set on the Host will put the device in Listening Mode, where a YMODEM download can be started by additionally sending an `f` character.

The configuration of the serial channel may also specify the number of data bits, stop bits and parity. The default is SERIAL_8N1 (8 data bits, no parity and 1 stop bit) and does not need to be specified to achieve this configuration.  To specify one of the following configurations, add one of these defines as the second parameter in the `begin()` function, e.g. `Serial.begin(9600, SERIAL_8E1);` for 8 data bits, even parity and 1 stop bit.

Serial configurations available:

`SERIAL_8N1` - 8 data bits, no parity, 1 stop bit (default)
`SERIAL_8N2` - 8 data bits, no parity, 2 stop bits
`SERIAL_8E1` - 8 data bits, even parity, 1 stop bit
`SERIAL_8E2` - 8 data bits, even parity, 2 stop bits
`SERIAL_8O1` - 8 data bits, odd parity, 1 stop bit
`SERIAL_8O2` - 8 data bits, odd parity, 2 stop bits
`SERIAL_9N1` - 9 data bits, no parity, 1 stop bit
`SERIAL_9N2` - 9 data bits, no parity, 2 stop bits

```C++
// SYNTAX
Serial.begin(speed);          // via USB port
Serial.begin(speed, config);  //  "

Serial1.begin(speed);         // via TX/RX pins
Serial1.begin(speed, config); //  "

Serial2.begin(speed);         // on Core via
                              // D1(TX) and D0(RX) pins
                              // on Photon/Electron via
                              // RGB-LED green(TX) and
                              // RGB-LED blue (RX) pins
Serial2.begin(speed, config); //  "
{{#if electron}}

Serial4.begin(speed);         // via C3(TX)/C2(RX) pins
Serial4.begin(speed, config); //  "

Serial5.begin(speed);         // via C1(TX)/C0(RX) pins
Serial5.begin(speed, config); //  "
{{/if}}
```

`speed`: parameter that specifies the baud rate *(long)*
`config`: parameter that specifies the number of data bits used, parity and stop bits *(long)*

`begin()` does not return anything

```C++
// EXAMPLE USAGE
void setup()
{
  Serial.begin(9600);   // open serial over USB
  // On Windows it will be necessary to implement the following line:
  // Make sure your Serial Terminal app is closed before powering your device
  // Now open your Serial Terminal, and hit any key to continue!
  while(!Serial.available()) Particle.process();

  Serial1.begin(9600);  // open serial over TX and RX pins

  Serial.println("Hello Computer");
  Serial1.println("Hello Serial 1");
}

void loop() {}
```

### end()

Disables serial communication, allowing the RX and TX pins to be used for general input and output. To re-enable serial communication, call `Serial1.begin()`.

```C++
// SYNTAX
Serial1.end();
```

### available()

Get the number of bytes (characters) available for reading from the serial port. This is data that's already arrived and stored in the serial receive buffer (which holds 64 bytes).

```C++
// EXAMPLE USAGE
void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);

}

void loop()
{
  // read from port 0, send to port 1:
  if (Serial.available())
  {
    int inByte = Serial.read();
    Serial1.write(inByte);
  }
  // read from port 1, send to port 0:
  if (Serial1.available())
  {
    int inByte = Serial1.read();
    Serial.write(inByte);
  }
}
```

### availableForWrite()

_Since 0.4.9. Available on Serial1, Serial2, etc._

_Since 0.5.0. Available on USB Serial (Serial)_

Retrieves the number of bytes (characters) that can be written to this serial port without blocking.

If `blockOnOverrun(false)` has been called, the method returns the number of bytes that can be written to the buffer without causing buffer overrun, which would cause old data to be discarded and overwritten.

### blockOnOverrun()

_Since 0.4.9. Available on Serial1, Serial2, etc._

_Since 0.5.0. Available on USB Serial (Serial)_

Defines what should happen when calls to `write()/print()/println()/printlnf()` that would overrun the buffer.

- `blockOnOverrun(true)` -  this is the default setting.  When there is no room in the buffer for the data to be written, the program waits/blocks until there is room. This avoids buffer overrun, where data that has not yet been sent over serial is overwritten by new data. Use this option for increased data integrity at the cost of slowing down realtime code execution when lots of serial data is sent at once.

- `blockOnOverrun(false)` - when there is no room in the buffer for data to be written, the data is written anyway, causing the new data to replace the old data. This option is provided when performance is more important than data integrity.

```cpp
// EXAMPLE - fast and furious over Serial1
Serial1.blockOnOverrun(false);
Serial1.begin(115200);
```

### serialEvent()

A family of application-defined functions that are called whenever there is data to be read
from a serial peripheral.

- serialEvent: called when there is data available from `Serial`
- serialEvent1: called when there is data available from `Serial1`
- serialEvent2: called when there is data available from `Serial2`
{{#if electron}}
- serialEvent4: called when there is data available from `Serial4`
- serialEvent5: called when there is data available from `Serial5`
{{/if}}

The `serialEvent` functions are called by the system as part of the application loop. Since these is an
extension of the application loop, it is ok to call any functions at you would also call from loop().

```cpp
// EXAMPLE - echo all characters typed over serial

void setup()
{
   Serial.begin(9600);
}

void serialEvent()
{
    char c = Serial.read();
    Serial.print(c);
}

```

### peek()

Returns the next byte (character) of incoming serial data without removing it from the internal serial buffer. That is, successive calls to peek() will return the same character, as will the next call to `read()`.

```C++
// SYNTAX
Serial.peek();
Serial1.peek();
```
`peek()` returns the first byte of incoming serial data available (or `-1` if no data is available) - *int*

### write()

Writes binary data to the serial port. This data is sent as a byte or series of bytes; to send the characters representing the digits of a number use the `print()` function instead.

```C++
// SYNTAX
Serial.write(val);
Serial.write(str);
Serial.write(buf, len);
```

```C++
// EXAMPLE USAGE

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  Serial.write(45); // send a byte with the value 45

  int bytesSent = Serial.write(“hello”); //send the string “hello” and return the length of the string.
}
```

*Parameters:*

- `val`: a value to send as a single byte
- `str`: a string to send as a series of bytes
- `buf`: an array to send as a series of bytes
- `len`: the length of the buffer

`write()` will return the number of bytes written, though reading that number is optional.


### read()

Reads incoming serial data.

```C++
// SYNTAX
Serial.read();
Serial1.read();
```
`read()` returns the first byte of incoming serial data available (or -1 if no data is available) - *int*

```C++
// EXAMPLE USAGE
int incomingByte = 0; // for incoming serial data

void setup() {
  Serial.begin(9600); // opens serial port, sets data rate to 9600 bps
}

void loop() {
  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    // say what you got:
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
  }
}
```
### print()

Prints data to the serial port as human-readable ASCII text.
This command can take many forms. Numbers are printed using an ASCII character for each digit. Floats are similarly printed as ASCII digits, defaulting to two decimal places. Bytes are sent as a single character. Characters and strings are sent as is. For example:

- Serial.print(78) gives "78"
- Serial.print(1.23456) gives "1.23"
- Serial.print('N') gives "N"
- Serial.print("Hello world.") gives "Hello world."

An optional second parameter specifies the base (format) to use; permitted values are BIN (binary, or base 2), OCT (octal, or base 8), DEC (decimal, or base 10), HEX (hexadecimal, or base 16). For floating point numbers, this parameter specifies the number of decimal places to use. For example:

- Serial.print(78, BIN) gives "1001110"
- Serial.print(78, OCT) gives "116"
- Serial.print(78, DEC) gives "78"
- Serial.print(78, HEX) gives "4E"
- Serial.println(1.23456, 0) gives "1"
- Serial.println(1.23456, 2) gives "1.23"
- Serial.println(1.23456, 4) gives "1.2346"

### println()

Prints data to the serial port as human-readable ASCII text followed by a carriage return character (ASCII 13, or '\r') and a newline character (ASCII 10, or '\n'). This command takes the same forms as `Serial.print()`.

```C++
// SYNTAX
Serial.println(val);
Serial.println(val, format);
```

*Parameters:*

- `val`: the value to print - any data type
- `format`: specifies the number base (for integral data types) or number of decimal places (for floating point types)

`println()` returns the number of bytes written, though reading that number is optional - `size_t (long)`

```C++
// EXAMPLE
//reads an analog input on analog in A0, prints the value out.

int analogValue = 0;    // variable to hold the analog value

void setup()
{
  // Make sure your Serial Terminal app is closed before powering your device
  Serial.begin(9600);
  // Now open your Serial Terminal, and hit any key to continue!
  while(!Serial.available()) Particle.process();
}

void loop() {
  // read the analog input on pin A0:
  analogValue = analogRead(A0);

  // print it out in many formats:
  Serial.println(analogValue);       // print as an ASCII-encoded decimal
  Serial.println(analogValue, DEC);  // print as an ASCII-encoded decimal
  Serial.println(analogValue, HEX);  // print as an ASCII-encoded hexadecimal
  Serial.println(analogValue, OCT);  // print as an ASCII-encoded octal
  Serial.println(analogValue, BIN);  // print as an ASCII-encoded binary

  // delay 10 milliseconds before the next reading:
  delay(10);
}
```

### printf()

*Since 0.4.6.*

Provides [printf](http://www.cplusplus.com/reference/cstdio/printf/)-style formatting over serial.

`printf` allows strings to be built by combining a number of values with text.

```C++
Serial.printf("Reading temperature sensor at %s...", Time.timeStr().c_str());
float temp = readTemp();
Serial.printf("the temperature today is %f Kelvin", temp);
Serial.println();
```

Running this code prints:

```
Reading temperature sensor at Thu 01 Oct 2015 12:34...the temperature today is 293.1 Kelvin.
```

The last `printf()` call could be changed to `printlnf()` to avoid a separate call to `println()`.


### printlnf()

*Since 0.4.6.*

formatted output followed by a newline.
Produces the same output as [printf](#printf-) which is then followed by a newline character,
so to that subsequent output appears on the next line.


### flush()

Waits for the transmission of outgoing serial data to complete.

**NOTE:** That this function does nothing at present, in particular it doesn't
wait for the data to be sent, since this causes the application to wait indefinitely
when there is no serial monitor connected.

```C++
// SYNTAX
Serial.flush();
Serial1.flush();
```

`flush()` neither takes a parameter nor returns anything

### halfduplex()

Puts Serial1 into half-duplex mode.  In this mode both the transmit and receive
are on the TX pin.  This mode can be used for a single wire bus communications
scheme between microcontrollers.

```C++
// SYNTAX
Serial1.halfduplex(true);  // Enable half-duplex mode
Serial1.halfduplex(false); // Disable half-duplex mode
```

```C++
// EXAMPLE
// Initializes Serial1 at 9600 baud and enables half duplex mode

Serial1.begin(9600);
Serial1.halfduplex(true);

```
`halfduplex()` takes one argument: `true` enables half-duplex mode, `false` disables half-duplex mode

`halfduplex()` returns nothing



SPI
----
This library allows you to communicate with SPI devices, with the {{device}} as the master device.

{{#unless core}}
_Since 0.5.0_ the {{device}} can function as a slave.
{{/unless}}

{{#if core}}
![SPI](/assets/images/core-pin-spi.jpg)
{{/if}}

The hardware SPI pin functions, which can
be used via the `SPI` object, are mapped as follows:
* `SS` => `A2` (default)
* `SCK` => `A3`
* `MISO` => `A4`
* `MOSI` => `A5`

{{#unless core}}
There is a second hardware SPI interface available, which can
be used via the `SPI1` object. This second port is mapped as follows:
* `SS` => `D5` (default)
* `SCK` => `D4`
* `MISO` => `D3`
* `MOSI` => `D2`
{{/unless}}

{{#if electron}}
Additionally on the Electron, there is an alternate pin location for the second SPI interface, which can
be used via the `SPI2` object. This alternate location is mapped as follows:
* `SS` => `D5` (default)
* `SCK` => `C3`
* `MISO` => `C2`
* `MOSI` => `C1`
{{/if}}

{{#unless core}}
**Note**: Because there are multiple SPI peripherals available, be sure to use the same `SPI`,`SPI1`{{#if electron}},`SPI2`{{/if}} object with all associated functions. I.e.,

Do **NOT** use **SPI**.begin() with **SPI1**.transfer();

**Do** use **SPI**.begin() with **SPI**.transfer();
{{/unless}}

### begin()

Initializes the SPI bus by setting SCK, MOSI, and a user-specified slave-select pin to outputs, MISO to input. SCK is pulled either high or low depending on the configured SPI data mode (default high for `SPI_MODE3`). Slave-select is pulled high.

**Note:**  The SPI firmware ONLY initializes the user-specified slave-select pin as an `OUTPUT`. The user's code must control the slave-select pin with `digitalWrite()` before and after each SPI transfer for the desired SPI slave device. Calling `SPI.end()` does NOT reset the pin mode of the SPI pins.

```C++
// SYNTAX
SPI.begin(ss);
{{#unless core}}
SPI1.begin(ss);
{{/unless}}
{{#if electron}}
SPI2.begin(ss);
{{/if}}
```

Where, the parameter `ss` is the `SPI` device slave-select pin to initialize.  If no pin is specified, the default pin is `SS (A2)`.
{{#unless core}}
For `SPI1`, the default `ss` pin is `SS (D5)`.
{{/unless}}
{{#if electron}}
For `SPI2`, the default `ss` pin is also `SS (D5)`.
{{/if}}

{{#unless core}}
```C++
// Example using SPI1, with D5 as the SS pin:
SPI1.begin();
// or
SPI1.begin(D5);
```
{{/unless}}
{{#if electron}}
```C++
// Example using SPI2, with C0 as the SS pin:
SPI2.begin(C0);
```
{{/if}}

{{#unless core}}

### begin(SPI_Mode, uint16_t)

_Since 0.5.0_

Initializes the {{device}} SPI peripheral in master or slave mode.

**Note:** MISO, MOSI and SCK idle in high-impedance state when SPI peripheral is configured in slave mode and the device is not selected.

Parameters:

- `mode`: `SPI_MODE_MASTER` or `SPI_MODE_SLAVE`
- `ss_pin`: slave-select pin to initialize. If no pin is specified, the default pin is `SS (A2)`. For `SPI1`, the default pin is `SS (D5)`. {{#if electron}}For `SPI2`, the default pin is also `SS (D5)`.{{/if}}

```C++
// Example using SPI in master mode, with A2 (default) as the SS pin:
SPI.begin(SPI_MODE_MASTER);
// Example using SPI1 in slave mode, with D5 as the SS pin
SPI1.begin(SPI_MODE_SLAVE, D5);
// Example using SPI2 in slave mode, with C0 as the SS pin
SPI2.begin(SPI_MODE_SLAVE, C0);
```

{{/unless}}

### end()

Disables the SPI bus (leaving pin modes unchanged).

```C++
// SYNTAX
SPI.end();
{{#unless core}}
SPI1.end();
{{/unless}}
{{#if electron}}
SPI2.end();
{{/if}}
```

### setBitOrder()

Sets the order of the bits shifted out of and into the SPI bus, either LSBFIRST (least-significant bit first) or MSBFIRST (most-significant bit first).

```C++
// SYNTAX
SPI.setBitOrder(order);
{{#unless core}}
SPI1.setBitOrder(order);
{{/unless}}
{{#if electron}}
SPI2.setBitOrder(order);
{{/if}}
```

Where, the parameter `order` can either be `LSBFIRST` or `MSBFIRST`.

### setClockSpeed

Sets the SPI clock speed. The value can be specified as a direct value, or as
as a value plus a multiplier.


```C++
// SYNTAX
SPI.setClockSpeed(value, scale);
SPI.setClockSpeed(frequency);
{{#unless core}}
SPI1.setClockSpeed(value, scale);
SPI1.setClockSpeed(frequency);
{{/unless}}
{{#if electron}}
SPI2.setClockSpeed(value, scale);
SPI2.setClockSpeed(frequency);
{{/if}}
```

```
// EXAMPLE
// Set the clock speed as close to 15MHz (but not over)
SPI.setClockSpeed(15, MHZ);
SPI.setClockSpeed(15000000);
```

The clock speed cannot be set to any arbitrary value, but is set internally by using a
divider (see `SPI.setClockDivider()`) that gives the highest clock speed not greater
than the one specified.

This method can make writing portable code easier, since it specifies the clock speed
absolutely, giving comparable results across devices. In contrast, specifying
the clock speed using dividers is typically not portable since is dependent upon the system clock speed.

### setClockDividerReference

This function aims to ease porting code from other platforms by setting the clock speed that
`SPI.setClockDivider` is relative to.

For example, when porting an Arduino SPI library, each to `SPI.setClockDivider()` would
need to be changed to reflect the system clock speed of the device being used.

This can be avoided by placing a call to `SPI.setClockDividerReference()` before the other SPI calls.

```cpp

// setting divider reference

// place this early in the library code
SPI.setClockDividerReference(SPI_CLK_ARDUINO);

// then all following calls to setClockDivider() will give comparable clock speeds
// to running on the Arduino Uno

// sets the clock to as close to 4MHz without going over.
SPI.setClockDivider(SPI_CLK_DIV4);
```

The default clock divider reference is the system clock.  {{#if core}}On the Core, this is 72 MHz.{{/if}} {{#unless core}}On the Photon and Electron, the system clock speeds are:
- SPI - 60 MHz
- SPI1 - 30 MHz
{{/unless}}

### setClockDivider()

Sets the SPI clock divider relative to the selected clock reference. The available dividers  are 2, 4, 8, 16, 32, 64, 128 or 256. The default setting is SPI_CLOCK_DIV4, which sets the SPI clock to one-quarter the frequency of the system clock.

```C++
// SYNTAX
SPI.setClockDivider(divider);
{{#unless core}}
SPI1.setClockDivider(divider);
{{/unless}}
{{#if electron}}
SPI2.setClockDivider(divider);
{{/if}}
```
Where the parameter, `divider` can be:

 - `SPI_CLOCK_DIV2`
 - `SPI_CLOCK_DIV4`
 - `SPI_CLOCK_DIV8`
 - `SPI_CLOCK_DIV16`
 - `SPI_CLOCK_DIV32`
 - `SPI_CLOCK_DIV64`
 - `SPI_CLOCK_DIV128`
 - `SPI_CLOCK_DIV256`

### setDataMode()

Sets the SPI data mode: that is, clock polarity and phase. See the [Wikipedia article on SPI](http://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus) for details.

```C++
// SYNTAX
SPI.setDataMode(mode);
{{#unless core}}
SPI1.setDataMode(mode);
{{/unless}}
{{#if electron}}
SPI2.setDataMode(mode);
{{/if}}
```
Where the parameter, `mode` can be:

 - `SPI_MODE0`
 - `SPI_MODE1`
 - `SPI_MODE2`
 - `SPI_MODE3`

### transfer()

Transfers one byte over the SPI bus, both sending and receiving.

```C++
// SYNTAX
SPI.transfer(val);
{{#unless core}}
SPI1.transfer(val);
{{/unless}}
{{#if electron}}
SPI2.transfer(val);
{{/if}}
```
Where the parameter `val`, can is the byte to send out over the SPI bus.

{{#unless core}}
### transfer(void\*, void\*, size_t, std::function)

For transferring a large number of bytes, this form of transfer() uses DMA to speed up SPI data transfer and at the same time allows you to run code in parallel to the data transmission. The function initialises, configures and enables the DMA peripheral’s channel and stream for the selected SPI peripheral for both outgoing and incoming data and initiates the data transfer. If a user callback function is passed then it will be called after completion of the DMA transfer. This results in asynchronous filling of RX buffer after which the DMA transfer is disabled till the transfer function is called again. If NULL is passed as a callback then the result is synchronous i.e. the function will only return once the DMA transfer is complete.

**Note**: The SPI protocol is based on a one byte OUT / one byte IN inteface. For every byte expected to be received, one (dummy, typically 0x00 or 0xFF) byte must be sent.

```C++
// SYNTAX
SPI.transfer(tx_buffer, rx_buffer, length, myFunction);
{{#unless core}}
SPI1.transfer(tx_buffer, rx_buffer, length, myFunction);
{{/unless}}
{{#if electron}}
SPI2.transfer(tx_buffer, rx_buffer, length, myFunction);
{{/if}}
```

Parameters:

- `tx_buffer`: array of Tx bytes that is filled by the user before starting the SPI transfer. If `NULL`, default dummy 0xFF bytes will be clocked out.
- `rx_buffer`: array of Rx bytes that will be filled by the slave during the SPI transfer. If `NULL`, the received data will be discarded.
- `length`: number of data bytes that are to be transferred
- `myFunction`: user specified function callback to be called after completion of the SPI DMA transfer

NOTE: `tx_buffer` and `rx_buffer` sizes MUST be identical (of size `length`)

_Since 0.5.0_ When SPI peripheral is configured in slave mode, the transfer will be canceled when the master deselects this slave device. The user application can check the actual number of bytes received/transmitted by calling `available()`.

### transferCancel()

_Since 0.5.0_

Aborts the configured DMA transfer and disables the DMA peripheral’s channel and stream for the selected SPI peripheral for both outgoing and incoming data.

**Note**: The user specified SPI DMA transfer completion function will still be called to indicate the end of DMA transfer. The user application can check the actual number of bytes received/transmitted by calling `available()`.

### onSelect()

_Since 0.5.0_

Registers a function to be called when the SPI master selects or deselects this slave device by pulling configured slave-select pin low (selected) or high (deselected).

Parameters: `handler`: the function to be called when the slave is selected or deselected; this should take a single uint8_t parameter (the current state: `1` - selected, `0` - deselected) and return nothing, e.g.: `void myHandler(uint8_t state)`

```C++
// SPI slave example
static uint8_t rx_buffer[64];
static uint8_t tx_buffer[64];
static uint32_t select_state = 0x00;
static uint32_t transfer_state = 0x00;

void onTransferFinished() {
    transfer_state = 1;
}

void onSelect(uint8_t state) {
    if (state)
        select_state = state;
}

/* executes once at startup */
void setup() {
    Serial.begin(9600);
    for (int i = 0; i < sizeof(tx_buffer); i++)
      tx_buffer[i] = (uint8_t)i;
    SPI.onSelect(onSelect);
    SPI.begin(SPI_MODE_SLAVE, A2);
}

/* executes continuously after setup() runs */
void loop() {
    while (1) {
        while(select_state == 0);
        select_state = 0;

        transfer_state = 0;
        SPI.transfer(tx_buffer, rx_buffer, sizeof(rx_buffer), onTransferFinished);
        while(transfer_state == 0);
        if (SPI.available() > 0) {
            Serial.printf("Received %d bytes", SPI.available());
            Serial.println();
            for (int i = 0; i < SPI.available(); i++) {
                Serial.printf("%02x ", rx_buffer[i]);
            }
            Serial.println();
        }
    }
}
```

### available()

_Since 0.5.0_

Returns the number of bytes available for reading in the `rx_buffer` supplied in `transfer()`. In general, returns the actual number of bytes received/transmitted during the ongoing or finished DMA transfer.

```C++
// SYNTAX
SPI.available();
```

Returns the number of bytes available.

{{/unless}}

Wire (I2C)
----

{{#unless electron}}
![I2C](/assets/images/core-pin-i2c.jpg)
{{/unless}}

This library allows you to communicate with I2C / TWI(Two Wire Interface) devices. On the Core/Photon/Electron, D0 is the Serial Data Line (SDA) and D1 is the Serial Clock (SCL). {{#if electron}}Additionally on the Electron, there is an alternate pin location for the I2C interface: C4 is the Serial Data Line (SDA) and C5 is the Serial Clock (SCL).{{/if}} Both SCL and SDA pins are open-drain outputs that only pull LOW and typically operate with 3.3V logic, but are tolerant to 5V. Connect a pull-up resistor(1.5k to 10k) on the SDA line to 3V3. Connect a pull-up resistor(1.5k to 10k) on the SCL line to 3V3.  If you are using a breakout board with an I2C peripheral, check to see if it already incorporates pull-up resistors.

These pins are used via the `Wire` object.

* `SCL` => `D1`
* `SDA` => `D0`

{{#if electron}}
Additionally on the Electron, there is an alternate pin location for the I2C interface, which can
be used via the `Wire1` object. This alternate location is mapped as follows:
* `SCL` => `C5`
* `SDA` => `C4`

**Note**: Because there are multiple I2C locations available, be sure to use the same `Wire` or `Wire1` object with all associated functions. I.e.,

Do **NOT** use **Wire**.begin() with **Wire1**.write();

**Do** use **Wire1**.begin() with **Wire1**.transfer();
{{/if}}

### setSpeed()

Sets the I2C clock speed. This is an optional call (not from the original Arduino specs.) and must be called once before calling begin().  The default I2C clock speed is 100KHz and the maximum clock speed is 400KHz.

```C++
// SYNTAX
Wire.setSpeed(clockSpeed);
Wire.begin();
```

Parameters:

- `clockSpeed`: CLOCK_SPEED_100KHZ, CLOCK_SPEED_400KHZ or a user specified speed in hertz (e.g. `Wire.setSpeed(20000)` for 20kHz)

### stretchClock()

Enables or Disables I2C clock stretching. This is an optional call (not from the original Arduino specs.) and must be called once before calling begin(). The default I2C stretch mode is disabled.

```C++
// SYNTAX
Wire.stretchClock(stretch);
Wire.begin();
```

Parameters:

- `stretch`: boolean. `true` will enable clock stretching. `false` will disable clock stretching.


### begin()

Initiate the Wire library and join the I2C bus as a master or slave. This should normally be called only once.

```C++
// SYNTAX
Wire.begin();
Wire.begin(address);
```

Parameters: `address`: the 7-bit slave address (optional); if not specified, join the bus as an I2C master.  If address is specified, join the bus as an I2C slave.


### end()

*Since 0.4.6.*

Releases the I2C bus so that the pins used by the I2C bus are available for general purpose I/O.

### isEnabled()

Used to check if the Wire library is enabled already.  Useful if using multiple slave devices on the same I2C bus.  Check if enabled before calling Wire.begin() again.

```C++
// SYNTAX
Wire.isEnabled();
```

Returns: boolean `true` if I2C enabled, `false` if I2C disabled.

```C++
// EXAMPLE USAGE

// Initialize the I2C bus if not already enabled
if ( !Wire.isEnabled() ) {
    Wire.begin();
}
```

### requestFrom()

Used by the master to request bytes from a slave device. The bytes may then be retrieved with the `available()` and `read()` functions.

```C++
// SYNTAX
Wire.requestFrom(address, quantity);
Wire.requestFrom(address, quantity, stop) ;
```

Parameters:

- `address`: the 7-bit address of the device to request bytes from
- `quantity`: the number of bytes to request (Max. 32)
- `stop`: boolean. `true` will send a stop message after the request, releasing the bus. `false` will continually send a restart after the request, keeping the connection active. The bus will not be released, which prevents another master device from transmitting between messages. This allows one master device to send multiple transmissions while in control.  If no argument is specified, the default value is `true`.

Returns: `byte` : the number of bytes returned from the slave device.  If a timeout occurs, will return `0`.

### reset()

*Since 0.4.6.*

Attempts to reset the I2C bus. This should be called only if the I2C bus has
has hung. In 0.4.6 additional rework was done for the I2C bus on the Photon and Electron, so
we hope this function isn't required, and it's provided for completeness.

### beginTransmission()

Begin a transmission to the I2C slave device with the given address. Subsequently, queue bytes for transmission with the `write()` function and transmit them by calling `endTransmission()`.

```C++
// SYNTAX
Wire.beginTransmission(address);
```

Parameters: `address`: the 7-bit address of the device to transmit to.

### endTransmission()

Ends a transmission to a slave device that was begun by `beginTransmission()` and transmits the bytes that were queued by `write()`.


```C++
// SYNTAX
Wire.endTransmission();
Wire.endTransmission(stop);
```

Parameters: `stop` : boolean.
`true` will send a stop message after the last byte, releasing the bus after transmission. `false` will send a restart, keeping the connection active. The bus will not be released, which prevents another master device from transmitting between messages. This allows one master device to send multiple transmissions while in control.  If no argument is specified, the default value is `true`.

Returns: `byte`, which indicates the status of the transmission:

- 0: success
- 1: busy timeout upon entering endTransmission()
- 2: START bit generation timeout
- 3: end of address transmission timeout
- 4: data byte transfer timeout
- 5: data byte transfer succeeded, busy timeout immediately after

### write()

Writes data from a slave device in response to a request from a master, or queues bytes for transmission from a master to slave device (in-between calls to `beginTransmission()` and `endTransmission()`). Buffer size is truncated to 32 bytes; writing bytes beyond 32 before calling endTransmission() will be ignored.

```C++
// SYNTAX
Wire.write(value);
Wire.write(string);
Wire.write(data, length);
```
Parameters:

- `value`: a value to send as a single byte
- `string`: a string to send as a series of bytes
- `data`: an array of data to send as bytes
- `length`: the number of bytes to transmit (Max. 32)

Returns:  `byte`

`write()` will return the number of bytes written, though reading that number is optional.

```C++
// EXAMPLE USAGE

// Master Writer running on Device No.1 (Use with corresponding Slave Reader running on Device No.2)

void setup()
{
  Wire.begin();              // join i2c bus as master
}

byte x = 0;

void loop()
{
  Wire.beginTransmission(4); // transmit to slave device #4
  Wire.write("x is ");       // sends five bytes
  Wire.write(x);             // sends one byte
  Wire.endTransmission();    // stop transmitting

  x++;
  delay(500);
}
```

### available()

Returns the number of bytes available for retrieval with `read()`. This should be called on a master device after a call to `requestFrom()` or on a slave inside the `onReceive()` handler.

```C++
Wire.available();
```

Returns: The number of bytes available for reading.

### read()

Reads a byte that was transmitted from a slave device to a master after a call to `requestFrom()` or was transmitted from a master to a slave. `read()` inherits from the `Stream` utility class.

```C++
// SYNTAX
Wire.read() ;
```

Returns: The next byte received

```C++
// EXAMPLE USAGE

// Master Reader running on Device No.1 (Use with corresponding Slave Writer running on Device No.2)

void setup()
{
  Wire.begin();              // join i2c bus as master
  Serial.begin(9600);        // start serial for output
}

void loop()
{
  Wire.requestFrom(2, 6);    // request 6 bytes from slave device #2

  while(Wire.available())    // slave may send less than requested
  {
    char c = Wire.read();    // receive a byte as character
    Serial.print(c);         // print the character
  }

  delay(500);
}
```

### peek()

Similar in use to read(). Reads (but does not remove from the buffer) a byte that was transmitted from a slave device to a master after a call to `requestFrom()` or was transmitted from a master to a slave. `read()` inherits from the `Stream` utility class. Useful for peeking at the next byte to be read.

```C++
// SYNTAX
Wire.peek();
```

Returns: The next byte received (without removing it from the buffer)

### onReceive()

Registers a function to be called when a slave device receives a transmission from a master.

Parameters: `handler`: the function to be called when the slave receives data; this should take a single int parameter (the number of bytes read from the master) and return nothing, e.g.: `void myHandler(int numBytes) `

```C++
// EXAMPLE USAGE

// Slave Reader running on Device No.2 (Use with corresponding Master Writer running on Device No.1)

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  while(1 < Wire.available())   // loop through all but the last
  {
    char c = Wire.read();       // receive byte as a character
    Serial.print(c);            // print the character
  }
  int x = Wire.read();          // receive byte as an integer
  Serial.println(x);            // print the integer
}

void setup()
{
  Wire.begin(4);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600);           // start serial for output
}

void loop()
{
  delay(100);
}
```

### onRequest()

Register a function to be called when a master requests data from this slave device.

Parameters: `handler`: the function to be called, takes no parameters and returns nothing, e.g.: `void myHandler() `

```C++
// EXAMPLE USAGE

// Slave Writer running on Device No.2 (Use with corresponding Master Reader running on Device No.1)

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  Wire.write("hello ");         // respond with message of 6 bytes as expected by master
}

void setup()
{
  Wire.begin(2);                // join i2c bus with address #2
  Wire.onRequest(requestEvent); // register event
}

void loop()
{
  delay(100);
}
```

{{#unless core}}

## CAN (CANbus)

![CAN bus](/assets/images/can.png)

*Since 0.4.9*

<a href="https://en.wikipedia.org/wiki/CAN_bus" target="_blank">Controller area network (CAN bus)</a> is a bus used in most automobiles, as well as some industrial equipment, for communication between different microcontrollers.

The Photon and Electron support communicating with CAN devices via the CAN bus.

- The Photon and Electron have a CANbus on pins D1 (CAN2_TX) and D2 (CAN2_RX).
- The Electron only, has a second CANbus on pins C4 (CAN1_TX) and C5 (CAN1_TX).

**Note**: an additional CAN transceiver integrated circuit is needed to convert the logic-level voltages of the Photon or Electron to the voltage levels of the CAN bus.

On the Photon or Electron, connect pin D1 to the TX pin of the CAN transceiver and pin D2 to the RX pin.

On the Electron only, connect pin C4 to the TX pin of the CAN transceiver and pin C5 to the RX pin.

```
// EXAMPLE USAGE on pins D1 & D2
CANChannel can(CAN_D1_D2);

void setup() {
    can.begin(125000); // pick the baud rate for your network
    // accept one message. If no filter added by user then accept all messages
    can.addFilter(0x100, 0x7FF);
}

void loop() {
    CANMessage message;

    Message.id = 0x100;
    can.transmit(message);

    delay(10);

    if(can.receive(message)) {
        // message received
    }
}
```

### CANMessage

The CAN message struct has these members:

```
struct CANMessage
{
   uint32_t id;
   bool     extended;
   bool     rtr;
   uint8_t  len;
   uint8_t  data[8];
}
```

### CANChannel

Create a `CANChannel` global object to connect to a CAN bus on the specified pins.

```C++
// SYNTAX
CANChannel can(pins, rxQueueSize, txQueueSize);
```

Parameters:

- `pins`: the Photon and Electron support pins `CAN_D1_D2`, and the Electron only, supports pins `CAN_C4_C5`
- `rxQueueSize` (optional): the receive queue size (default 32 message)
- `txQueueSize` (optional): the transmit queue size (default 32 message)

```C++
// EXAMPLE
CANChannel can(CAN_D1_D2);
// Buffer 10 received messages and 5 transmitted messages
CANChannel can(CAN_D1_D2, 10, 5);
```

### begin()

Joins the bus at the given `baud` rate.

```C++
// SYNTAX
can.begin(baud, flags);
```

Parameters:

- `baud`: common baud rates are 50000, 100000, 125000, 250000, 500000, 1000000
- `flags` (optional): `CAN_TEST_MODE` to run the CAN bus in test mode where every transmitted message will be received back

```C++
// EXAMPLE
CANChannel can(CAN_D1_D2);
can.begin(500000);
// Use for testing without a CAN transceiver
can.begin(500000, CAN_TEST_MODE);
```

### end()

Disconnect from the bus.

```
// SYNTAX
CANChannel can(CAN_D1_D2);
can.end();
```

### available()

The number of received messages waiting in the receive queue.

Returns: `uint8_t` : the number of messages.

```
// SYNTAX
uint8_t count = can.available();
```

```
// EXAMPLE
CANChannel can(CAN_D1_D2);
if(can.available() > 0) {
  // there are messages waiting
}
```

### receive()

Take a received message from the receive queue. This function does not wait for a message to arrive.

```
// SYNTAX
can.receive(message);
```

Parameters:

- `message`: where the received message will be copied

Returns: boolean `true` if a message was received, `false` if the receive queue was empty.

```
// EXAMPLE
CANChannel can(CAN_D1_D2);
CANMessage message;
if(can.receive(message)) {
  Serial.println(message.id);
  Serial.println(message.len);
}
```

### transmit()

Add a message to the queue to be transmitted to the CAN bus as soon as possible.

```
// SYNTAX
can.transmit(message);
```

Parameters:

- `message`: the message to be transmitted

Returns: boolean `true` if the message was added to the queue, `false` if the transmit queue was full.

```
// EXAMPLE
CANChannel can(CAN_D1_D2);
CANMessage message;
message.id = 0x100;
message.length = 1;
message.data[0] = 42;
can.transmit(message);
```

**Note**: Since the CAN bus requires at least one other CAN node to acknowledge transmitted messages if the Photon or Electron is alone on the bus (such as when using a CAN shield with no other CAN node connected) then messages will never be transmitted and the transmit queue will fill up.

### addFilter()

Filter which messages will be added to the receive queue.

```
// SYNTAX
can.addFilter(id, mask);
can.addFilter(id, mask, type);
```

By default all messages are received. When filters are added, only messages matching the filters will be received. Others will be discarded.

Parameters:

- `id`: the id pattern to match
- `mask`: the mask pattern to match
- `type` (optional): `CAN_FILTER_STANDARD` (default) or `CAN_FILTER_EXTENDED`

Returns: boolean `true` if the filter was added, `false` if there are too many filters (14 filters max).

```
// EXAMPLES
CANChannel can(CAN_D1_D2);
// Match only message ID 0x100
can.addFilter(0x100, 0x7FF);
// Match any message with the highest ID bit set
can.addFilter(0x400, 0x400);
// Match any message with the higest ID bit cleared
can.addFilter(0x0, 0x400);
// Match only messages with extended IDs
can.addFilter(0, 0, CAN_FILTER_EXTENDED);
```

### clearFilters()

Clear filters and accept all messages.

```
// SYNTAX
CANChannel can(CAN_D1_D2);
can.clearFilters();
```

### isEnabled()

Used to check if the CAN bus is enabled already.  Check if enabled before calling can.begin() again.

```
// SYNTAX
CANChannel can(CAN_D1_D2);
can.isEnabled();
```

Returns: boolean `true` if the CAN bus is enabled, `false` if the CAN bus is disabled.

### errorStatus()

Get the current error status of the CAN bus.

```
// SYNTAX
int status = can.errorStatus();
```

Returns: int `CAN_NO_ERROR` when everything is ok, `CAN_ERROR_PASSIVE` when not attempting to transmit messages but still acknowledging messages, `CAN_BUS_OFF` when not transmitting or acknowledging messages.

```
// EXAMPLE
CANChannel can(CAN_D1_D2);
if(can.errorStatus() == CAN_BUS_OFF) {
  Serial.println("Not properly connected to CAN bus");
}
```

This value is only updated when attempting to transmit messages.

The two most common causes of error are: being alone on the bus (such as when using a CAN shield not connected to anything) or using the wrong baud rate. Attempting to transmit in those situations will result in `CAN_BUS_OFF`.

Errors heal automatically when properly communicating with other microcontrollers on the CAN bus.
{{/unless}}

## IPAddress

Creates an IP address that can be used with TCPServer, TCPClient, and UDP objects.

```C++
// EXAMPLE USAGE

IPAddress localIP;
IPAddress server(8,8,8,8);
IPAddress IPfromInt( 167772162UL );  // 10.0.0.2 as 10*256^3+0*256^2+0*256+2
uint8_t server[] = { 10, 0, 0, 2};
IPAddress IPfromBytes( server );
```

The IPAddress also allows for comparisons.

```C++
if (IPfromInt == IPfromBytes)
{
  Serial.println("Same IP addresses");
}
```

You can also use indexing the get or change individual bytes in the IP address.

```C++
// PING ALL HOSTS ON YOUR SUBNET EXCEPT YOURSELF
IPAddress localIP = WiFi.localIP();
uint8_t myLastAddrByte = localIP[3];
for(uint8_t ipRange=1; ipRange<255; ipRange++)
{
  if (ipRange != myLastAddrByte)
  {
    localIP[3] = ipRange;
    WiFi.ping(localIP);
  }
}
```

You can also assign to an IPAddress from an array of uint8's or a 32-bit unsigned integer.

```C++
IPAddress IPfromInt;  // 10.0.0.2 as 10*256^3+0*256^2+0*256+2
IPfromInt = 167772162UL;
uint8_t server[] = { 10, 0, 0, 2};
IPAddress IPfromBytes;
IPfromBytes = server;
```

Finally IPAddress can be used directly with print.

```C++
// PRINT THE DEVICE'S IP ADDRESS IN
// THE FORMAT 192.168.0.10
IPAddress myIP = WiFi.localIP();
Serial.println(myIP);    // prints the device's IP address
```

## TCPServer

Create a server that listens for incoming connections on the specified port.

```C++
// SYNTAX
TCPServer server = TCPServer(port);
```

Parameters: `port`: the port to listen on (`int`)

```C++
// EXAMPLE USAGE

// telnet defaults to port 23
TCPServer server = TCPServer(23);
TCPClient client;

void setup()
{
  // start listening for clients
  server.begin();

  // Make sure your Serial Terminal app is closed before powering your device
  Serial.begin(9600);
  // Now open your Serial Terminal, and hit any key to continue!
  while(!Serial.available()) Particle.process();

  Serial.println(WiFi.localIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.SSID());
}

void loop()
{
  if (client.connected()) {
    // echo all available bytes back to the client
    while (client.available()) {
      server.write(client.read());
    }
  } else {
    // if no client is yet connected, check for a new connection
    client = server.available();
  }
}
```

### begin()

Tells the server to begin listening for incoming connections.

```C++
// SYNTAX
server.begin();
```

### available()

Gets a client that is connected to the server and has data available for reading. The connection persists when the returned client object goes out of scope; you can close it by calling `client.stop()`.

`available()` inherits from the `Stream` utility class.

### write()

Write data to the last client that connected to a server. This data is sent as a byte or series of bytes.

```C++
// SYNTAX
server.write(val);
server.write(buf, len);
```

Parameters:

- `val`: a value to send as a single byte (byte or char)
- `buf`: an array to send as a series of bytes (byte or char)
- `len`: the length of the buffer

Returns: `byte`: `write()` returns the number of bytes written. It is not necessary to read this.

### print()

Print data to the last client connected to a server. Prints numbers as a sequence of digits, each an ASCII character (e.g. the number 123 is sent as the three characters '1', '2', '3').

```C++
// SYNTAX
server.print(data);
server.print(data, BASE) ;
```

Parameters:

- `data`: the data to print (char, byte, int, long, or string)
- `BASE`(optional): the base in which to print numbers: BIN for binary (base 2), DEC for decimal (base 10), OCT for octal (base 8), HEX for hexadecimal (base 16).

Returns:  `byte`:  `print()` will return the number of bytes written, though reading that number is optional

### println()

Print data, followed by a newline, to the last client connected to a server. Prints numbers as a sequence of digits, each an ASCII character (e.g. the number 123 is sent as the three characters '1', '2', '3').

```C++
// SYNTAX
server.println();
server.println(data);
server.println(data, BASE) ;
```

Parameters:

- `data` (optional): the data to print (char, byte, int, long, or string)
- `BASE` (optional): the base in which to print numbers: BIN for binary (base 2), DEC for decimal (base 10), OCT for octal (base 8), HEX for hexadecimal (base 16).


## TCPClient

Creates a client which can connect to a specified internet IP address and port (defined in the `client.connect()` function).

```C++
// SYNTAX
TCPClient client;
```

```C++
// EXAMPLE USAGE

TCPClient client;
byte server[] = { 74, 125, 224, 72 }; // Google
void setup()
{
  // Make sure your Serial Terminal app is closed before powering your device
  Serial.begin(9600);
  // Now open your Serial Terminal, and hit any key to continue!
  while(!Serial.available()) Particle.process();

  Serial.println("connecting...");

  if (client.connect(server, 80))
  {
    Serial.println("connected");
    client.println("GET /search?q=unicorn HTTP/1.0");
    client.println("Host: www.google.com");
    client.println("Content-Length: 0");
    client.println();
  }
  else
  {
    Serial.println("connection failed");
  }
}

void loop()
{
  if (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }

  if (!client.connected())
  {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    for(;;);
  }
}
```

### connected()

Whether or not the client is connected. Note that a client is considered connected if the connection has been closed but there is still unread data.

```C++
// SYNTAX
client.connected();
```

Returns true if the client is connected, false if not.

### connect()

Connects to a specified IP address and port. The return value indicates success or failure. Also supports DNS lookups when using a domain name.

```C++
// SYNTAX
client.connect();
client.connect(ip, port);
client.connect(URL, port);
```

Parameters:

- `ip`: the IP address that the client will connect to (array of 4 bytes)
- `URL`: the domain name the client will connect to (string, ex.:"particle.io")
- `port`: the port that the client will connect to (`int`)

Returns true if the connection succeeds, false if not.

### write()

Write data to the server the client is connected to. This data is sent as a byte or series of bytes.

```C++
// SYNTAX
client.write(val);
client.write(buf, len);
```

Parameters:

- `val`: a value to send as a single byte (byte or char)
- `buf`: an array to send as a series of bytes (byte or char)
- `len`: the length of the buffer

Returns: `byte`: `write()` returns the number of bytes written. It is not necessary to read this value.

### print()

Print data to the server that a client is connected to. Prints numbers as a sequence of digits, each an ASCII character (e.g. the number 123 is sent as the three characters '1', '2', '3').

```C++
// SYNTAX
client.print(data);
client.print(data, BASE) ;
```

Parameters:

- `data`: the data to print (char, byte, int, long, or string)
- `BASE`(optional): the base in which to print numbers: BIN for binary (base 2), DEC for decimal (base 10), OCT for octal (base 8), HEX for hexadecimal (base 16).

Returns:  `byte`:  `print()` will return the number of bytes written, though reading that number is optional

### println()

Print data, followed by a carriage return and newline, to the server a client is connected to. Prints numbers as a sequence of digits, each an ASCII character (e.g. the number 123 is sent as the three characters '1', '2', '3').

```C++
// SYNTAX
client.println();
client.println(data);
client.println(data, BASE) ;
```

Parameters:

- `data` (optional): the data to print (char, byte, int, long, or string)
- `BASE` (optional): the base in which to print numbers: BIN for binary (base 2), DEC for decimal (base 10), OCT for octal (base 8), HEX for hexadecimal (base 16).

### available()

Returns the number of bytes available for reading (that is, the amount of data that has been written to the client by the server it is connected to).

```C++
// SYNTAX
client.available();
```

Returns the number of bytes available.

### read()
Read the next byte received from the server the client is connected to (after the last call to `read()`).

```C++
// SYNTAX
client.read();
```

Returns the next byte (or character), or -1 if none is available.

### flush()

Discard any bytes that have been written to the client but not yet read.

```C++
// SYNTAX
client.flush();
```

### remoteIP()

_Since 0.4.5_

Retrieves the remote `IPAddress` of a connected `TCPClient`. When the `TCPClient` is retrieved
from `TCPServer.available()` (where the client is a remote client connecting to a local server) the
`IPAddress` gives the remote address of the connecting client.

When `TCPClient` was created directly via `TCPClient.connect()`, then `remoteIP`
returns the remote server the client is connected to.

```C++

// EXAMPLE - TCPClient from TCPServer

TCPServer server(80);
// ...

void setup()
{
    Serial.begin(9600);
    server.begin(80);
}

void loop()
{
    // check for a new client to our server
    TCPClient client = server.available();
    if (client.connected())
    {
        // we got a new client
        // find where the client's remote address
        IPAddress clientIP = client.remoteIP();
        // print the address to Serial
        Serial.println(clientIP);
    }
}
```

```C++
// EXAMPLE - TCPClient.connect()

TCPClient client;
client.connect("www.google.com", 80);
if (client.connected())
{
    IPAddress clientIP = client.remoteIP();
    // IPAddress equals whatever www.google.com resolves to
}

```


### stop()

Disconnect from the server.

```C++
// SYNTAX
client.stop();
```


## UDP

This class enables UDP messages to be sent and received.

```cpp
// EXAMPLE USAGE

// UDP Port used for two way communication
unsigned int localPort = 8888;

// An UDP instance to let us send and receive packets over UDP
UDP Udp;

void setup() {
  // start the UDP
  Udp.begin(localPort);

  // Print your device IP Address via serial
  Serial.begin(9600);
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check if data has been received
  if (Udp.parsePacket() > 0) {

    // Read first char of data received
    char c = Udp.read();

    // Ignore other chars
    Udp.flush();

    // Store sender ip and port
    IPAddress ipAddress = Udp.remoteIP();
    int port = Udp.remotePort();

    // Echo back data to sender
    Udp.beginPacket(ipAddress, port);
    Udp.write(c);
    Udp.endPacket();
  }
}
```

_Note that UDP does not guarantee that messages are always delivered, or that
they are delivered in the order supplied. In cases where your application
requires a reliable connection, `TCPClient` is a simpler alternative._

{{#if core}}
The UDP protocol implementation has known issues that will require extra consideration when programming with it. Please refer to the Known Issues category of the Community for details. The are also numerous working examples and workarounds in the searchable Community topics.
{{/if}}

There are two primary ways of working with UDP - buffered operation and unbuffered operation.

1. buffered operation allows you to read and write packets in small pieces, since the system takes care of allocating the required buffer to hold the entire packet.
 - to read a buffered packet, call `parsePacket`, then use `available` and `read` to retrieve the packet received
 - to write a buffered packet, optionally call `setBuffer` to set the maximum size of the packet (the default is 512 bytes), followed by
  `beginPacket`, then as many calls to `write`/`print` as necessary to build the packet contents, followed finally by `end` to send the packet over the network.

2. unbuffered operation allows you to read and write entire packets in a single operation - your application is responsible for allocating the buffer to contain the packet to be sent or received over the network.
 - to read an unbuffered packet, call `receivePacket` with a buffer to hold the received packet.
 - to write an unbuffered packet,  call `sendPacket` with the packet buffer to send, and the destination address.


<!-- TO DO -->
<!-- Add more examples-->

### begin()

Initializes the UDP library and network settings.

```cpp
// SYNTAX
Udp.begin(port);
```

### available()

Get the number of bytes (characters) available for reading from the buffer. This is data that's already arrived.

```cpp
// SYNTAX
int count = Udp.available();
```

This function can only be successfully called after `UDP.parsePacket()`.

`available()` inherits from the `Stream` utility class.

Returns the number of bytes available to read.

### beginPacket()

Starts a connection to write UDP data to the remote connection.

```cpp
// SYNTAX
Udp.beginPacket(remoteIP, remotePort);
```

Parameters:

 - `remoteIP`: the IP address of the remote connection (4 bytes)
 - `remotePort`: the port of the remote connection (int)

It returns nothing.

### endPacket()

Called after writing buffered UDP data using `write()` or `print()`. The buffered data is then sent to the
remote UDP peer.


```cpp
// SYNTAX
Udp.endPacket();
```

Parameters: NONE

### write()

Writes UDP data to the buffer - no data is actually sent. Must be wrapped between `beginPacket()` and `endPacket()`. `beginPacket()` initializes the packet of data, it is not sent until `endPacket()` is called.

```cpp
// SYNTAX
Udp.write(message);
Udp.write(buffer, size);
```

Parameters:

 - `message`: the outgoing message (char)
 - `buffer`: an array to send as a series of bytes (byte or char)
 - `size`: the length of the buffer

Returns:

 - `byte`: returns the number of characters sent. This does not have to be read


### parsePacket()

Checks for the presence of a UDP packet, and reports the size. `parsePacket()` must be called before reading the buffer with `UDP.read()`.

```cpp
// SYNTAX
size = Udp.parsePacket();
```

Parameters: NONE

Returns:

 - `int`: the size of a received UDP packet

### read()

Reads UDP data from the specified buffer. If no arguments are given, it will return the next character in the buffer.

This function can only be successfully called after `UDP.parsePacket()`.

```cpp
// SYNTAX
count = Udp.read();
count = Udp.read(packetBuffer, MaxSize);
```
Parameters:

 - `packetBuffer`: buffer to hold incoming packets (char)
 - `MaxSize`: maximum size of the buffer (int)

Returns:

 - `int`: returns the character in the buffer or -1 if no character is available


### stop()

Disconnect from the server. Release any resource being used during the UDP session.

```cpp
// SYNTAX
Udp.stop();
```

Parameters: NONE

### remoteIP()

Returns the IP address of sender of the packet parsed by `Udp.parsePacket()`/`Udp.receivePacket()`.

```cpp
// SYNTAX
ip = Udp.remoteIP();
```

Parameters: NONE

Returns:

 - IPAddress : the IP address of the sender of the packet parsed by `Udp.parsePacket()`/`Udp.receivePacket()`.

### remotePort()

Returns the port from which the UDP packet was sent. The packet is the one most recently processed by `Udp.parsePacket()`/`Udp.receivePacket()`.

```cpp
// SYNTAX
int port = Udp.remotePort();
```

Parameters: NONE

Returns:

- `int`: the port from which the packet parsed by `Udp.parsePacket()`/`Udp.receivePacket()` was sent.


### setBuffer()

_Since 0.4.5_

Initializes the buffer used by a `UDP` instance for buffered reads/writes. The buffer
is used when your application calls `beginPacket()` and `parsePacket()`.  If `setBuffer()` isn't called,
the buffer size defaults to 512 bytes, and is allocated when buffered operation is initialized via `beginPacket()` or `parsePacket()`.

```cpp
// SYNTAX
Udp.setBuffer(size); // dynamically allocated buffer
Udp.setBuffer(size, buffer); // application provided buffer

// EXAMPLE USAGE - dynamically allocated buffer
UDP Udp;

// uses a dynamically allocated buffer that is 1024 bytes in size
if (!Udp.setBuffer(1024))
{
    // on no, couldn't allocate the buffer
}
else
{
    // 'tis good!
}
```

```cpp
// EXAMPLE USAGE - application-provided buffer
UDP Udp;

char appBuffer[800];
Udp.setBuffer(800, appBuffer);
```

Parameters:

- `unsigned int`: the size of the buffer
- `pointer`:  the buffer. If not provided, or `NULL` the system will attempt to
 allocate a buffer of the size requested.

Returns:
- `true` when the buffer was successfully allocated, `false` if there was insufficient memory. (For application-provided buffers
the function always returns `true`.)

### releaseBuffer()

_Since 0.4.5_

Releases the buffer previously set by a call to `setBuffer()`.

```cpp
// SYNTAX
Udp.releaseBuffer();
```

_This is typically required only when performing advanced memory management and the UDP instance is
not scoped to the lifetime of the application._

### sendPacket()

_Since 0.4.5_

Sends a packet, unbuffered, to a remote UDP peer.

```cpp
// SYNTAX
Udp.sendPacket(buffer, bufferSize, remoteIP, remotePort);

// EXAMPLE USAGE
UDP Udp;

char buffer[] = "Particle powered";

IPAddress remoteIP(192, 168, 1, 100);
int port = 1337;

void setup() {
  // Required for two way communication 
  Udp.begin(8888);

  if (Udp.sendPacket(buffer, sizeof(buffer), remoteIP, port) < 0) {
    Particle.publish("Error");
  }
}
```

Parameters:
- `pointer` (buffer): the buffer of data to send
- `int` (bufferSize): the number of bytes of data to send
- `IPAddress` (remoteIP): the destination address of the remote peer
- `int` (remotePort): the destination port of the remote peer

Returns:
- `int`: The number of bytes written. Negative value on error.

{{#if photon}}
### joinMulticast()

_Since 0.4.5_

Join a multicast address for all UDP sockets which are on the same network interface as this one.

```cpp
// SYNTAX
Udp.joinMulticast(IPAddress& ip);

// EXAMPLE USAGE
UDP Udp;

int remotePort = 1024;
IPAddress multicastAddress(224,0,0,0);

Udp.begin(remotePort);
Udp.joinMulticast(multicastAddress);
```

This will allow reception of multicast packets sent to the given address for UDP sockets
which have bound the port to which the multicast packet was sent.
Must be called only after `begin()` so that the network interface is established.

### leaveMulticast()

_Since 0.4.5_

Leaves a multicast group previously joined on a specific multicast address.

```cpp
// SYNTAX
Udp.leaveMulticast(multicastAddress);

// EXAMPLE USAGE
UDP Udp;
IPAddress multicastAddress(224,0,0,0);
Udp.leaveMulticast(multicastAddress);
```

{{/if}}

## Servo

This library allows your device to control RC (hobby) servo motors. Servos have integrated gears and a shaft that can be precisely controlled. Standard servos allow the shaft to be positioned at various angles, usually between 0 and 180 degrees. Continuous rotation servos allow the rotation of the shaft to be set to various speeds.

```cpp
// EXAMPLE CODE

Servo myservo;  // create servo object to control a servo
                // a maximum of eight servo objects can be created

int pos = 0;    // variable to store the servo position

void setup()
{
  myservo.attach(D0);  // attaches the servo on the D0 pin to the servo object
  // Only supported on pins that have PWM
}


void loop()
{
  for(pos = 0; pos < 180; pos += 1)  // goes from 0 degrees to 180 degrees
  {                                  // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  for(pos = 180; pos>=1; pos-=1)     // goes from 180 degrees to 0 degrees
  {
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}
```

**NOTE:** Unlike Arduino, you do not need to include `Servo.h`; it is included automatically.


### attach()

Set up a servo on a particular pin. Note that, Servo can only be attached to pins with a timer.

- on the Core, Servo can be connected to A0, A1, A4, A5, A6, A7, D0, and D1.
- on the Photon, Servo can be connected to A4, A5, WKP, RX, TX, D0, D1, D2, D3

```cpp
// SYNTAX
servo.attach(pin)
```

### write()

Writes a value to the servo, controlling the shaft accordingly. On a standard servo, this will set the angle of the shaft (in degrees), moving the shaft to that orientation. On a continuous rotation servo, this will set the speed of the servo (with 0 being full-speed in one direction, 180 being full speed in the other, and a value near 90 being no movement).

```cpp
// SYNTAX
servo.write(angle)
```

### writeMicroseconds()

Writes a value in microseconds (uS) to the servo, controlling the shaft accordingly. On a standard servo, this will set the angle of the shaft. On standard servos a parameter value of 1000 is fully counter-clockwise, 2000 is fully clockwise, and 1500 is in the middle.

```cpp
// SYNTAX
servo.writeMicroseconds(uS)
```

Note that some manufactures do not follow this standard very closely so that servos often respond to values between 700 and 2300. Feel free to increase these endpoints until the servo no longer continues to increase its range. Note however that attempting to drive a servo past its endpoints (often indicated by a growling sound) is a high-current state, and should be avoided.

Continuous-rotation servos will respond to the writeMicrosecond function in an analogous manner to the write function.


### read()

Read the current angle of the servo (the value passed to the last call to write()). Returns an integer from 0 to 180 degrees.

```cpp
// SYNTAX
servo.read()
```

### attached()

Check whether the Servo variable is attached to a pin. Returns a boolean.

```cpp
// SYNTAX
servo.attached()
```

### detach()

Detach the Servo variable from its pin.

```cpp
// SYNTAX
servo.detach()
```

### setTrim()

Sets a trim value that allows minute timing adjustments to correctly
calibrate 90 as the stationary point.

```cpp
// SYNTAX

// shortens the pulses sent to the servo
servo.setTrim(-3);

// a larger trim value
servo.setTrim(30);

// removes any previously configured trim
servo.setTrim(0);
```


## RGB

This library allows the user to control the RGB LED on the front of the device.

```cpp
// EXAMPLE CODE

// take control of the LED
RGB.control(true);

// red, green, blue, 0-255.
// the following sets the RGB LED to white:
RGB.color(255, 255, 255);

// wait one second
delay(1000);

// scales brightness of all three colors, 0-255.
// the following sets the RGB LED brightness to 25%:
RGB.brightness(64);

// wait one more second
delay(1000);

// resume normal operation
RGB.control(false);
```

### control(user_control)

User can take control of the RGB LED, or give control back to the system.

```cpp
// take control of the RGB LED
RGB.control(true);

// resume normal operation
RGB.control(false);
```

### controlled()

Returns Boolean `true` when the RGB LED is under user control, or `false` when it is not.

```cpp
// take control of the RGB LED
RGB.control(true);

// Print true or false depending on whether
// the RGB LED is currently under user control.
// In this case it prints "true".
Serial.println(RGB.controlled());

// resume normal operation
RGB.control(false);
```

### color(red, green, blue)

Set the color of the RGB with three values, 0 to 255 (0 is off, 255 is maximum brightness for that color).  User must take control of the RGB LED before calling this method.

```cpp
// Set the RGB LED to red
RGB.color(255, 0, 0);

// Sets the RGB LED to cyan
RGB.color(0, 255, 255);

// Sets the RGB LED to white
RGB.color(255, 255, 255);
```

### brightness(val)

Scale the brightness value of all three RGB colors with one value, 0 to 255 (0 is 0%, 255 is 100%).  This setting persists after `RGB.control()` is set to `false`, and will govern the overall brightness of the RGB LED under normal system operation. User must take control of the RGB LED before calling this method.

```cpp
// Scale the RGB LED brightness to 25%
RGB.brightness(64);

// Scale the RGB LED brightness to 50%
RGB.brightness(128);

// Scale the RGB LED brightness to 100%
RGB.brightness(255);
```

### onChange(handler)

Specifies a function to call when the color of the RGB LED changes. It can be used to implement an external RGB LED.

```cpp
// EXAMPLE USAGE

void ledChangeHandler(uint8_t r, uint8_t g, uint8_t b) {
  // Duplicate the green color to an external LED
  analogWrite(D0, g);
}

void setup()
{
  pinMode(D0, OUTPUT);
  RGB.onChange(ledChangeHandler);
}

```

---

`onChange` can also call a method on an object.

```
// Automatically mirror the onboard RGB LED to an external RGB LED
// No additional code needed in setup() or loop()

class ExternalRGB {
  public:
    ExternalRGB(pin_t r, pin_t g, pin_t b) : pin_r(r), pin_g(g), pin_b(b) {
      pinMode(pin_r, OUTPUT);
      pinMode(pin_g, OUTPUT);
      pinMode(pin_b, OUTPUT);
      RGB.onChange(&ExternalRGB::handler, this);
    }

    void handler(uint8_t r, uint8_t g, uint8_t b) {
      analogWrite(pin_r, 255 - r);
      analogWrite(pin_g, 255 - g);
      analogWrite(pin_b, 255 - b);
    }

    private:
      pin_t pin_r;
      pin_t pin_g;
      pin_t pin_b;
};

// Connect an external RGB LED to D0, D1 and D2 (R, G, and B)
ExternalRGB myRGB(D0, D1, D2);
```


## Time

The device synchronizes time with the Particle Cloud during the handshake.
From then, the time is continually updated on the device.
This reduces the need for external libraries to manage dates and times.


### hour()

Retrieve the hour for the current or given time.
Integer is returned without a leading zero.

```cpp
// Print the hour for the current time
Serial.print(Time.hour());

// Print the hour for the given time, in this case: 4
Serial.print(Time.hour(1400647897));
```

Optional parameters: Integer (Unix timestamp)

Returns: Integer 0-23


### hourFormat12()

Retrieve the hour in 12-hour format for the current or given time.
Integer is returned without a leading zero.

```cpp
// Print the hour in 12-hour format for the current time
Serial.print(Time.hourFormat12());

// Print the hour in 12-hour format for the given time, in this case: 15
Serial.print(Time.hourFormat12(1400684400));
```

Optional parameters: Integer (Unix timestamp)

Returns: Integer 1-12


### isAM()

Returns true if the current or given time is AM.

```cpp
// Print true or false depending on whether the current time is AM
Serial.print(Time.isAM());

// Print whether the given time is AM, in this case: true
Serial.print(Time.isAM(1400647897));
```

Optional parameters: Integer (Unix timestamp)

Returns: Unsigned 8-bit integer: 0 = false, 1 = true


### isPM()

Returns true if the current or given time is PM.

```cpp
// Print true or false depending on whether the current time is PM
Serial.print(Time.isPM());

// Print whether the given time is PM, in this case: false
Serial.print(Time.isPM(1400647897));
```

Optional parameters: Integer (Unix timestamp)

Returns: Unsigned 8-bit integer: 0 = false, 1 = true


### minute()

Retrieve the minute for the current or given time.
Integer is returned without a leading zero.

```cpp
// Print the minute for the current time
Serial.print(Time.minute());

// Print the minute for the given time, in this case: 51
Serial.print(Time.minute(1400647897));
```

Optional parameters: Integer (Unix timestamp)

Returns: Integer 0-59


### second()

Retrieve the seconds for the current or given time.
Integer is returned without a leading zero.

```cpp
// Print the second for the current time
Serial.print(Time.second());

// Print the second for the given time, in this case: 51
Serial.print(Time.second(1400647897));
```

Optional parameters: Integer (Unix timestamp)

Returns: Integer 0-59


### day()

Retrieve the day for the current or given time.
Integer is returned without a leading zero.

```cpp
// Print the day for the current time
Serial.print(Time.day());

// Print the minute for the given time, in this case: 21
Serial.print(Time.day(1400647897));
```

Optional parameters: Integer (Unix timestamp)

Returns: Integer 1-31


### weekday()

Retrieve the weekday for the current or given time.

 - 1 = Sunday
 - 2 = Monday
 - 3 = Tuesday
 - 4 = Wednesday
 - 5 = Thursday
 - 6 = Friday
 - 7 = Saturday

```cpp
// Print the weekday number for the current time
Serial.print(Time.weekday());

// Print the weekday for the given time, in this case: 4
Serial.print(Time.weekday(1400647897));
```

Optional parameters: Integer (Unix timestamp)

Returns: Integer 1-7


### month()

Retrieve the month for the current or given time.
Integer is returned without a leading zero.

```cpp
// Print the month number for the current time
Serial.print(Time.month());

// Print the month for the given time, in this case: 5
Serial.print(Time.month(1400647897));
```

Optional parameters: Integer (Unix timestamp)

Returns: Integer 1-12


### year()

Retrieve the 4-digit year for the current or given time.

```cpp
// Print the current year
Serial.print(Time.year());

// Print the year for the given time, in this case: 2014
Serial.print(Time.year(1400647897));
```

Optional parameters: Integer (Unix timestamp)

Returns: Integer


### now()

Retrieve the current time as seconds since January 1, 1970 (commonly known as "Unix time" or "epoch time"). This time is not affected by the timezone setting.

```cpp
// Print the current Unix timestamp
Serial.print(Time.now()); // 1400647897
```

Returns: Integer

### local()

Retrieve the current time in the configured timezone as seconds since January 1, 1970 (commonly known as "Unix time" or "epoch time"). This time is affected by the timezone setting.

Note that the functions in the `Time` class expect times in UTC time, so the result from this should be used carefully.


### zone()

Set the time zone offset (+/-) from UTC.
The device will remember this offset until reboot.

*NOTE*: This function does not observe daylight savings time.

```cpp
// Set time zone to Eastern USA daylight saving time
Time.zone(-4);
```

Parameters: floating point offset from UTC in hours, from -12.0 to 13.0


### setTime()

Set the system time to the given timestamp.

*NOTE*: This will override the time set by the Particle Cloud.
If the cloud connection drops, the reconnection handshake will set the time again

Also see: [`Particle.syncTime()`](#particle-synctime-)

```cpp
// Set the time to 2014-10-11 13:37:42
Time.setTime(1413034662);
```

Parameters: Unix timestamp (integer)


### timeStr()

Return string representation for the given time.
```cpp
Serial.print(Time.timeStr()); // Wed May 21 01:08:47 2014
```

Returns: String

_NB: In 0.3.4 and earlier, this function included a newline at the end of the returned string. This has been removed in 0.4.0._

### format()

Formats a time string using a configurable format.

```cpp
// EXAMPLE

time_t time = Time.now();
Time.format(time, TIME_FORMAT_DEFAULT); // Sat Jan 10 08:22:04 2004 , same as Time.timeStr()

Time.zone(-5.25);  // setup a time zone, which is part of the ISO6801 format
Time.format(time, TIME_FORMAT_ISO8601_FULL); // 2004-01-10T08:22:04-05:15

```

The formats available are:

- `TIME_FORMAT_DEFAULT`
- `TIME_FORMAT_ISO8601_FULL`
- custom format based on `strftime()`

{{#if core}}
Note that the format function is implemented using `strftime()` which adds several kilobytes to the size of firmware.
Application firmware that has limited space available may want to consider using simpler alternatives that consume less firmware space, such as `sprintf()`.
{{/if}}

### setFormat()

Sets the format string that is the default value used by `format()`.

```cpp

Time.setFormat(TIME_FORMAT_ISO8601_FULL);

```

In more advanced cases, you can set the format to a static string that follows
the same syntax as the `strftime()` function.

```
// custom formatting

Time.format(Time.now(), "Now it's %I:%M%p.");
// Now it's 03:21AM.

```

### getFormat()

Retrieves the currently configured format string for time formatting with `format()`.


### millis()

Returns the number of milliseconds since the device began running the current program. This number will overflow (go back to zero), after approximately 49 days.

`unsigned long time = millis();`

```C++
// EXAMPLE USAGE

unsigned long time;

void setup()
{
  Serial.begin(9600);
}
void loop()
{
  Serial.print("Time: ");
  time = millis();
  //prints time since program started
  Serial.println(time);
  // wait a second so as not to send massive amounts of data
  delay(1000);
}
```
**Note:**
The return value for millis is an unsigned long, errors may be generated if a programmer tries to do math with other datatypes such as ints.

### micros()

Returns the number of microseconds since the device began running the current program.

Firmware v0.4.3 and earlier:
- This number will overflow (go back to zero), after exactly 59,652,323 microseconds (0 .. 59,652,322) on the Core and after exactly 35,791,394 microseconds (0 .. 35,791,394) on the Photon and Electron.



`unsigned long time = micros();`

```C++
// EXAMPLE USAGE

unsigned long time;

void setup()
{
  Serial.begin(9600);
}
void loop()
{
  Serial.print("Time: ");
  time = micros();
  //prints time since program started
  Serial.println(time);
  // wait a second so as not to send massive amounts of data
  delay(1000);
}
```

### delay()

Pauses the program for the amount of time (in miliseconds) specified as parameter. (There are 1000 milliseconds in a second.)

```C++
// SYNTAX
delay(ms);
```

`ms` is the number of milliseconds to pause *(unsigned long)*

```C++
// EXAMPLE USAGE

int ledPin = D1;              // LED connected to digital pin D1

void setup()
{
  pinMode(ledPin, OUTPUT);    // sets the digital pin as output
}

void loop()
{
  digitalWrite(ledPin, HIGH); // sets the LED on
  delay(1000);                // waits for a second
  digitalWrite(ledPin, LOW);  // sets the LED off
  delay(1000);                // waits for a second
}
```
**NOTE:**
the parameter for millis is an unsigned long, errors may be generated if a programmer tries to do math with other datatypes such as ints.

### delayMicroseconds()

Pauses the program for the amount of time (in microseconds) specified as parameter. There are a thousand microseconds in a millisecond, and a million microseconds in a second.

```C++
// SYNTAX
delayMicroseconds(us);
```
`us` is the number of microseconds to pause *(unsigned int)*

```C++
// EXAMPLE USAGE

int outPin = D1;              // digital pin D1

void setup()
{
  pinMode(outPin, OUTPUT);    // sets the digital pin as output
}

void loop()
{
  digitalWrite(outPin, HIGH); // sets the pin on
  delayMicroseconds(50);      // pauses for 50 microseconds
  digitalWrite(outPin, LOW);  // sets the pin off
  delayMicroseconds(50);      // pauses for 50 microseconds
}
```

## Interrupts

Interrupts are a way to write code that is run when an external event occurs.
As a general rule, interrupt code should be very fast, and non-blocking. This means
performing transfers, such as I2C, Serial, TCP should not be done as part of the
interrupt handler. Rather, the interrupt handler can set a variable which instructs
the main loop that the event has occurred.

### attachInterrupt()

Specifies a function to call when an external interrupt occurs. Replaces any previous function that was attached to the interrupt.

**NOTE:**
`pinMode()` MUST be called prior to calling attachInterrupt() to set the desired mode for the interrupt pin (INPUT, INPUT_PULLUP or INPUT_PULLDOWN).

External interrupts are supported on the following pins:

- Core: D0, D1, D2, D3, D4, A0, A1, A3, A4, A5, A6, A7
- Photon: All pins with the exception of D0 and A5 (since at present Mode Button external interrupt(EXTI) line is shared with D0, A5). Also please note following are the pins for which EXTI lines are shared so only one can work at a time:
    - D1, A4
    - D2, A0, A3
    - D3, DAC
    - D4, A1

```
// SYNTAX
attachInterrupt(pin, function, mode);
attachInterrupt(pin, function, mode, priority);
attachInterrupt(pin, function, mode, priority, subpriority);
```

*Parameters:*

- `pin`: the pin number
- `function`: the function to call when the interrupt occurs; this function must take no parameters and return nothing. This function is sometimes referred to as an *interrupt service routine* (ISR).
- `mode`: defines when the interrupt should be triggered. Three constants are predefined as valid values:
    - CHANGE to trigger the interrupt whenever the pin changes value,
    - RISING to trigger when the pin goes from low to high,
    - FALLING for when the pin goes from high to low.
- `priority` (optional): the priority of this interrupt. Default priority is 13. Lower values increase the priority of the interrupt.
- `subpriority` (optional): the subpriority of this interrupt. Default subpriority is 0.

The function returns a boolaen whether the ISR was successfully attached (true) or not (false).

```C++
// EXAMPLE USAGE

void blink(void);
int ledPin = D1;
volatile int state = LOW;

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(D2, INPUT_PULLUP);
  attachInterrupt(D2, blink, CHANGE);
}

void loop()
{
  digitalWrite(ledPin, state);
}

void blink()
{
  state = !state;
}
```

You can attach a method in a C++ object as an interrupt handler.

```cpp
class Robot {
  public:
    Robot() {
      pinMode(D2, INPUT_PULLUP);
      attachInterrupt(D2, &Robot::handler, this, CHANGE);
    }
    void handler() {
      // do something on interrupt
    }
};

Robot myRobot;
// nothing else needed in setup() or loop()
```

**Using Interrupts:**
Interrupts are useful for making things happen automatically in microcontroller programs, and can help solve timing problems. Good tasks for using an interrupt may include reading a rotary encoder, or monitoring user input.

If you wanted to insure that a program always caught the pulses from a rotary encoder, so that it never misses a pulse, it would make it very tricky to write a program to do anything else, because the program would need to constantly poll the sensor lines for the encoder, in order to catch pulses when they occurred. Other sensors have a similar interface dynamic too, such as trying to read a sound sensor that is trying to catch a click, or an infrared slot sensor (photo-interrupter) trying to catch a coin drop. In all of these situations, using an interrupt can free the microcontroller to get some other work done while not missing the input.

**About Interrupt Service Routines:**
ISRs are special kinds of functions that have some unique limitations most other functions do not have. An ISR cannot have any parameters, and they shouldn't return anything.

Generally, an ISR should be as short and fast as possible. If your sketch uses multiple ISRs, only one can run at a time, other interrupts will be executed after the current one finishes in an order that depends on the priority they have. `millis()` relies on interrupts to count, so it will never increment inside an ISR. Since `delay()` requires interrupts to work, it will not work if called inside an ISR. Using `delayMicroseconds()` will work as normal.

Typically global variables are used to pass data between an ISR and the main program. To make sure variables shared between an ISR and the main program are updated correctly, declare them as `volatile`.


### detachInterrupt()

Turns off the given interrupt.

```
// SYNTAX
detachInterrupt(pin);
```

`pin` is the pin number of the interrupt to disable.


### interrupts()

Re-enables interrupts (after they've been disabled by `noInterrupts()`). Interrupts allow certain important tasks to happen in the background and are enabled by default. Some functions will not work while interrupts are disabled, and incoming communication may be ignored. Interrupts can slightly disrupt the timing of code, however, and may be disabled for particularly critical sections of code.

```C++
// EXAMPLE USAGE

void setup() {}

void loop()
{
  noInterrupts(); // disable interrupts
  //
  // put critical, time-sensitive code here
  //
  interrupts();   // enable interrupts
  //
  // other code here
  //
}
```

`interrupts()` neither accepts a parameter nor returns anything.

### noInterrupts()

Disables interrupts (you can re-enable them with `interrupts()`). Interrupts allow certain important tasks to happen in the background and are enabled by default. Some functions will not work while interrupts are disabled, and incoming communication may be ignored. Interrupts can slightly disrupt the timing of code, however, and may be disabled for particularly critical sections of code.

// SYNTAX
noInterrupts();

`noInterrupts()` neither accepts a parameter nor returns anything.

## Software Timers

_Since 0.4.7. This feature is available on the Photon, P1 and Electron out the box. On the Core, the
`freertos4core` library should be used to add FreeRTOS to the core._

Software Timers provide a way to have timed actions in your program.  FreeRTOS provides the ability to have up to 10 Software Timers at a time with a minimum resolution of 1 millisecond.  It is common to use millis() based "timers" though exact timing is not always possible (due to other program delays).  Software timers are maintained by FreeRTOS and provide a more reliable method for running timed actions using callback functions.  Please note that Software Timers are "chained" and will be serviced sequencially when several timers trigger simultaneously, thus requiring special consideration when writing callback functions.

```cpp
// EXAMPLE

void print_every_second()
{
    static int count = 0;
    Serial.println(count++);
}

Timer timer(1000, print_every_second);

void setup()
{
    Serial.begin(9600);
    timer.start();
}
```

Timers may be started, stopped, reset within a user program or an ISR.  They may also be "disposed", removing them from the (max. 10) active timer list.

The timer callback is similar to an interrupt - it shouldn't block. However, it is less restrictive than an interrupt. If the code does block, the system will not crash - the only consequence is that other software timers that should have triggered will be delayed until the blocking timer callback function returns.

// SYNTAX

`Timer timer(period, callback, one_shot)`

- `period` is the period of the timer in milliseconds  (unsigned int)
- `callback` is the callback function which gets called when the timer expires.
- `one_shot` (optional, since 0.4.9) when `true`, the timer is fired once and then stopped automatically.  The default is `false` - a repeating timer.


### Class member callbacks

_Since 0.4.9_

A class member function can be used as a callback using this syntax to create the timer:

`Timer timer(period, callback, instance, one_shot)`

- `period` is the period of the timer in milliseconds  (unsigned int)
- `callback` is the class member function which gets called when the timer expires.
- `instance` the instance of the class to call the callback function on.
- `one_shot` (optional, since 0.4.9) when `true`, the timer is fired once and then stopped automatically.  The default is `false` - a repeating timer.


```
// Class member function callback example

class CallbackClass
{
public:
     void onTimeout();
}

CallbackClass callback;
Timer t(1000, &CallbackClass::onTimeout, callback);

```


### start()

Starts a stopped timer (a newly created timer is stopped). If `start()` is called for a running timer, it will be reset.

`start()`

```C++
// EXAMPLE USAGE
timer.start(); // starts timer if stopped or resets it if started.

```

### stop()

Stops a running timer.

`stop()`

```C++
// EXAMPLE USAGE
timer.stop(); // stops a running timer.

```

### changePeriod()

Changes the period of a previously created timer. It can be called to change the period of an running or stopped timer.

`changePeriod(newPeriod)`

`newPeriod` is the new timer period (unsigned int)

```C++
// EXAMPLE USAGE
timer.changePeriod(1000); // Reset period of timer to 1000ms.

```


### reset()

Resets a timer.  If a timer is running, it will reset to "zero".  If a timer is stopped, it will be started.

`reset()`

```C++
// EXAMPLE USAGE
timer.reset(); // reset timer if running, or start timer if stopped.

```

### startFromISR()
### stopFromISR()
### resetFromISR()
### changePeriodFromISR()

`startFromISR()`
`stopFromISR()`
`resetFromISR()`
`changePeriodFromISR()`

Start, stop and reset a timer or change a timer's period (as above) BUT from within an ISR.  These functions MUST be called when doing timer operations within an ISR.

```C++
// EXAMPLE USAGE
timer.startFromISR(); // WITHIN an ISR, starts timer if stopped or resets it if started.

timer.stopFromISR(); // WITHIN an ISR,stops a running timer.

timer.resetFromISR(); // WITHIN an ISR, reset timer if running, or start timer if stopped.

timer.changePeriodFromISR(newPeriod);  // WITHIN an ISR, change the timer period.
```

### dispose()

`dispose()`

Stop and remove a timer from the (max. 10) timer list, freeing a timer "slot" in the list.

```C++
// EXAMPLE USAGE
timer.dispose(); // stop and delete timer from timer list.

```

### isActive()

_Since 0.5.0_

`bool isActive()`

Returns `true` if the timer is in active state (pending), or `false` otherwise.

```C++
// EXAMPLE USAGE
if (timer.isActive()) {
    // ...
}
```

{{#unless core}}

## Application Watchdog

_Since 0.5.0_

The Application Watchdog is a software-implemented watchdog using a critical-priority thread that wakes up at a given timeout interval to see if the application has checked in.

If the application has not exited loop, or called Particle.process() within the given timeout, or called `ApplicationWatchdog.checkin()`, the watchdog calls the given timeout function, which is typically `System.reset`.  This could also be a user defined function that takes care of critical tasks before finally calling `System.reset`.


```cpp
// SYNTAX
// declare a global watchdog instance
ApplicationWatchdog wd(timeout_milli_seconds, timeout_function_to_call, stack_size);

// default stack_size of 512 bytes is used
ApplicationWatchdog wd(timeout_milli_seconds, timeout_function_to_call);

// EXAMPLE USAGE
// reset the system after 60 seconds if the application is unresponsive
ApplicationWatchdog wd(60000, System.reset);

void loop() {
  while (some_long_process_within_loop) {
    wd.checkin(); // resets the AWDT count
  }
}
// AWDT count reset automatically after loop() ends
```

A default `stack_size` of 512 is used for the thread. `stack_size` is an optional parameter. The stack can be made larger or smaller as needed.  This is the amount of memory needed to support the thread and function that is called.  In practice, on the Photon (v0.5.0) calling the `System.reset` function requires approx. 170 bytes of memory. If not enough memory is allocated, the application will crash due to a Stack Overflow.  The RGB LED will flash a [red SOS pattern, followed by 13 blinks](/guide/getting-started/modes/photon/#red-flash-sos).

The application watchdog requires interrupts to be active in order to function.  Enabling the hardware watchdog in combination with this is recommended, so that the system resets in the event that interrupts are not firing.

{{/unless}}

## Math

Note that in addition to functions outlined below all of the newlib math functions described at [sourceware.org](https://sourceware.org/newlib/libm.html) are also available for use by simply including the math.h header file thus:

`#include "math.h"`

### min()

Calculates the minimum of two numbers.

`min(x, y)`

`x` is the first number, any data type
`y` is the second number, any data type

The functions returns the smaller of the two numbers.

```C++
// EXAMPLE USAGE
sensVal = min(sensVal, 100); // assigns sensVal to the smaller of sensVal or 100
                             // ensuring that it never gets above 100.
```

**NOTE:**
Perhaps counter-intuitively, max() is often used to constrain the lower end of a variable's range, while min() is used to constrain the upper end of the range.

**WARNING:**
Because of the way the min() function is implemented, avoid using other functions inside the brackets, it may lead to incorrect results

```C++
min(a++, 100);   // avoid this - yields incorrect results

a++;
min(a, 100);    // use this instead - keep other math outside the function
```

### max()

Calculates the maximum of two numbers.

`max(x, y)`

`x` is the first number, any data type
`y` is the second number, any data type

The functions returns the larger of the two numbers.

```C++
// EXAMPLE USAGE
sensVal = max(senVal, 20); // assigns sensVal to the larger of sensVal or 20
                           // (effectively ensuring that it is at least 20)
```

**NOTE:**
Perhaps counter-intuitively, max() is often used to constrain the lower end of a variable's range, while min() is used to constrain the upper end of the range.

**WARNING:**
Because of the way the max() function is implemented, avoid using other functions inside the brackets, it may lead to incorrect results

```C++
max(a--, 0);   // avoid this - yields incorrect results

a--;           // use this instead -
max(a, 0);     // keep other math outside the function
```

### abs()

Computes the absolute value of a number.

`abs(x);`

where `x` is the number

The function returns `x` if `x` is greater than or equal to `0`
and returns `-x` if `x` is less than `0`.

**WARNING:**
Because of the way the abs() function is implemented, avoid using other functions inside the brackets, it may lead to incorrect results.

```C++
abs(a++);   // avoid this - yields incorrect results

a++;          // use this instead -
abs(a);       // keep other math outside the function
```

### constrain()

Constrains a number to be within a range.

`constrain(x, a, b);`

`x` is the number to constrain, all data types
`a` is the lower end of the range, all data types
`b` is the upper end of the range, all data types

The function will return:
`x`: if x is between `a` and `b`
`a`: if `x` is less than `a`
`b`: if `x` is greater than `b`

```C++
// EXAMPLE USAGE
sensVal = constrain(sensVal, 10, 150);
// limits range of sensor values to between 10 and 150
```

### map()

```C++
// EXAMPLE USAGE

// Map an analog value to 8 bits (0 to 255)
void setup() {
  pinMode(D1, OUTPUT);
}

void loop()
{
  int val = analogRead(A0);
  val = map(val, 0, 4095, 0, 255);
  analogWrite(D1, val);
}
```

Re-maps a number from one range to another. That is, a value of fromLow would get mapped to `toLow`, a `value` of `fromHigh` to `toHigh`, values in-between to values in-between, etc.

`map(value, fromLow, fromHigh, toLow, toHigh);`

Does not constrain values to within the range, because out-of-range values are sometimes intended and useful. The `constrain()` function may be used either before or after this function, if limits to the ranges are desired.

Note that the "lower bounds" of either range may be larger or smaller than the "upper bounds" so the `map()` function may be used to reverse a range of numbers, for example

`y = map(x, 1, 50, 50, 1);`

The function also handles negative numbers well, so that this example

`y = map(x, 1, 50, 50, -100);`

is also valid and works well.

The `map()` function uses integer math so will not generate fractions, when the math might indicate that it should do so. Fractional remainders are truncated, and are not rounded or averaged.

*Parameters:*

- `value`: the number to map
- `fromLow`: the lower bound of the value's current range
- `fromHigh`: the upper bound of the value's current range
- `toLow`: the lower bound of the value's target range
- `toHigh`: the upper bound of the value's target range

The function returns the mapped value

*Appendix:*
For the mathematically inclined, here's the whole function

```C++
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
```

### pow()

Calculates the value of a number raised to a power. `pow()` can be used to raise a number to a fractional power. This is useful for generating exponential mapping of values or curves.

`pow(base, exponent);`

`base` is the number *(float)*
`exponent` is the power to which the base is raised *(float)*

The function returns the result of the exponentiation *(double)*

EXAMPLE **TBD**

### sqrt()

Calculates the square root of a number.

`sqrt(x)`

`x` is the number, any data type

The function returns the number's square root *(double)*

## Random Numbers

The firmware incorporates a pseudo-random number generator.

### random()

Retrieves the next random value, restricted to a given range.

 `random(max);`

Parameters

- `max` - the upper limit of the random number to retrieve.

Returns: a random value between 0 and up to, but not including `max`.

```c++
int r = random(10);
// r is >= 0 and < 10
// The smallest value returned is 0
// The largest value returned is 9
```

 NB: When `max` is 0, the result is always 0.

---

`random(min,max);`

Parameters:

 - `min` - the lower limit (inclusive) of the random number to retrieve.
 - `max` - the upper limit (exclusive) of the random number to retrieve.

Returns: a random value from `min` and up to, but not including `max`.


```c++
int r = random(10, 100);
// r is >= 10 and < 100
// The smallest value returned is 10
// The largest value returned is 99
```

  NB: If `min` is greater or equal to `max`, the result is always 0.

### randomSeed()

`randomSeed(newSeed);`

Parameters:

 - `newSeed` - the new random seed

The pseudorandom numbers produced by the firmware are derived from a single value - the random seed.
The value of this seed fully determines the sequence of random numbers produced by successive
calls to `random()`. Using the same seed on two separate runs will produce
the same sequence of random numbers, and in contrast, using different seeds
will produce a different sequence of random numbers.

On startup, the default random seed is [set by the system](http://www.cplusplus.com/reference/cstdlib/srand/) to 1.
Unless the seed is modified, the same sequence of random numbers would be produced each time
the system starts.

Fortunately, when the device connects to the cloud, it receives a very randomized seed value,
which is used as the random seed. So you can be sure the random numbers produced
will be different each time your program is run.


*** Disable random seed from the cloud ***

When the device receives a new random seed from the cloud, it's passed to this function:

```
void random_seed_from_cloud(unsigned int seed);
```

The system implementation of this function calls `randomSeed()` to set
the new seed value. If you don't wish to use random seed values from the cloud,
you can take control of the random seeds set by adding this code to your app:

```cpp
void random_seed_from_cloud(unsigned int seed) {
   // don't do anything with this. Continue with existing seed.
}
```

In the example, the seed is simply ignored, so the system will continue using
whatever seed was previously set. In this case, the random seed will not be set
from the cloud, and setting the seed is left to up you.


## EEPROM

EEPROM emulation allocates a region of the device's built-in Flash memory to act as EEPROM.
Unlike "true" EEPROM, flash doesn't suffer from write "wear" with each write to
each individual address. Instead, the page suffers wear when it is filled. Each write
will add more data to the page until it is full, causing a page erase.

The EEPROM functions can be used to store small amounts of data in Flash that
will persist even after the device resets after a deep sleep or is powered off.

### length()
Returns the total number of bytes available in the emulated EEPROM.

```c++
// SYNTAX
size_t length = EEPROM.length();
```

- The Core has 127 bytes of emulated EEPROM.
- The Photon and Electron have 2047 bytes of emulated EEPROM.

### put()
This function will write an object to the EEPROM. You can write single values like `int` and
`float` or group multiple values together using `struct` to ensure that all values of the struct are
updated together.

```
// SYNTAX
EEPROM.put(int address, object)
```

`address` is the start address (int) of the EERPOM locations to write. It must be a value between 0
and `EEPROM.length()-1`

`object` is the object data to write. The number of bytes to write is automatically determined from
the type of object.

```C++
// EXAMPLE USAGE
// Write a value (2 bytes in this case) to the EEPROM address
int addr = 10;
uint16_t value = 12345;
EEPROM.put(addr, value);

// Write an object to the EEPROM address
addr = 20;
struct MyObject {
  uint8_t version;
  float field1;
  uint16_t field2;
  char name[10];
};
MyObject myObj = { 0, 12.34f, 25, "Test!" };
EEPROM.put(addr, myObj);
```

The object data is first compared to the data written in the EEPROM to avoid writing values that
haven't changed.

If the {{device}} loses power before the write finishes, the partially written data will be ignored.

If you write several objects to EEPROM, make sure they don't overlap: the address of the second
object must be larger than the address of the first object plus the size of the first object. You
can leave empty room between objects in case you need to make the first object bigger later.

### get()
This function will retrieve an object from the EEPROM. Use the same type of object you used in the
`put` call.

```
// SYNTAX
EEPROM.get(int address, object)
```

`address` is the start address (int) of the EERPOM locations to read. It must be a value between 0
and `EEPROM.length()-1`

`object` is the object data that would be read. The number of bytes read is automatically determined
from the type of object.

```C++
// EXAMPLE USAGE
// Read a value (2 bytes in this case) from EEPROM addres
int addr = 10;
uint16_t value;
EEPROM.get(addr, value);
if(value == 0xFFFF) {
  // EEPROM was empty -> initialize value
  value = 25;
}

// Read an object from the EEPROM addres
addr = 20;
struct MyObject {
  uint8_t version;
  float field1;
  uint16_t field2;
  char name[10];
};
MyObject myObj;
EEPROM.get(addr, myObj);
if(myObj.version != 0) {
  // EEPROM was empty -> initialize myObj
  MyObject defaultObj = { 0, 12.34f, 25, "Test!" };
  myObj = defaultObj;
}
```

The default value of bytes in the EEPROM is 255 (hexadecimal 0xFF) so reading an object on a new
{{device}} will return an object filled with 0xFF. One trick to deal with default data is to include
a version field that you can check to see if there was valid data written in the EEPROM.

### read()
Read a single byte of data from the emulated EEPROM.

```
// SYNTAX
uint8_t value = EEPROM.read(int address);
```

`address` is the address (int) of the EERPOM location to read

```C++
// EXAMPLE USAGE

// Read the value of the second byte of EEPROM
int addr = 1;
uint8_t value = EEPROM.read(addr);
```

When reading more than 1 byte, prefer `get()` over multiple `read()` since it's faster.

### write()
Write a single byte of data to the emulated EEPROM.

```
// SYNTAX
write(int address, uint8_t value);
```

`address` is the address (int) of the EERPOM location to write to
`value` is the byte data (uint8_t) to write

```C++
// EXAMPLE USAGE

// Write a byte value to the second byte of EEPROM
int addr = 1;
uint8_t val = 0x45;
EEPROM.write(addr, val);
```

When writing more than 1 byte, prefer `put()` over multiple `write()` since it's faster and it ensures
consistent data even when power is lost while writing.

### clear()
Erase all the EEPROM so that all reads will return 255 (hexadecimal 0xFF).

```C++
// EXAMPLE USAGE
// Reset all EEPROM locations to 0xFF
EEPROM.clear();
```

Calling this function pauses processor execution (including code running in interrupts) for 800ms since
no instructions can be fetched from Flash while the Flash controller is busy erasing both EEPROM
pages.

### hasPendingErase()
### performPendingErase()

*Automatic page erase is the default behavior. This section describes optional functions the
application can call to manually control page erase for advanced use cases.*

After enough data has been written to fill the first page, the EEPROM emulation will write new data
to a second page. The first page must be erased before being written again.

Erasing a page of Flash pauses processor execution (including code running in interrupts) for 500ms since
no instructions can be fetched from Flash while the Flash controller is busy erasing the EEPROM
page. This could cause issues in applications that use EEPROM but rely on precise interrupt timing.

`hasPendingErase()` lets the application developer check if a full EEPROM page needs to be erased.
When the application determines it is safe to pause processor execution to erase EEPROM it calls
`performPendingErase()`. You can call this at boot, or when your device is idle if you expect it to
run without rebooting for a long time.

```
// EXAMPLE USAGE
void setup() {
  // Erase full EEPROM page at boot when necessary
  if(EEPROM.hasPendingErase()) {
    EEPROM.performPendingErase();
  }
}
```

To estimate how often page erases will be necessary in your application, assume that it takes
`2*EEPROM.length()` byte writes to fill a page (it will usually be more because not all bytes will always be
updated with different values).

If the application never calls `performPendingErase()` then the pending page erase will be performed
when data is written using `put()` or `write()` and both pages are full. So calling
`performPendingErase()` is optional and provided to avoid the uncertainty of a potential processor
pause any time `put()` or `write()` is called.

{{#unless core}}
## Backup RAM (SRAM)

The STM32F2xx features 4KB of backup RAM. Unlike the regular RAM memory, the backup
RAM is retained so long as power is provided to VIN or to VBAT. In particular this means that
the data in backup RAM is retained when:

- the device goes into deep sleep mode
- the device is hardware or software reset (while maintaining power)
- power is removed from VIN but retained on VBAT (which will retain both the backup RAM and the RTC)

Note that _if neither VIN or VBAT is powered then the contents of the backup RAM will be lost; for data to be
retained, the device needs a power source._  For persistent storage of data through a total power loss, please use the [EEPROM](#eeprom) library.

Power Conditions and how they relate to Backup RAM initilization and data retention:

| Power Down Method | Power Up Method | When VIN Powered | When VBAT Powered | SRAM Initialized | SRAM Retained |
| -: | :- | :-: | :-: | :-: | :-: |
| Power removed on VIN and VBAT | Power applied on VIN | - | No<sup>[1]</sup> | Yes | No |
| Power removed on VIN and VBAT | Power applied on VIN | - | Yes | Yes | No |
| Power removed on VIN | Power applied on VIN | - | Yes | No | Yes |
| System.sleep(SLEEP_MODE_DEEP) | Rising edge on WKP pin, or Hard Reset | Yes | Yes/No | No | Yes |
| System.sleep(SLEEP_MODE_DEEP,10) | RTC alarm after 10 seconds | Yes | Yes/No | No | Yes |
| System.reset() | Boot after software reset | Yes | Yes/No | No | Yes |
| Hard reset | Boot after hard reset | Yes | Yes/No | No | Yes |

<sup>[1]</sup> Note: If VBAT is floating when powering up for the first time, SRAM remains uninitialized.  When using this feature for Backup RAM, it is recommended to have VBAT connected to a 3V3 or a known good power source on system first boot.  When using this feature for Extra RAM, it is recommended to jumper VBAT to GND to ensure it always initializes on system first boot.

### Storing data in Backup RAM (SRAM)

With regular RAM, data is stored in RAM by declaring variables.

```C++
// regular variables stored in RAM
float lastTemperature;
int numberOfPresses;
int numberOfTriesRemaining = 10;
```

This tells the system to store these values in RAM so they can be changed. The
system takes care of giving them initial values. Before they are set,
they will have the initial value 0 if an initial value isn't specified.

Variables stored in backup RAM follow a similar scheme but use an additional keyword `retained`:

```C++
// retained variables stored in backup RAM
retained float lastTemperature;
retained int numberOfPresses;
retained int numberOfTriesRemaining = 10;
```

A `retained` variable is similar to a regular variable, with some key differences:

- it is stored in backup RAM - no space is used in regular RAM
- instead of being initialized on each program start, `retained` variables are initialized
when the device is first powered on (with VIN, from being powered off with VIN and VBAT completely removed).
When the device is powered on, the system takes care of setting these variables to their initial values.
`lastTemperature` and `numberOfPresses` would be initialized to 0, while `numberOfTriesRemaining` would be initialized to 10.
- the last value set on the variable is retained *as long as the device is powered from VIN or VBAT and is not hard reset*.

`retained` variables can be updated freely just as with regular RAM variables and operate
just as fast as regular RAM variables.

Here's some typical use cases for `retained` variables:

- storing data for use after waking up from deep sleep
- storing data for use after power is removed on VIN, while power is still applied to VBAT (with coin cell battery or super capacitor)
- storing data for use after a hardware or software reset

Finally, if you don't need the persistence of `retained` variables, you
can consider them simply as 4KB of extra RAM to use.

```C++
// EXAMPLE USAGE
STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

retained int value = 10;

void setup() {
    Serial.begin(9600);
}

void loop() {
    Serial.println(value);
    value = 20;
    Serial.println(value);
    delay(100); // Give the serial TX buffer a chance to empty
    System.sleep(SLEEP_MODE_DEEP, 10);
    // Or try a software reset
    // System.reset();
}

/* OUTPUT
 *
 * 10
 * 20
 * DEEP SLEEP for 10 seconds
 * 20 (value is retained as 20)
 * 20
 *
 */
```

### Enabling Backup RAM (SRAM)

Backup RAM is disabled by default, since it does require some maintenance power
which may not be desired on some low-powered projects.  Backup RAM consumes roughly
5uA or less on VIN and 9uA or less on VBAT.

Backup RAM is enabled with this code (to be placed at the top of your application outside of any functions):

```cpp

STARTUP(System.enableFeature(FEATURE_RETAINED_MEMORY));

```

### Making changes to the layout or types of retained variables

When adding new `retained` variables to an existing set of `retained` variables,
it's a good idea to add them after the existing variables. this ensures the
existing retained data is still valid even with the new code.

For example, if we wanted to add a new variable `char name[50]` we should add this after
the existing `retained` variables:

```
retained float lastTemperature;
retained int numberOfPresses;
retained int numberOfTriesRemaining = 10;
retained char name[50];
```

If instead we added `name` to the beginning or middle of the block of variables,
the program would end up reading the stored values of the wrong variables.  This is
because the new code would be expecting to find the variables in a different memory location.

Similarly, you should avoid changing the type of your variables as this will also
alter the memory size and location of data in memory.

This caveat is particularly important when updating firmware without power-cycling
the device, which uses a software reset to reboot the device.  This will allow previously
`retained` variables to persist.

During development, a good suggestion to avoid confusion is to design your application to work
correctly when power is being applied for the first time, and all `retained` variables are
initialized.  If you must rearrange variables, simply power down the device (VIN and VBAT)
after changes are made to allow reinitialization of `retained` variables on the next power
up of the device.

It's perfectly fine to mix regular and `retained` variables, but for clarity we recommend
keeping the `retained` variables in their own separate block. In this way it's easier to recognize
when new `retained` variables are added to the end of the list, or when they are rearranged.


{{/unless}}

## Macros

### STARTUP()

_Since 0.4.5_

Typically an application will have its initialization code in the `setup()` function.
This works well if a delay of a few seconds from power on/reset is acceptable.

In other cases, the application wants to have code run as early as possible, before the cloud or network connection
are initialized. The `STARTUP()` function instructs the system to execute the code early on in startup.

```cpp
void setup_the_fundulating_conbobulator()
{
   pinMode(D3, OUTPUT);
   digitalWrite(D3, HIGH);
}

// The STARTUP call is placed outside of any other function
// What goes inside is any valid code that can be executed. Here, we use a function call.
// Using a single function is preferable to having several `STARTUP()` calls.
STARTUP( setup_the_fundulating_conbobulator() );

```

The code referenced by `STARTUP()` is executed very early in the startup sequence, so it's best suited
to initializing digital I/O and peripherals. Networking setup code should still be placed in `setup()`.
{{#if photon}}
Although there is one notable exception - `WiFi.selectAntenna()` should be called from `STARTUP()` to select the default antenna before the Wi-Fi connection is made.
{{/if}}

_Note that when startup code performs digital I/O, there will still be a period of at least few hundred milliseconds
where the I/O pins are in their default power-on state, namely `INPUT`. Circuits should be designed with this
in mind, using pullup/pulldown resistors as appropriate._

## System Events

*Since 0.4.9*

### System Events Overview

System events are messages sent by the system and received by application code. They inform the application about changes in the system, such as when the system has entered setup mode, or when an Over-the-Air (OTA) update starts, or when the system is about to reset.

System events are recieved by the application by registering a handler. The handler has this general format:

```
void handler(system_event_t event, int data, void* moredata);
```

Unused parameters can be removed from right to left, giving these additional function signatures:

```
void handler(system_event_t event, int data);
void handler(system_event_t event);
void handler();
```

Here's an example of an application that listens for `reset` events so that the application is notified the device is about to reset. The application publishes a reset message to the cloud and turns off connected equipment before returning from the handler, allowing the device to reset.

```
void reset_handler()
{
	// turn off the crankenspitzen
	digitalWrite(D6, LOW);
	// tell the world what we are doing
	Particle.publish("reset", "going down for reboot NOW!");
}

void setup()
{
	// register the reset handler
	System.on(reset, reset_handler);
}
```

Some event types provide additional information. For example the `button_click` event provides a parameter with the number of button clicks:

```
void button_clicked(system_event_t event, int param)
{
	int times = system_button_clicks(param);
	Serial.printlnf("button was clicked %d times", times);
}
```

#### Registering multiple events with the same handler

It's possible to subscribe to multiple events with the same handler in cases where you want the same handler to be notified for all the events. For example:

```
void handle_all_the_events(system_event_t event, int param)
{
	Serial.printlnf("got event %d with value %d");
}

void setup()
{
	// listen for Wi-Fi Listen events and Firmware Update events
	System.on(wifi_listen+firmware_update, handle_all_the_events);
}
```

To subscribe to all events, there is the placeholder `all_events`:

```
void setup()
{
	// listen for network events and firmware update events
	System.on(all_events, handle_all_the_events);
}
```

### System Events Reference

These are the system events produced by the system, their numeric value (what you will see when printing the system event to Serial) and details of how to handle the parameter value.

| Event Name | ID | Description | Parameter |
|------------|----------|-----------|
| setup_begin | 2 | signals the device has entered setup mode |  not used |
| setup_update | 4 | periodic event signalling the device is still in setup mode. | milliseconds since setup mode was started |
| setup_end | 8 | signals setup mode was exited | time in ms since setup mode was started |
| network_credentials | 16 | network credentials were changed | `network_credentials_added` or `network_credentials_cleared` |
 | button_status | 128 | button pressed or releasesed | the duration in ms the button was pressed: 0 when pressed, >0 on release. |
 | firmware_update | 256 | firmwarwe update status | one of `firmware_update_begin`, `firmware_update_progress`, `firmware_update_complete`, `firmware_update_failed` |
 | firmware_update_pending | 512 | notifies the application that a firmware update is available. This event is sent even when updates are disabled, giving the application chance to re-enable firmware updates with `System.enableUpdates()` | not used |
 | reset_pending | 1024 | notifies the application that the system would like to reset. This event is sent even when resets are disabled, giving the application chance to re-enable resets with `System.enableReset()` | not used |
 | reset | 2048 | notifies that the system will reset once the application has completed handling this event | not used |
 | button_click | 4096 | event sent each time setup button is clicked. | `int clicks = system_button_clicks(param); ` retrieves the number of clicks so far. |
| button_final_click | 8192 | sent after a run of one or more clicks not followed by additional clicks. Unlike the `button_click` event, the `button_final_click` event is sent once, at the end of a series of clicks. | `int clicks = system_button_clicks(param); ` retrieves the number of times the button was pushed. |


## System Modes

System modes help you control how the device manages the connection with the cloud.

By default, the device connects to the Cloud and processes messages automatically. However there are many cases where a user will want to take control over that connection. There are three available system modes: `AUTOMATIC`, `SEMI_AUTOMATIC`, and `MANUAL`. These modes describe how connectivity is handled.
These system modes describe how connectivity is handled and when user code is run.

System modes must be called before the setup() function. By default, the device is always in `AUTOMATIC` mode.

### Automatic mode

The automatic mode of connectivity provides the default behavior of the device, which is that:

```cpp
SYSTEM_MODE(AUTOMATIC);

void setup() {
  // This won't be called until the device is connected to the cloud
}

void loop() {
  // Neither will this
}
```

- When the device starts up, it automatically tries to connect to Wi-Fi and the Particle Cloud.
- Once a connection with the Particle Cloud has been established, the user code starts running.
- Messages to and from the Cloud are handled in between runs of the user loop; the user loop automatically alternates with [`Particle.process()`](#particle-process-).
- `Particle.process()` is also called during any delay() of at least 1 second.
- If the user loop blocks for more than about 20 seconds, the connection to the Cloud will be lost. To prevent this from happening, the user can call `Particle.process()` manually.
- If the connection to the Cloud is ever lost, the device will automatically attempt to reconnect. This re-connection will block from a few milliseconds up to 8 seconds.
- `SYSTEM_MODE(AUTOMATIC)` does not need to be called, because it is the default state; however the user can invoke this method to make the mode explicit.

In automatic mode, the user can still call `Particle.disconnect()` to disconnect from the Cloud, but is then responsible for re-connecting to the Cloud by calling `Particle.connect()`.

### Semi-automatic mode


The semi-automatic mode will not attempt to connect the device to the Cloud automatically. However once the device is connected to the Cloud (through some user intervention), messages will be processed automatically, as in the automatic mode above.

```cpp
SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
  // This is called immediately
}

void loop() {
  if (buttonIsPressed()) {
    Particle.connect();
  } else {
    doOfflineStuff();
  }
}
```

The semi-automatic mode is therefore much like the automatic mode, except:

- When the device boots up, `setup()` and `loop()` will begin running immediately.
- Once the user calls [`Particle.connect()`](#particle-connect-), the user code will be blocked between calls of `loop()` while the device attempts to negotiate a connection. This connection will block execution of `loop()` until either the device connects to the Cloud or an interrupt is fired that calls [`Particle.disconnect()`](#particle-disconnect-).

To block inside `setup()` or `loop()` during the connection attempt, use `waitFor`.

`Particle.connect();

waitFor(Particle.connected, 30000);`

### Manual mode


The "manual" mode puts the device's connectivity completely in the user's control. This means that the user is responsible for both establishing a connection to the Particle Cloud and handling communications with the Cloud by calling [`Particle.process()`](#particle-process-) on a regular basis.

```cpp
SYSTEM_MODE(MANUAL);

void setup() {
  // This will run automatically
}

void loop() {
  if (buttonIsPressed()) {
    Particle.connect();
  }
  if (Particle.connected()) {
    Particle.process();
    doOtherStuff();
  }
}
```

When using manual mode:

- The user code will run immediately when the device is powered on.
- Once the user calls [`Particle.connect()`](#particle-connect-), the device will attempt to begin the connection process.
- Once the device is connected to the Cloud ([`Particle.connected()`](#particle-connected-)` == true`), the user must call `Particle.process()` regularly to handle incoming messages and keep the connection alive. The more frequently `Particle.process()` is called, the more responsive the device will be to incoming messages.
- If `Particle.process()` is called less frequently than every 20 seconds, the connection with the Cloud will die. It may take a couple of additional calls of `Particle.process()` for the device to recognize that the connection has been lost.


{{#unless core}}
## System Thread

*Since 0.4.6.*

{{#if electron}}**Please note:** The System Thread feature is in Beta - we advise only using this
in production after extensive testing.{{/if}}

The System Thread is a system configuration that helps ensure the application loop
is not interrupted by the system background processing and network management.
It does this by running the application loop and the system loop on separate threads,
so they execute in parallel rather than sequentially.

At present, System Thread is an opt-in change. To enable system threading for your application, add

```
SYSTEM_THREAD(ENABLED);
```

to the top of your application code.


### System Threading Behavior

When the system thread is enabled, application execution changes compared to
non-threaded execution:

- `setup()` is executed immediately regardless of the system mode, which means
setup typically executes before the Network or Cloud is connected.
Calls to network-related code will be impacted and may fail because the network is not up yet.
`Particle.function()`, `Particle.variable()` and `Particle.subscribe()` will function
as intended whether the cloud is connected or not.  `Particle.publish()` will return
`false` when the cloud is not available and the event will not be published.
Other network initialisation (such as those in `UDP`, `TCPServer` and `TCPClient`)
may not function yet.
See `waitUntil` below for details on waiting for the network or cloud connection.

- after `setup()` is called, `loop()` is called repeatedly, independent from the current state of the
network or cloud connection. The system does not block `loop()` waiting
for the network or cloud to be available, nor while connecting to Wi-Fi.

- System modes `SEMI_AUTOMATIC` and `MANUAL` behave identically - both of these
modes do not not start the Networking or a Cloud
connection automatically. while `AUTOMATIC` mode connects to the cloud as soon as possible.
Neither has an affect on when the application `setup()` function is run - it is run
as soon as possible, independently from the system network activities, as described above.

- `Particle.process()` and `delay()` are not needed to keep the background tasks active - they run independently. These functions have a new role in keeping the application events serviced. Application events are:
 - cloud function calls
 - cloud events
 - system events

- Cloud functions registered with `Particle.function()` and event handlers
registered with `Particle.subscribe()` continue to execute on the application
thread in between calls to `loop()`, or when `Particle.process()` or `delay()` is called.
A long running cloud function will block the application loop (since it is application code)
but not the system code, so cloud connectivity is maintained.

 - the application continues to execute during listening mode
 - the application continues to execute during OTA updates

### System Functions

With system threading enabled, the majority of the Particle API continues to run on the calling thread, as it does for non-threaded mode. For example, when a function, such as `Time.now()`, is called, it is processed entirely on the calling thread (typically the application thread when calling from `loop()`.)

There are a small number of API functions that are system functions. These functions execute on the system thread regardless of which thread they are called from.

There are two types of system functions:

- asynchronous system functions: these functions do not return a result to the caller and do not block the caller
- synchronous system functions: these functions return a result to the caller, and block the caller until the function completes

Asynchronous system functions do not block the application thread, even when the system thread is busy, so these can be used liberally without causing unexpected delays in the application.  (Exception: when more than 20 asynchronous system functions are invoked, but not yet serviced by the application thread, the application will block for 5 seconds while attempting to put the function on the system thread queue.)

Synchronous system functions always block the caller until the system has performed the requested operation. These are the synchronous system functions:

- `WiFi.hasCredentials()`, `WiFi.setCredentials()`, `WiFi.clearCredentials()`
- `Particle.function()`
- `Particle.variable()`
- `Particle.subscribe()`
- `Particle.publish()`

For example, when the system is busy connecting to Wi-Fi or establishing the cloud connection and the application calls `Particle.variable()` then the application will be blocked until the system finished connecting to the cloud (or gives up) so that it is free to service the `Particle.variable()` function call.

This presents itself typically in automatic mode and where `setup()` registers functions, variables or subscriptions. Even though the application thread is running `setup()` independently of the system thread, calling synchronous functions will cause the application to block until the system thread has finished connecting to the cloud. This can be avoided by delaying the cloud connection until after the synchronous functions have been called.

```
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

void setup()
{
	// the system thread isn't busy so these synchronous functions execute quickly
    Particle.subscribe("event", handler);
	Particle.publish("myvar", myvar);
	Particle.connect();    // <-- now connect to the cloud, which ties up the system thread
}
```

### Task Switching

The system firmware includes an RTOS (Real Time Operating System). The RTOS is responsible for switching between the application thread and the system thread, which it does automatically every millisecond. This has 2 main consequences:

- delays close to 1ms are typically much longer
- application code may be stopped at any time when the RTOS switches to the system thread

When executing timing-critical sections of code, the task switching needs to be momentarily disabled.

### SINGLE_THREADED_BLOCK()

`SINGLE_THREADED_BLOCK()` declares that the next code block is executed in single threaded mode. Task switching is disabled until the end of the block and automatically re-enabled when the block exits. Interrupts remain enabled, so the thread may be interrupted for small periods of time, such as by interrupts from peripherals.

```
// SYNTAX
SINGLE_THREADED_BLOCK() {
   // code here is executed atomically, without task switching
   // or interrupts
}
```

Here's an example:

```
void so_timing_sensitive()
{
    if (ready_to_send) {
   		SINGLE_THREADED_BLOCK() { // single threaded execution starts now
	    		digitalWrite(D0, LOW);		// timing critical GPIO
    			delayMicroseconds(1500);
    			digitalWrite(D0, HIGH);
		}
    }  // single threaded execution stops now
}
```


### ATOMIC_BLOCK()

`ATOMIC_BLOCK()` is similar to `SINGLE_THREADED_BLOCK()` in that it prevents other threads executing during a block of code. In addition, interrupts are also disabled.

WARNING: Disabling interrupts prevents normal system operation. Consequently, `ATOMIC_BLOCK()` should be used only for brief periods where atomicity is essential.

```
// SYNTAX
ATOMIC_BLOCK() {
   // code here is executed atomically, without task switching
   // or interrupts
}
```

Here's an example:

```
void so_timing_sensitive_and_no_interrupts()
{
    if (ready_to_send) {
   		ATOMIC_BLOCK() { // only this code runs from here on - no other threads or interrupts
    		digitalWrite(D0, LOW);		// timing critical GPIO
    		delayMicroseconds(1500);
    		digitalWrite(D0, HIGH);
    		}
    }  // other threads and interrupts can run from here
}
```

### Synchronizing Access to Shared System Resources

With system threading enabled, the system thread and the application thread run in parallel. When both attempt to use the same resource, such as writing a message to `Serial`, there is no guaranteed order - the message printed by the system and the message printed by the application are arbitrarily interleaved as the RTOS rapidly switches between running a small part of the system code and then the application code. This results in both messages being intermixed.

This can be avoided by acquiring exclusive access to a resource. To get exclusive access to a resource, we can use locks. A lock
ensures that only the thread owning the lock can access the resource. Any other thread that tries to use the resource via the lock will not be granted access until the first thread eventually unlocks the resource when it is done.

At present there is only one shared resource that is used by the system and the application - `Serial`. The system makes use of `Serial` during listening mode. If the application also makes use of serial during listening mode, then it should be locked before use.

```
void print_status()
{
    WITH_LOCK(Serial) {
	    Serial.print("Current status is:");
    		Serial.println(status);
	}
}
```

The primary difference compared to using Serial without a lock is the `WITH_LOCK` declaration. This does several things:

- attempts to acquire the lock for `Serial`. If the lock isn't available, the thread blocks indefinitely until it is available.

- once `Serial` has been locked, the code in the following block is executed.

- when the block has finished executing, the lock is released, allowing other threads to use the resource.

It's also possible to attempt to lock a resource, but not block when the resource isn't available.

```
TRY_LOCK(Serial) {
    // this code is only run when no other thread is using Serial
}
```

The `TRY_LOCK()` statement functions similarly to `WITH_LOCK()` but it does not block the current thread if the lock isn't available. Instead, the entire block is skipped over.


### Waiting for the system

The [waitUntil](#waituntil-) function can be used to wait for something to happen.
Typically this is waiting for something that the system is doing,
such as waiting for Wi-Fi to be ready or the cloud to be connected.


#### waitUntil()

Sometimes you want your application  to wait until the system is in a given state.

For example, you want to publish a critical event. this can be done using the `waitUntil` function:

```cpp
    // wait for the cloud to be connected
    waitUntil(Particle.connected);
    bool sent = Particle.publish("weather", "sunny");
```

This will delay the application indefinitely until the cloud is connected. To delay the application
only for a period of time, we can use `waitFor`

```cpp
    // wait for the cloud connection to be connected or timeout after 10 seconds
    if (waitFor(Particle.connected, 10000)) {
        bool sent = Particle.publish("weather", "sunny");
    }
```

`WiFi.ready` is another common event to wait for.

```cpp
    // wait until Wi-Fi is ready
    waitUntil(WiFi.ready);
```

{{/unless}}

## System Calls

### version()

_Since 0.4.7_

Determine the version of system firmware available. Returns a version string
of the format:

> MAJOR.MINOR.PATCH

Such as "0.4.7".

For example

```

void setup()
{
   Serial.printlnf("System version: %s", System.version().c_str());
   // prints
   // System version: 0.4.7
}

```

### versionNumber()

Determines the version of system firmware available. Returns the version encoded
as a number:

> 0xAABBCCDD

 - `AA` is the major release
 - `BB` is the minor release
 - `CC` is the patch number
 - `DD` is 0

Firmware 0.4.7 has a version number 0x00040700


### buttonPushed()

_Since 0.4.6_

Can be used to determine how long the System button (MODE on Core/Electron, SETUP on Photon) has been pushed.

Returns `uint16_t` as duration button has been held down in milliseconds.

```C++
// EXAMPLE USAGE
void button_handler(system_event_t event, int duration, void* )
{
    if (!duration) { // just pressed
        RGB.control(true);
        RGB.color(255, 0, 255); // MAGENTA
    }
    else { // just released
        RGB.control(false);
    }
}

void setup()
{
    System.on(button_status, button_handler);
}

void loop()
{
    // it would be nice to fire routine events while
    // the button is being pushed, rather than rely upon loop
    if (System.buttonPushed() > 1000) {
        RGB.color(255, 255, 0); // YELLOW
    }
}
```


### System Cycle Counter

_Since 0.4.6_

The system cycle counter is incremented for each instruction executed. It functions
in normal code and during interrupts. Since it operates at the clock frequency
of the device, it can be used for accurately measuring small periods of time.

```cpp
    // overview of System tick functions
    uint32_t now = System.ticks();

    // for converting an the unknown system tick frequency into microseconds
    uint32_t scale = System.ticksPerMicrosecond();

    // delay a given number of ticks.
    System.ticksDelay(10);
```

The system ticks are intended for measuring times from less than a microsecond up
to a second. For longer time periods, using [micros()](#micros-) or [millis()](#millis-) would
be more suitable.


#### ticks()

Returns the current value of the system tick count. One tick corresponds to
one cpu cycle.

```cpp
    // measure a precise time whens something start
    uint32_t ticks = System.ticks();

```

#### ticksPerMicrosecond();

Retrieves the number of ticks per microsecond for this device. This is useful
when converting between a number of ticks and time in microseconds.

```cpp

    uint32_t start = System.ticks();
    startTheFrobnicator();
    uint32_t end = System.ticks();
    uint32_t duration = (end-start)/System.ticksPerMicrosecond();

    Serial.printlnf("The frobnicator took %d microseconds to start", duration);

```

#### ticksDelay()

Pause execution a given number of ticks. This can be used to implement precise
delays.

```cpp
    // delay 10 ticks. How long this is actually depends upon the clock speed of the
    // device.
    System.ticksDelay(10);

    // to delay for 3 microseconds on any device:
    System.ticksDelay(3*System.ticksPerMicrosecond());

```

The system code has been written such that the compiler can compute the number
of ticks to delay
at compile time and inline the function calls, reducing overhead to a minimum.



### freeMemory()

*Since v0.4.4.*

Retrieves the amount of free memory in the system in bytes.

```cpp
uint32_t freemem = System.freeMemory();
Serial.print("free memory: ");
Serial.println(freemem);
```



### factoryReset()

This will perform a factory reset and do the following:

- Restore factory reset firmware from external flash (tinker)
- Erase Wi-Fi profiles
- Enter Listening mode upon completion

```cpp
System.factoryReset()
```

### dfu()

The device will enter DFU-mode to allow new user firmware to be refreshed. DFU mode is cancelled by
- flashing firmware to the device using dfu-util, specifying the `:leave` option, or
- a system reset

```cpp
System.dfu()
```

To make DFU mode permanent - so that it continues to enter DFU mode even after a reset until
new firmware is flashed, pass `true` to the `dfu()` function.

```cpp
System.dfu(true);   // persistent DFU mode - will enter DFU after a reset until firmware is flashed.
```


### deviceID()

`System.deviceID()` provides an easy way to extract the device ID of your device. It returns a [String object](#string-class) of the device ID, which is used to identify your device.

```cpp
// EXAMPLE USAGE

void setup()
{
  // Make sure your Serial Terminal app is closed before powering your device
  Serial.begin(9600);
  // Now open your Serial Terminal, and hit any key to continue!
  while(!Serial.available()) Particle.process();

  String myID = System.deviceID();
  // Prints out the device ID over Serial
  Serial.println(myID);
}

void loop() {}
```

### enterSafeMode()

_Since 0.4.6_

```C++
// SYNTAX
System.enterSafeMode();
```


Resets the device and restarts in safe mode.


### sleep() [ Sleep ]

`System.sleep()` can be used to dramatically improve the battery life of a Particle-powered project by temporarily deactivating the Wi-Fi module, which is by far the biggest power draw.

```C++
// SYNTAX
System.sleep(long seconds);
```

```C++
// EXAMPLE USAGE

// Put the Wi-Fi module in standby (low power) for 5 seconds
System.sleep(5);
// The device LED will breathe white during sleep
```

`System.sleep(long seconds)` does NOT stop the execution of application code (non-blocking call).  Application code will continue running while the Wi-Fi module is in standby mode.

`System.sleep(SLEEP_MODE_DEEP, long seconds)` can be used to put the entire device into a *deep sleep* mode.
In this particular mode, the device shuts down the network subsystem and puts the microcontroller in a stand-by mode.
When the device awakens from deep sleep, it will reset and run all user code from the beginning with no values being maintained in memory from before the deep sleep.

As such, it is recommended that deep sleep be called only after all user code has completed. The Standby mode is used to achieve the lowest power consumption.  After entering Standby mode, the SRAM and register contents are lost except for registers in the backup domain.

```C++
// SYNTAX
System.sleep(SLEEP_MODE_DEEP, long seconds);
```

```C++
// EXAMPLE USAGE

// Put the device into deep sleep for 60 seconds
System.sleep(SLEEP_MODE_DEEP,60);
// The device LED will shut off during deep sleep
```
The device will automatically *wake up* and reestablish the Wi-Fi connection after the specified number of seconds.

`System.sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode)` can be used to put the entire device into a *stop* mode with *wakeup on interrupt*. In this particular mode, the device shuts down the network and puts the microcontroller in a stop mode with configurable wakeup pin and edge triggered interrupt. When the specific interrupt arrives, the device awakens from stop mode, it will behave as if the device is reset and run all user code from the beginning with no values being maintained in memory from before the stop mode.

As such, it is recommended that stop mode be called only after all user code has completed. (Note: The Photon and Electron will not reset before going into stop mode so all the application variables are preserved after waking up from this mode. The voltage regulator is put in low-power mode. This mode achieves the lowest power consumption while retaining the contents of SRAM and registers.)

{{#if core}}
It is mandatory to update the *bootloader* (https://github.com/spark/firmware/tree/bootloader-patch-update) for proper functioning of this mode.
{{/if}}

{{#if electron}}
The Electron maintains the cellular connection for the duration of the sleep when  `SLEEP_NETWORK_STANDBY` is given as the last parameter value. On wakeup, the device is able to reconnect to the cloud much quicker, at the expense of increased power consumption.
{{/if}}

```C++
// SYNTAX
System.sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode);
{{#if electron}}
System.sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode, SLEEP_NETWORK_STANDBY);
{{/if}}
```

```C++
// EXAMPLE USAGE

// Put the device into stop mode with wakeup using RISING edge interrupt on D0 pin
System.sleep(D0,RISING);
// The device LED will shut off during sleep
```

*Parameters:*

- `wakeUpPin`: the wakeup pin number. supports external interrupts on the following pins:
    - D0, D1, D2, D3, D4, A0, A1, A3, A4, A5, A6, A7
- `edgeTriggerMode`: defines when the interrupt should be triggered. Four constants are predefined as valid values:
    - CHANGE to trigger the interrupt whenever the pin changes value,
    - RISING to trigger when the pin goes from low to high,
    - FALLING for when the pin goes from high to low.

`System.sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds)` can be used to put the entire device into a *stop* mode with *wakeup on interrupt* or *wakeup after specified seconds*. In this particular mode, the device shuts network subsystem and puts the microcontroller in a stop mode with configurable wakeup pin and edge triggered interrupt or wakeup after the specified seconds. When the specific interrupt arrives or upon reaching the configured timeout, the device awakens from stop mode. {{#if core}} On the Core, the Core is reset on entering stop mode and runs all user code from the beginning with no values being maintained in memory from before the stop mode. As such, it is recommended that stop mode be called only after all user code has completed.{{/if}} {{#unless core}}The device will not reset before going into stop mode so all the application variables are preserved after waking up from this mode. The voltage regulator is put in low-power mode. This mode achieves the lowest power consumption while retaining the contents of SRAM and registers.{{/unless}}

```C++
// SYNTAX
System.sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode, long seconds{{#if electron}},[SLEEP_NETWORK_STANDBY]{{/if}});
```

```C++
// EXAMPLE USAGE

// Put the device into stop mode with wakeup using RISING edge interrupt on D0 pin or wakeup after 60 seconds whichever comes first
System.sleep(D0,RISING,60);
// The device LED will shut off during sleep
```

{{#if core}}On the Core, it is necessary to update the *bootloader* (https://github.com/spark/firmware/tree/bootloader-patch-update) for proper functioning of this mode.{{/if}}


*Parameters:*

- `wakeUpPin`: the wakeup pin number. supports external interrupts on the following pins:
    - D0, D1, D2, D3, D4, A0, A1, A3, A4, A5, A6, A7
- `edgeTriggerMode`: defines when the interrupt should be triggered. Four constants are predefined as valid values:
    - CHANGE to trigger the interrupt whenever the pin changes value,
    - RISING to trigger when the pin goes from low to high,
    - FALLING for when the pin goes from high to low.
- `seconds`: wakeup after the specified number of seconds
{{#if electron}}
- `SLEEP_NETWORK_STANDBY`: optional - keeps the cellular modem in a standby state while the device is sleeping.. 
{{/if}}

*Power consumption:*

- Core
 - In *standard sleep mode*, the device current consumption is in the range of: **30mA to 38mA**
 - In *deep sleep mode*, the current consumption is around: **3.2 μA**
- Photon
 - Please see the [Photon datasheet](/datasheets/photon-datasheet/#recommended-operating-conditions)


_Since 0.4.5._ The state of the {{#unless electron}}Wi-Fi{{/unless}}{{#if electron}}Cellular{{/if}} and Cloud connections is restored when the system wakes up from sleep. So if the device was connected to the cloud before sleeping, then the cloud connection
is automatically resumed on waking up.
_Since 0.5.0._ In automatic modes, the `sleep()` function doesn't return until the cloud connection has been established. This means that application code can use the cloud connection as soon as  `sleep()` returns. In previous versions, it was necessary to call `Particle.process()` to have the cloud reconnected by the system in the background.  


### reset()

Resets the device, just like hitting the reset button or powering down and back up.

```C++
uint32_t lastReset = 0;

void setup() {
    lastReset = millis();
}

void loop() {
  // Reset after 5 minutes of operation
  // ==================================
  if (millis() - lastReset > 5*60000UL) {
    System.reset();
  }
}
```


## OTA Updates

Application firmware can use these functions to turn on or off OTA updates.

TODO: document system events when an update is received but not yet applied

### System.enableUpdates()

Enables OTA updates. Updates are enabled by default.

### System.disableUpdates()

Disables OTA updates. An attempt to start an OTA update will fail.

### System.updatesEnabled()

Determine if OTA updates are presently enabled or disabled.

### System.updatesPending()

Indicates if there are OTA updates pending.




## String Class

The String class allows you to use and manipulate strings of text in more complex ways than character arrays do. You can concatenate Strings, append to them, search for and replace substrings, and more. It takes more memory than a simple character array, but it is also more useful.

For reference, character arrays are referred to as strings with a small s, and instances of the String class are referred to as Strings with a capital S. Note that constant strings, specified in "double quotes" are treated as char arrays, not instances of the String class.

### String()

Constructs an instance of the String class. There are multiple versions that construct Strings from different data types (i.e. format them as sequences of characters), including:

  * a constant string of characters, in double quotes (i.e. a char array)
  * a single constant character, in single quotes
  * another instance of the String object
  * a constant integer or long integer
  * a constant integer or long integer, using a specified base
  * an integer or long integer variable
  * an integer or long integer variable, using a specified base

```C++
// SYNTAX
String(val)
String(val, base)
```

```cpp
// EXAMPLES

String stringOne = "Hello String";                     // using a constant String
String stringOne =  String('a');                       // converting a constant char into a String
String stringTwo =  String("This is a string");        // converting a constant string into a String object
String stringOne =  String(stringTwo + " with more");  // concatenating two strings
String stringOne =  String(13);                        // using a constant integer
String stringOne =  String(analogRead(0), DEC);        // using an int and a base
String stringOne =  String(45, HEX);                   // using an int and a base (hexadecimal)
String stringOne =  String(255, BIN);                  // using an int and a base (binary)
String stringOne =  String(millis(), DEC);             // using a long and a base
```
Constructing a String from a number results in a string that contains the ASCII representation of that number. The default is base ten, so

`String thisString = String(13)`
gives you the String "13". You can use other bases, however. For example,
`String thisString = String(13, HEX)`
gives you the String "D", which is the hexadecimal representation of the decimal value 13. Or if you prefer binary,
`String thisString = String(13, BIN)`
gives you the String "1101", which is the binary representation of 13.



Parameters:

  * val: a variable to format as a String - string, char, byte, int, long, unsigned int, unsigned long
  * base (optional) - the base in which to format an integral value

Returns: an instance of the String class



### charAt()

Access a particular character of the String.

```C++
// SYNTAX
string.charAt(n)
```
Parameters:

  * `string`: a variable of type String
  * `n`: the character to access

Returns: the n'th character of the String


### compareTo()

Compares two Strings, testing whether one comes before or after the other, or whether they're equal. The strings are compared character by character, using the ASCII values of the characters. That means, for example, that 'a' comes before 'b' but after 'A'. Numbers come before letters.


```C++
// SYNTAX
string.compareTo(string2)
```

Parameters:

  * string: a variable of type String
  * string2: another variable of type String

Returns:

  * a negative number: if string comes before string2
  * 0: if string equals string2
  * a positive number: if string comes after string2

### concat()

Combines, or *concatenates* two strings into one string. The second string is appended to the first, and the result is placed in the original string.

```C++
// SYNTAX
string.concat(string2)
```

Parameters:

  * string, string2: variables of type String

Returns: None

### endsWith()

Tests whether or not a String ends with the characters of another String.

```C++
// SYNTAX
string.endsWith(string2)
```

Parameters:

  * string: a variable of type String
  * string2: another variable of type String

Returns:

  * true: if string ends with the characters of string2
  * false: otherwise


### equals()

Compares two strings for equality. The comparison is case-sensitive, meaning the String "hello" is not equal to the String "HELLO".

```C++
// SYNTAX
string.equals(string2)
```
Parameters:

  * string, string2: variables of type String

Returns:

  * true: if string equals string2
  * false: otherwise

### equalsIgnoreCase()

Compares two strings for equality. The comparison is not case-sensitive, meaning the String("hello") is equal to the String("HELLO").

```C++
// SYNTAX
string.equalsIgnoreCase(string2)
```
Parameters:

  * string, string2: variables of type String

Returns:

  * true: if string equals string2 (ignoring case)
  * false: otherwise

### format()

*Since 0.4.6.*

Provides printf-style formatting for strings.

```C++

Particle.publish("startup", String::format("frobnicator started at %s", Time.timeStr().c_str()));

```


### getBytes()

Copies the string's characters to the supplied buffer.

```C++
// SYNTAX
string.getBytes(buf, len)
```
Parameters:

  * string: a variable of type String
  * buf: the buffer to copy the characters into (byte [])
  * len: the size of the buffer (unsigned int)

Returns: None

### indexOf()

Locates a character or String within another String. By default, searches from the beginning of the String, but can also start from a given index, allowing for the locating of all instances of the character or String.

```C++
// SYNTAX
string.indexOf(val)
string.indexOf(val, from)
```

Parameters:

  * string: a variable of type String
  * val: the value to search for - char or String
  * from: the index to start the search from

Returns: The index of val within the String, or -1 if not found.

### lastIndexOf()

Locates a character or String within another String. By default, searches from the end of the String, but can also work backwards from a given index, allowing for the locating of all instances of the character or String.

```C++
// SYNTAX
string.lastIndexOf(val)
string.lastIndexOf(val, from)
```

Parameters:

  * string: a variable of type String
  * val: the value to search for - char or String
  * from: the index to work backwards from

Returns: The index of val within the String, or -1 if not found.

### length()

Returns the length of the String, in characters. (Note that this doesn't include a trailing null character.)

```C++
// SYNTAX
string.length()
```

Parameters:

  * string: a variable of type String

Returns: The length of the String in characters.

### remove()

The String `remove()` function modifies a string, in place, removing chars from the provided index to the end of the string or from the provided index to index plus count.

```C++
// SYNTAX
string.remove(index)
string.remove(index,count)
```

Parameters:

  * string: the string which will be modified - a variable of type String
  * index: a variable of type unsigned int
  * count: a variable of type unsigned int

Returns: None

### replace()

The String `replace()` function allows you to replace all instances of a given character with another character. You can also use replace to replace substrings of a string with a different substring.

```C++
// SYNTAX
string.replace(substring1, substring2)
```

Parameters:

  * string: the string which will be modified - a variable of type String
  * substring1: searched for - another variable of type String (single or multi-character), char or const char (single character only)
  * substring2: replaced with - another variable of type String (signle or multi-character), char or const char (single character only)

Returns: None

### reserve()

The String reserve() function allows you to allocate a buffer in memory for manipulating strings.

```C++
// SYNTAX
string.reserve(size)
```
Parameters:

  * size: unsigned int declaring the number of bytes in memory to save for string manipulation

Returns: None

```cpp
//EXAMPLE

String myString;

void setup() {
  // initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  myString.reserve(26);
  myString = "i=";
  myString += "1234";
  myString += ", is that ok?";

  // print the String:
  Serial.println(myString);
}

void loop() {
 // nothing to do here
}
```

### setCharAt()

Sets a character of the String. Has no effect on indices outside the existing length of the String.

```C++
// SYNTAX
string.setCharAt(index, c)
```
Parameters:

  * string: a variable of type String
  * index: the index to set the character at
  * c: the character to store to the given location

Returns: None

### startsWith()

Tests whether or not a String starts with the characters of another String.

```C++
// SYNTAX
string.startsWith(string2)
```

Parameters:

  * string, string2: variable2 of type String

Returns:

  * true: if string starts with the characters of string2
  * false: otherwise


### substring()

Get a substring of a String. The starting index is inclusive (the corresponding character is included in the substring), but the optional ending index is exclusive (the corresponding character is not included in the substring). If the ending index is omitted, the substring continues to the end of the String.

```C++
// SYNTAX
string.substring(from)
string.substring(from, to)
```

Parameters:

  * string: a variable of type String
  * from: the index to start the substring at
  * to (optional): the index to end the substring before

Returns: the substring

### toCharArray()

Copies the string's characters to the supplied buffer.

```C++
// SYNTAX
string.toCharArray(buf, len)
```
Parameters:

  * string: a variable of type String
  * buf: the buffer to copy the characters into (char [])
  * len: the size of the buffer (unsigned int)

Returns: None

### toFloat()

Converts a valid String to a float. The input string should start with a digit. If the string contains non-digit characters, the function will stop performing the conversion. For example, the strings "123.45", "123", and "123fish" are converted to 123.45, 123.00, and 123.00 respectively. Note that "123.456" is approximated with 123.46. Note too that floats have only 6-7 decimal digits of precision and that longer strings might be truncated.

```C++
// SYNTAX
string.toFloat()
```

Parameters:

  * string: a variable of type String

Returns: float (If no valid conversion could be performed because the string doesn't start with a digit, a zero is returned.)

### toInt()

Converts a valid String to an integer. The input string should start with an integral number. If the string contains non-integral numbers, the function will stop performing the conversion.

```C++
// SYNTAX
string.toInt()
```

Parameters:

  * string: a variable of type String

Returns: long (If no valid conversion could be performed because the string doesn't start with a integral number, a zero is returned.)

### toLowerCase()

Get a lower-case version of a String. `toLowerCase()` modifies the string in place.

```C++
// SYNTAX
string.toLowerCase()
```

Parameters:

  * string: a variable of type String

Returns: None

### toUpperCase()

Get an upper-case version of a String. `toUpperCase()` modifies the string in place.

```C++
// SYNTAX
string.toUpperCase()
```

Parameters:

  * string: a variable of type String

Returns: None

### trim()

Get a version of the String with any leading and trailing whitespace removed.

```C++
// SYNTAX
string.trim()
```

Parameters:

  * string: a variable of type String

Returns: None


## Stream Class
Stream is the base class for character and binary based streams. It is not called directly, but invoked whenever you use a function that relies on it.  The Particle Stream Class is based on the Arduino Stream Class.

Stream defines the reading functions in Particle. When using any core functionality that uses a read() or similar method, you can safely assume it calls on the Stream class. For functions like print(), Stream inherits from the Print class.

Some of the Particle classes that rely on Stream include :
`Serial`
`Wire`
`TCPClient`
`UDP`

### setTimeout()
`setTimeout()` sets the maximum milliseconds to wait for stream data, it defaults to 1000 milliseconds.

```C++
// SYNTAX
stream.setTimeout(time);
```

Parameters:

  * stream: an instance of a class that inherits from Stream
  * time: timeout duration in milliseconds (unsigned int)

Returns: None

### find()
`find()` reads data from the stream until the target string of given length is found.

```C++
// SYNTAX
stream.find(target);		// reads data from the stream until the target string is found
stream.find(target, length);	// reads data from the stream until the target string of given length is found
```

Parameters:

  * stream : an instance of a class that inherits from Stream
  * target : pointer to the string to search for (char *)
  * length : length of target string to search for (size_t)

Returns: returns true if target string is found, false if timed out

### findUntil()
`findUntil()` reads data from the stream until the target string or terminator string is found.

```C++
// SYNTAX
stream.findUntil(target, terminal);		// reads data from the stream until the target string or terminator is found
stream.findUntil(target, terminal, length);	// reads data from the stream until the target string of given length or terminator is found
```

Parameters:

  * stream : an instance of a class that inherits from Stream
  * target : pointer to the string to search (char *)
  * terminal : pointer to the terminal string to search for (char *)
  * length : length of target string to search for (size_t)

Returns: returns true if target string or terminator string is found, false if timed out

### readBytes()
`readBytes()` read characters from a stream into a buffer. The function terminates if the determined length has been read, or it times out.

```C++
// SYNTAX
stream.readBytes(buffer, length);
```

Parameters:

  * stream : an instance of a class that inherits from Stream
  * buffer : pointer to the buffer to store the bytes in (char *)
  * length : the number of bytes to read (size_t)

Returns: returns the number of characters placed in the buffer (0 means no valid data found)

### readBytesUntil()
`readBytesUntil()` reads characters from a stream into a buffer. The function terminates if the terminator character is detected, the determined length has been read, or it times out.

```C++
// SYNTAX
stream.readBytesUntil(terminator, buffer, length);
```

Parameters:

  * stream : an instance of a class that inherits from Stream
  * terminator : the character to search for (char)
  * buffer : pointer to the buffer to store the bytes in (char *)
  * length : the number of bytes to read (size_t)

Returns: returns the number of characters placed in the buffer (0 means no valid data found)

### readString()
`readString()` reads characters from a stream into a string. The function terminates if it times out.

```C++
// SYNTAX
stream.readString();
```

Parameters:

  * stream : an instance of a class that inherits from Stream

Returns: the entire string read from stream (String)

### readStringUntil()
`readStringUntil()` reads characters from a stream into a string until a terminator character is detected. The function terminates if it times out.

```C++
// SYNTAX
stream.readStringUntil(terminator);
```

Parameters:

  * stream : an instance of a class that inherits from Stream
  * terminator : the character to search for (char)

Returns: the entire string read from stream, until the terminator character is detected

### parseInt()
`parseInt()` returns the first valid (long) integer value from the current position under the following conditions:

 - Initial characters that are not digits or a minus sign, are skipped;
 - Parsing stops when no characters have been read for a configurable time-out value, or a non-digit is read;

```C++
// SYNTAX
stream.parseInt();
stream.parseInt(skipChar);	// allows format characters (typically commas) in values to be ignored
```

Parameters:

  * stream : an instance of a class that inherits from Stream
  * skipChar : the character to ignore while parsing (char).

Returns: parsed int value (long). If no valid digits were read when the time-out occurs, 0 is returned.

### parseFloat()
`parseFloat()` as `parseInt()` but returns the first valid floating point value from the current position.

```C++
// SYNTAX
stream.parsetFloat();
stream.parsetFloat(skipChar);	// allows format characters (typically commas) in values to be ignored
```

Parameters:

  * stream : an instance of a class that inherits from Stream
  * skipChar : the character to ignore while parsing (char).

Returns: parsed float value (float). If no valid digits were read when the time-out occurs, 0 is returned.



## Language Syntax
The following documentation is based on the Arduino reference which can be found [here.](http://www.arduino.cc/en/Reference/HomePage)

### Structure

#### setup()
The setup() function is called when an application starts. Use it to initialize variables, pin modes, start using libraries, etc. The setup function will only run once, after each powerup or device reset.

```cpp
// EXAMPLE USAGE

int button = D0;
int LED = D1;
//setup initializes D0 as input and D1 as output
void setup()
{
  pinMode(button, INPUT_PULLDOWN);
  pinMode(LED, OUTPUT);
}

void loop()
{
  // ...
}
```

#### loop()
After creating a setup() function, which initializes and sets the initial values, the loop() function does precisely what its name suggests, and loops consecutively, allowing your program to change and respond. Use it to actively control the device.

```C++
// EXAMPLE USAGE

int button = D0;
int LED = D1;
//setup initializes D0 as input and D1 as output
void setup()
{
  pinMode(button, INPUT_PULLDOWN);
  pinMode(LED, OUTPUT);
}

//loops to check if button was pressed,
//if it was, then it turns ON the LED,
//else the LED remains OFF
void loop()
{
  if (digitalRead(button) == HIGH)
    digitalWrite(LED,HIGH);
  else
    digitalWrite(LED,LOW);
}
```

### Control structures

#### if

`if`, which is used in conjunction with a comparison operator, tests whether a certain condition has been reached, such as an input being above a certain number.

```C++
// SYNTAX
if (someVariable > 50)
{
  // do something here
}
```
The program tests to see if someVariable is greater than 50. If it is, the program takes a particular action. Put another way, if the statement in parentheses is true, the statements inside the brackets are run. If not, the program skips over the code.

The brackets may be omitted after an *if* statement. If this is done, the next line (defined by the semicolon) becomes the only conditional statement.

```C++
if (x > 120) digitalWrite(LEDpin, HIGH);

if (x > 120)
digitalWrite(LEDpin, HIGH);

if (x > 120){ digitalWrite(LEDpin, HIGH); }

if (x > 120)
{
  digitalWrite(LEDpin1, HIGH);
  digitalWrite(LEDpin2, HIGH);
}                                 // all are correct
```
The statements being evaluated inside the parentheses require the use of one or more operators:

#### Comparison Operators

```C++
x == y (x is equal to y)
x != y (x is not equal to y)
x <  y (x is less than y)
x >  y (x is greater than y)
x <= y (x is less than or equal to y)
x >= y (x is greater than or equal to y)
```

**WARNING:**
Beware of accidentally using the single equal sign (e.g. `if (x = 10)` ). The single equal sign is the assignment operator, and sets x to 10 (puts the value 10 into the variable x). Instead use the double equal sign (e.g. `if (x == 10)` ), which is the comparison operator, and tests whether x is equal to 10 or not. The latter statement is only true if x equals 10, but the former statement will always be true.

This is because C evaluates the statement `if (x=10)` as follows: 10 is assigned to x (remember that the single equal sign is the assignment operator), so x now contains 10. Then the 'if' conditional evaluates 10, which always evaluates to TRUE, since any non-zero number evaluates to TRUE. Consequently, `if (x = 10)` will always evaluate to TRUE, which is not the desired result when using an 'if' statement. Additionally, the variable x will be set to 10, which is also not a desired action.

`if` can also be part of a branching control structure using the `if...else`] construction.

#### if...else

*if/else* allows greater control over the flow of code than the basic *if* statement, by allowing multiple tests to be grouped together. For example, an analog input could be tested and one action taken if the input was less than 500, and another action taken if the input was 500 or greater. The code would look like this:

```C++
// SYNTAX
if (pinFiveInput < 500)
{
  // action A
}
else
{
  // action B
}
```
`else` can proceed another `if` test, so that multiple, mutually exclusive tests can be run at the same time.

Each test will proceed to the next one until a true test is encountered. When a true test is found, its associated block of code is run, and the program then skips to the line following the entire if/else construction. If no test proves to be true, the default else block is executed, if one is present, and sets the default behavior.

Note that an *else if* block may be used with or without a terminating *else* block and vice versa. An unlimited number of such else if branches is allowed.

```C++
if (pinFiveInput < 500)
{
  // do Thing A
}
else if (pinFiveInput >= 1000)
{
  // do Thing B
}
else
{
  // do Thing C
}
```

Another way to express branching, mutually exclusive tests, is with the [`switch case`](#switch-case) statement.

#### for

The `for` statement is used to repeat a block of statements enclosed in curly braces. An increment counter is usually used to increment and terminate the loop. The `for` statement is useful for any repetitive operation, and is often used in combination with arrays to operate on collections of data/pins.

There are three parts to the for loop header:

```C++
// SYNTAX
for (initialization; condition; increment)
{
  //statement(s);
}
```
The *initialization* happens first and exactly once. Each time through the loop, the *condition* is tested; if it's true, the statement block, and the *increment* is executed, then the condition is tested again. When the *condition* becomes false, the loop ends.

```C++
// EXAMPLE USAGE

// slowy make the LED glow brighter
int ledPin = D1; // LED in series with 470 ohm resistor on pin D1

void setup()
{
  // set ledPin as an output
  pinMode(ledPin,OUTPUT);
}

void loop()
{
  for (int i=0; i <= 255; i++){
    analogWrite(ledPin, i);
    delay(10);
  }
}
```
The C `for` loop is much more flexible than for loops found in some other computer languages, including BASIC. Any or all of the three header elements may be omitted, although the semicolons are required. Also the statements for initialization, condition, and increment can be any valid C statements with unrelated variables, and use any C datatypes including floats. These types of unusual for statements may provide solutions to some rare programming problems.

For example, using a multiplication in the increment line will generate a logarithmic progression:

```C++
for(int x = 2; x < 100; x = x * 1.5)
{
  Serial.print(x);
}
//Generates: 2,3,4,6,9,13,19,28,42,63,94
```
Another example, fade an LED up and down with one for loop:

```C++
// slowy make the LED glow brighter
int ledPin = D1; // LED in series with 470 ohm resistor on pin D1

void setup()
{
  // set ledPin as an output
  pinMode(ledPin,OUTPUT);
}

void loop()
{
  int x = 1;
  for (int i = 0; i > -1; i = i + x)
  {
    analogWrite(ledPin, i);
    if (i == 255) x = -1;     // switch direction at peak
    delay(10);
  }
}
```

#### switch case

Like `if` statements, `switch`...`case` controls the flow of programs by allowing programmers to specify different code that should be executed in various conditions. In particular, a switch statement compares the value of a variable to the values specified in case statements. When a case statement is found whose value matches that of the variable, the code in that case statement is run.

The `break` keyword exits the switch statement, and is typically used at the end of each case. Without a break statement, the switch statement will continue executing the following expressions ("falling-through") until a break, or the end of the switch statement is reached.

```C++
// SYNTAX
switch (var)
{
  case label:
    // statements
    break;
  case label:
    // statements
    break;
  default:
    // statements
}
```
`var` is the variable whose value to compare to the various cases
`label` is a value to compare the variable to

```C++
// EXAMPLE USAGE

switch (var)
{
  case 1:
    // do something when var equals 1
    break;
  case 2:
    // do something when var equals 2
    break;
  default:
    // if nothing else matches, do the
    // default (which is optional)
}
```

#### while

`while` loops will loop continuously, and infinitely, until the expression inside the parenthesis, () becomes false. Something must change the tested variable, or the `while` loop will never exit. This could be in your code, such as an incremented variable, or an external condition, such as testing a sensor.

```C++
// SYNTAX
while(expression)
{
  // statement(s)
}
```
`expression` is a (boolean) C statement that evaluates to true or false.

```C++
// EXAMPLE USAGE

var = 0;
while(var < 200)
{
  // do something repetitive 200 times
  var++;
}
```

#### do... while

The `do` loop works in the same manner as the `while` loop, with the exception that the condition is tested at the end of the loop, so the do loop will *always* run at least once.

```C++
// SYNTAX
do
{
  // statement block
} while (test condition);
```

```C++
// EXAMPLE USAGE

do
{
  delay(50);          // wait for sensors to stabilize
  x = readSensors();  // check the sensors

} while (x < 100);
```

#### break

`break` is used to exit from a `do`, `for`, or `while` loop, bypassing the normal loop condition. It is also used to exit from a `switch` statement.

```C++
// EXAMPLE USAGE

for (int x = 0; x < 255; x++)
{
  digitalWrite(ledPin, x);
  sens = analogRead(sensorPin);
  if (sens > threshold)
  {
    x = 0;
    break;  // exit for() loop on sensor detect
  }
  delay(50);
}
```

#### continue

The continue statement skips the rest of the current iteration of a loop (`do`, `for`, or `while`). It continues by checking the conditional expression of the loop, and proceeding with any subsequent iterations.

```C++
// EXAMPLE USAGE

for (x = 0; x < 255; x++)
{
    if (x > 40 && x < 120) continue;  // create jump in values

    digitalWrite(PWMpin, x);
    delay(50);
}
```

#### return

Terminate a function and return a value from a function to the calling function, if desired.

```C++
//EXAMPLE USAGE

// A function to compare a sensor input to a threshold
 int checkSensor()
 {
    if (analogRead(0) > 400) return 1;
    else return 0;
}
```
The return keyword is handy to test a section of code without having to "comment out" large sections of possibly buggy code.

```C++
void loop()
{
  // brilliant code idea to test here

  return;

  // the rest of a dysfunctional sketch here
  // this code will never be executed
}
```

#### goto

Transfers program flow to a labeled point in the program

```C++
// SYNTAX

label:

goto label; // sends program flow to the label

```

**TIP:**
The use of `goto` is discouraged in C programming, and some authors of C programming books claim that the `goto` statement is never necessary, but used judiciously, it can simplify certain programs. The reason that many programmers frown upon the use of `goto` is that with the unrestrained use of `goto` statements, it is easy to create a program with undefined program flow, which can never be debugged.

With that said, there are instances where a `goto` statement can come in handy, and simplify coding. One of these situations is to break out of deeply nested `for` loops, or `if` logic blocks, on a certain condition.

```C++
// EXAMPLE USAGE

for(byte r = 0; r < 255; r++) {
  for(byte g = 255; g > -1; g--) {
    for(byte b = 0; b < 255; b++) {
      if (analogRead(0) > 250) {
        goto bailout;
      }
      // more statements ...
    }
  }
}
bailout:
// Code execution jumps here from
// goto bailout; statement
```

### Further syntax

#### ; (semicolon)

Used to end a statement.

`int a = 13;`

**Tip:**
Forgetting to end a line in a semicolon will result in a compiler error. The error text may be obvious, and refer to a missing semicolon, or it may not. If an impenetrable or seemingly illogical compiler error comes up, one of the first things to check is a missing semicolon, in the immediate vicinity, preceding the line at which the compiler complained.

#### {} (curly braces)

Curly braces (also referred to as just "braces" or as "curly brackets") are a major part of the C programming language. They are used in several different constructs, outlined below, and this can sometimes be confusing for beginners.

```C++
//The main uses of curly braces

//Functions
  void myfunction(datatype argument){
    statements(s)
  }

//Loops
  while (boolean expression)
  {
     statement(s)
  }

  do
  {
     statement(s)
  } while (boolean expression);

  for (initialisation; termination condition; incrementing expr)
  {
     statement(s)
  }

//Conditional statements
  if (boolean expression)
  {
     statement(s)
  }

  else if (boolean expression)
  {
     statement(s)
  }
  else
  {
     statement(s)
  }

```

An opening curly brace "{" must always be followed by a closing curly brace "}". This is a condition that is often referred to as the braces being balanced.

Beginning programmers, and programmers coming to C from the BASIC language often find using braces confusing or daunting. After all, the same curly braces replace the RETURN statement in a subroutine (function), the ENDIF statement in a conditional and the NEXT statement in a FOR loop.

Because the use of the curly brace is so varied, it is good programming practice to type the closing brace immediately after typing the opening brace when inserting a construct which requires curly braces. Then insert some carriage returns between your braces and begin inserting statements. Your braces, and your attitude, will never become unbalanced.

Unbalanced braces can often lead to cryptic, impenetrable compiler errors that can sometimes be hard to track down in a large program. Because of their varied usages, braces are also incredibly important to the syntax of a program and moving a brace one or two lines will often dramatically affect the meaning of a program.


#### // (single line comment)
#### /\* \*/ (multi-line comment)

Comments are lines in the program that are used to inform yourself or others about the way the program works. They are ignored by the compiler, and not exported to the processor, so they don't take up any space on the device.

Comments only purpose are to help you understand (or remember) how your program works or to inform others how your program works. There are two different ways of marking a line as a comment:

```C++
// EXAMPLE USAGE

x = 5;  // This is a single line comment. Anything after the slashes is a comment
        // to the end of the line

/* this is multiline comment - use it to comment out whole blocks of code

if (gwb == 0) {   // single line comment is OK inside a multiline comment
  x = 3;          /* but not another multiline comment - this is invalid */
}
// don't forget the "closing" comment - they have to be balanced!
*/
```

**TIP:**
When experimenting with code, "commenting out" parts of your program is a convenient way to remove lines that may be buggy. This leaves the lines in the code, but turns them into comments, so the compiler just ignores them. This can be especially useful when trying to locate a problem, or when a program refuses to compile and the compiler error is cryptic or unhelpful.


#### #define

`#define` is a useful C component that allows the programmer to give a name to a constant value before the program is compiled. Defined constants don't take up any program memory space on the chip. The compiler will replace references to these constants with the defined value at compile time.

`#define constantName value`

Note that the # is necessary.

This can have some unwanted side effects if the constant name in a `#define` is used in some other constant or variable name. In that case the text would be replaced by the `#define` value.

```C++
// EXAMPLE USAGE

#define ledPin 3
// The compiler will replace any mention of ledPin with the value 3 at compile time.
```

In general, the `const` keyword is preferred for defining constants and should be used instead of #define.

**TIP:**
There is no semicolon after the #define statement. If you include one, the compiler will throw cryptic errors further down the page.

`#define ledPin 3;   // this is an error`

Similarly, including an equal sign after the #define statement will also generate a cryptic compiler error further down the page.

`#define ledPin = 3  // this is also an error`

#### #include

`#include` is used to include outside libraries in your application code. This gives the programmer access to a large group of standard C libraries (groups of pre-made functions), and also libraries written especially for your device.

Note that #include, similar to #define, has no semicolon terminator, and the compiler will yield cryptic error messages if you add one.

### Arithmetic operators

#### = (assignment operator)

Stores the value to the right of the equal sign in the variable to the left of the equal sign.

The single equal sign in the C programming language is called the assignment operator. It has a different meaning than in algebra class where it indicated an equation or equality. The assignment operator tells the microcontroller to evaluate whatever value or expression is on the right side of the equal sign, and store it in the variable to the left of the equal sign.

```C++
// EXAMPLE USAGE

int sensVal;                // declare an integer variable named sensVal
senVal = analogRead(A0);    // store the (digitized) input voltage at analog pin A0 in SensVal
```
**TIP:**
The variable on the left side of the assignment operator ( = sign ) needs to be able to hold the value stored in it. If it is not large enough to hold a value, the value stored in the variable will be incorrect.

Don't confuse the assignment operator `=` (single equal sign) with the comparison operator `==` (double equal signs), which evaluates whether two expressions are equal.

#### + - * / (additon subtraction multiplication division)

These operators return the sum, difference, product, or quotient (respectively) of the two operands. The operation is conducted using the data type of the operands, so, for example,`9 / 4` gives 2 since 9 and 4 are ints. This also means that the operation can overflow if the result is larger than that which can be stored in the data type (e.g. adding 1 to an int with the value 2,147,483,647 gives -2,147,483,648). If the operands are of different types, the "larger" type is used for the calculation.

If one of the numbers (operands) are of the type float or of type double, floating point math will be used for the calculation.

```C++
// EXAMPLE USAGES

y = y + 3;
x = x - 7;
i = j * 6;
r = r / 5;
```

```C++
// SYNTAX
result = value1 + value2;
result = value1 - value2;
result = value1 * value2;
result = value1 / value2;
```
`value1` and `value2` can be any variable or constant.

**TIPS:**

  - Know that integer constants default to int, so some constant calculations may overflow (e.g. 50 * 50,000,000 will yield a negative result).
  - Choose variable sizes that are large enough to hold the largest results from your calculations
  - Know at what point your variable will "roll over" and also what happens in the other direction e.g. (0 - 1) OR (0 + 2147483648)
  - For math that requires fractions, use float variables, but be aware of their drawbacks: large size, slow computation speeds
  - Use the cast operator e.g. (int)myFloat to convert one variable type to another on the fly.

#### % (modulo)

Calculates the remainder when one integer is divided by another. It is useful for keeping a variable within a particular range (e.g. the size of an array).  It is defined so that `a % b == a - ((a / b) * b)`.

`result = dividend % divisor`

`dividend` is the number to be divided and
`divisor` is the number to divide by.

`result` is the remainder

The remainder function can have unexpected behavoir when some of the opperands are negative.  If the dividend is negative, then the result will be the smallest negative equivalency class.  In other words, when `a` is negative, `(a % b) == (a mod b) - b` where (a mod b) follows the standard mathematical definition of mod.  When the divisor is negative, the result is the same as it would be if it was positive.

```C++
// EXAMPLE USAGES

x = 9 % 5;   // x now contains 4
x = 5 % 5;   // x now contains 0
x = 4 % 5;   // x now contains 4
x = 7 % 5;   // x now contains 2
x = -7 % 5;  // x now contains -2
x = 7 % -5;  // x now contains 2
x = -7 % -5; // x now contains -2
```

```C++
EXAMPLE CODE
//update one value in an array each time through a loop

int values[10];
int i = 0;

void setup() {}

void loop()
{
  values[i] = analogRead(A0);
  i = (i + 1) % 10;   // modulo operator rolls over variable
}
```

**TIP:**
The modulo operator does not work on floats.  For floats, an equivalent expression to `a % b` is `a - (b * ((int)(a / b)))`

### Boolean operators

These can be used inside the condition of an if statement.

#### && (and)

True only if both operands are true, e.g.

```C++
if (digitalRead(D2) == HIGH  && digitalRead(D3) == HIGH)
{
  // read two switches
  // ...
}
//is true only if both inputs are high.
```

#### || (or)

True if either operand is true, e.g.

```C++
if (x > 0 || y > 0)
{
  // ...
}
//is true if either x or y is greater than 0.
```

#### ! (not)

True if the operand is false, e.g.

```C++
if (!x)
{
  // ...
}
//is true if x is false (i.e. if x equals 0).
```

**WARNING:**
Make sure you don't mistake the boolean AND operator, && (double ampersand) for the bitwise AND operator & (single ampersand). They are entirely different beasts.

Similarly, do not confuse the boolean || (double pipe) operator with the bitwise OR operator | (single pipe).

The bitwise not ~ (tilde) looks much different than the boolean not ! (exclamation point or "bang" as the programmers say) but you still have to be sure which one you want where.

`if (a >= 10 && a <= 20){}   // true if a is between 10 and 20`

### Bitwise operators

#### & (bitwise and)

The bitwise AND operator in C++ is a single ampersand, &, used between two other integer expressions. Bitwise AND operates on each bit position of the surrounding expressions independently, according to this rule: if both input bits are 1, the resulting output is 1, otherwise the output is 0. Another way of expressing this is:

```
    0  0  1  1    operand1
    0  1  0  1    operand2
    ----------
    0  0  0  1    (operand1 & operand2) - returned result
```

```C++
// EXAMPLE USAGE

int a =  92;    // in binary: 0000000001011100
int b = 101;    // in binary: 0000000001100101
int c = a & b;  // result:    0000000001000100, or 68 in decimal.
```
One of the most common uses of bitwise AND is to select a particular bit (or bits) from an integer value, often called masking.

#### | (bitwise or)

The bitwise OR operator in C++ is the vertical bar symbol, |. Like the & operator, | operates independently each bit in its two surrounding integer expressions, but what it does is different (of course). The bitwise OR of two bits is 1 if either or both of the input bits is 1, otherwise it is 0. In other words:

```
    0  0  1  1    operand1
    0  1  0  1    operand2
    ----------
    0  1  1  1    (operand1 | operand2) - returned result
```
```C++
// EXAMPLE USAGE

int a =  92;    // in binary: 0000000001011100
int b = 101;    // in binary: 0000000001100101
int c = a | b;  // result:    0000000001111101, or 125 in decimal.
```

#### ^ (bitwise xor)

There is a somewhat unusual operator in C++ called bitwise EXCLUSIVE OR, also known as bitwise XOR. (In English this is usually pronounced "eks-or".) The bitwise XOR operator is written using the caret symbol ^. This operator is very similar to the bitwise OR operator |, only it evaluates to 0 for a given bit position when both of the input bits for that position are 1:

```
    0  0  1  1    operand1
    0  1  0  1    operand2
    ----------
    0  1  1  0    (operand1 ^ operand2) - returned result
```
Another way to look at bitwise XOR is that each bit in the result is a 1 if the input bits are different, or 0 if they are the same.

```C++
// EXAMPLE USAGE

int x = 12;     // binary: 1100
int y = 10;     // binary: 1010
int z = x ^ y;  // binary: 0110, or decimal 6
```

The ^ operator is often used to toggle (i.e. change from 0 to 1, or 1 to 0) some of the bits in an integer expression. In a bitwise OR operation if there is a 1 in the mask bit, that bit is inverted; if there is a 0, the bit is not inverted and stays the same.


#### ~ (bitwise not)

The bitwise NOT operator in C++ is the tilde character ~. Unlike & and |, the bitwise NOT operator is applied to a single operand to its right. Bitwise NOT changes each bit to its opposite: 0 becomes 1, and 1 becomes 0. For example:

```
    0  1    operand1
   ----------
    1  0   ~ operand1

int a = 103;    // binary:  0000000001100111
int b = ~a;     // binary:  1111111110011000 = -104
```
You might be surprised to see a negative number like -104 as the result of this operation. This is because the highest bit in an int variable is the so-called sign bit. If the highest bit is 1, the number is interpreted as negative. This encoding of positive and negative numbers is referred to as two's complement. For more information, see the Wikipedia article on [two's complement.](http://en.wikipedia.org/wiki/Twos_complement)

As an aside, it is interesting to note that for any integer x, ~x is the same as -x-1.

At times, the sign bit in a signed integer expression can cause some unwanted surprises.

#### << (bitwise left shift), >> (bitwise right shift)

There are two bit shift operators in C++: the left shift operator << and the right shift operator >>. These operators cause the bits in the left operand to be shifted left or right by the number of positions specified by the right operand.

More on bitwise math may be found [here.](http://playground.arduino.cc/Code/BitMath)

```
variable << number_of_bits
variable >> number_of_bits
```

`variable` can be `byte`, `int`, `long`
`number_of_bits` and integer <= 32

```C++
// EXAMPLE USAGE

int a = 5;        // binary: 0000000000000101
int b = a << 3;   // binary: 0000000000101000, or 40 in decimal
int c = b >> 3;   // binary: 0000000000000101, or back to 5 like we started with
```
When you shift a value x by y bits (x << y), the leftmost y bits in x are lost, literally shifted out of existence:

```C++
int a = 5;        // binary: 0000000000000101
int b = a << 14;  // binary: 0100000000000000 - the first 1 in 101 was discarded
```
If you are certain that none of the ones in a value are being shifted into oblivion, a simple way to think of the left-shift operator is that it multiplies the left operand by 2 raised to the right operand power. For example, to generate powers of 2, the following expressions can be employed:

```
1 <<  0  ==    1
1 <<  1  ==    2
1 <<  2  ==    4
1 <<  3  ==    8
...
1 <<  8  ==  256
1 <<  9  ==  512
1 << 10  == 1024
...
```
When you shift x right by y bits (x >> y), and the highest bit in x is a 1, the behavior depends on the exact data type of x. If x is of type int, the highest bit is the sign bit, determining whether x is negative or not, as we have discussed above. In that case, the sign bit is copied into lower bits, for esoteric historical reasons:

```C++
int x = -16;     // binary: 1111111111110000
int y = x >> 3;  // binary: 1111111111111110
```
This behavior, called sign extension, is often not the behavior you want. Instead, you may wish zeros to be shifted in from the left. It turns out that the right shift rules are different for unsigned int expressions, so you can use a typecast to suppress ones being copied from the left:

```C++
int x = -16;                   // binary: 1111111111110000
int y = (unsigned int)x >> 3;  // binary: 0001111111111110
```

If you are careful to avoid sign extension, you can use the right-shift operator >> as a way to divide by powers of 2. For example:

```C++
int x = 1000;
int y = x >> 3;   // integer division of 1000 by 8, causing y = 125
```
### Compound operators

#### ++ (increment), -- (decrement)

Increment or decrement a variable

```C++
// SYNTAX
x++;  // increment x by one and returns the old value of x
++x;  // increment x by one and returns the new value of x

x-- ;   // decrement x by one and returns the old value of x
--x ;   // decrement x by one and returns the new value of x
```

where `x` is an integer or long (possibly unsigned)

```C++
// EXAMPLE USAGE

x = 2;
y = ++x;      // x now contains 3, y contains 3
y = x--;      // x contains 2 again, y still contains 3
```

#### compound arithmetic

- += (compound addition)
- -= (compound subtraction)
- *= (compound multiplication)
- /= (compound division)

Perform a mathematical operation on a variable with another constant or variable. The += (et al) operators are just a convenient shorthand for the expanded syntax.

```C++
// SYNTAX
x += y;   // equivalent to the expression x = x + y;
x -= y;   // equivalent to the expression x = x - y;
x *= y;   // equivalent to the expression x = x * y;
x /= y;   // equivalent to the expression x = x / y;
```

`x` can be any variable type
`y` can be any variable type or constant

```C++
// EXAMPLE USAGE

x = 2;
x += 4;      // x now contains 6
x -= 3;      // x now contains 3
x *= 10;     // x now contains 30
x /= 2;      // x now contains 15
```

#### &= (compound bitwise and)

The compound bitwise AND operator (&=) is often used with a variable and a constant to force particular bits in a variable to the LOW state (to 0). This is often referred to in programming guides as "clearing" or "resetting" bits.

`x &= y;   // equivalent to x = x & y;`

`x` can be a char, int or long variable
`y` can be an integer constant, char, int, or long

```
   0  0  1  1    operand1
   0  1  0  1    operand2
   ----------
   0  0  0  1    (operand1 & operand2) - returned result
```
Bits that are "bitwise ANDed" with 0 are cleared to 0 so, if myByte is a byte variable,
`myByte & B00000000 = 0;`

Bits that are "bitwise ANDed" with 1 are unchanged so,
`myByte & B11111111 = myByte;`

**Note:** because we are dealing with bits in a bitwise operator - it is convenient to use the binary formatter with constants. The numbers are still the same value in other representations, they are just not as easy to understand. Also, B00000000 is shown for clarity, but zero in any number format is zero (hmmm something philosophical there?)

Consequently - to clear (set to zero) bits 0 & 1 of a variable, while leaving the rest of the variable unchanged, use the compound bitwise AND operator (&=) with the constant B11111100

```
   1  0  1  0  1  0  1  0    variable
   1  1  1  1  1  1  0  0    mask
   ----------------------
   1  0  1  0  1  0  0  0

 variable unchanged
                     bits cleared
```
Here is the same representation with the variable's bits replaced with the symbol x

```
   x  x  x  x  x  x  x  x    variable
   1  1  1  1  1  1  0  0    mask
   ----------------------
   x  x  x  x  x  x  0  0

 variable unchanged
                     bits cleared
```

So if:
`myByte =  10101010;`
`myByte &= B1111100 == B10101000;`


#### |= (compound bitwise or)

The compound bitwise OR operator (|=) is often used with a variable and a constant to "set" (set to 1) particular bits in a variable.

```C++
// SYNTAX
x |= y;   // equivalent to x = x | y;
```
`x` can be a char, int or long variable
`y` can be an integer constant or char, int or long

```
   0  0  1  1    operand1
   0  1  0  1    operand2
   ----------
   0  1  1  1    (operand1 | operand2) - returned result
```
Bits that are "bitwise ORed" with 0 are unchanged, so if myByte is a byte variable,
`myByte | B00000000 = myByte;`

Bits that are "bitwise ORed" with 1 are set to 1 so:
`myByte | B11111111 = B11111111;`

Consequently - to set bits 0 & 1 of a variable, while leaving the rest of the variable unchanged, use the compound bitwise OR operator (|=) with the constant B00000011

```
   1  0  1  0  1  0  1  0    variable
   0  0  0  0  0  0  1  1    mask
   ----------------------
   1  0  1  0  1  0  1  1

 variable unchanged
                     bits set
```
Here is the same representation with the variables bits replaced with the symbol x
```
   x  x  x  x  x  x  x  x    variable
   0  0  0  0  0  0  1  1    mask
   ----------------------
   x  x  x  x  x  x  1  1

 variable unchanged
                     bits set
```
So if:
`myByte =  B10101010;`
`myByte |= B00000011 == B10101011;`



### Variables

#### HIGH | LOW

When reading or writing to a digital pin there are only two possible values a pin can take/be-set-to: HIGH and LOW.

`HIGH`

The meaning of `HIGH` (in reference to a pin) is somewhat different depending on whether a pin is set to an `INPUT` or `OUTPUT`. When a pin is configured as an INPUT with pinMode, and read with digitalRead, the microcontroller will report HIGH if a voltage of 3 volts or more is present at the pin.

A pin may also be configured as an `INPUT` with `pinMode`, and subsequently made `HIGH` with `digitalWrite`, this will set the internal 40K pullup resistors, which will steer the input pin to a `HIGH` reading unless it is pulled LOW by external circuitry. This is how INPUT_PULLUP works as well

When a pin is configured to `OUTPUT` with `pinMode`, and set to `HIGH` with `digitalWrite`, the pin is at 3.3 volts. In this state it can source current, e.g. light an LED that is connected through a series resistor to ground, or to another pin configured as an output, and set to `LOW.`

`LOW`

The meaning of `LOW` also has a different meaning depending on whether a pin is set to `INPUT` or `OUTPUT`. When a pin is configured as an `INPUT` with `pinMode`, and read with `digitalRead`, the microcontroller will report `LOW` if a voltage of 1.5 volts or less is present at the pin.

When a pin is configured to `OUTPUT` with `pinMode`, and set to `LOW` with digitalWrite, the pin is at 0 volts. In this state it can sink current, e.g. light an LED that is connected through a series resistor to, +3.3 volts, or to another pin configured as an output, and set to `HIGH.`

#### INPUT, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN

Digital pins can be used as INPUT, INPUT_PULLUP, INPUT_PULLDOWN or OUTPUT. Changing a pin with `pinMode()` changes the electrical behavior of the pin.

Pins Configured as `INPUT`

The device's pins configured as `INPUT` with `pinMode()` are said to be in a high-impedance state. Pins configured as `INPUT` make extremely small demands on the circuit that they are sampling, equivalent to a series resistor of 100 Megohms in front of the pin. This makes them useful for reading a sensor, but not powering an LED.

If you have your pin configured as an `INPUT`, you will want the pin to have a reference to ground, often accomplished with a pull-down resistor (a resistor going to ground).

Pins Configured as `INPUT_PULLUP` or `INPUT_PULLDOWN`

The STM32 microcontroller has internal pull-up resistors (resistors that connect to power internally) and pull-down resistors (resistors that connect to ground internally) that you can access. If you prefer to use these instead of external resistors, you can use these argument in `pinMode()`.

Pins Configured as `OUTPUT`

Pins configured as `OUTPUT` with `pinMode()` are said to be in a low-impedance state. This means that they can provide a substantial amount of current to other circuits. STM32 pins can source (provide positive current) or sink (provide negative current) up to 20 mA (milliamps) of current to other devices/circuits. This makes them useful for powering LED's but useless for reading sensors. Pins configured as outputs can also be damaged or destroyed if short circuited to either ground or 3.3 volt power rails. The amount of current provided by the pin is also not enough to power most relays or motors, and some interface circuitry will be required.

#### true | false

There are two constants used to represent truth and falsity in the Arduino language: true, and false.

`false`

`false` is the easier of the two to define. false is defined as 0 (zero).

`true`

`true` is often said to be defined as 1, which is correct, but true has a wider definition. Any integer which is non-zero is true, in a Boolean sense. So -1, 2 and -200 are all defined as true, too, in a Boolean sense.

Note that the true and false constants are typed in lowercase unlike `HIGH, LOW, INPUT, & OUTPUT.`

### Data Types

**Note:** The Core/Photon/Electron uses a 32-bit ARM based microcontroller and hence the datatype lengths are different from a standard 8-bit system (for e.g. Arduino Uno).

#### void

The `void` keyword is used only in function declarations. It indicates that the function is expected to return no information to the function from which it was called.

```cpp

//EXAMPLE
// actions are performed in the functions "setup" and "loop"
// but  no information is reported to the larger program

void setup()
{
  // ...
}

void loop()
{
  // ...
}
```

#### boolean

A `boolean` holds one of two values, `true` or `false`. (Each boolean variable occupies one byte of memory.)

```cpp
//EXAMPLE

int LEDpin = D0;       // LED on D0
int switchPin = A0;   // momentary switch on A0, other side connected to ground

boolean running = false;

void setup()
{
  pinMode(LEDpin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);
}

void loop()
{
  if (digitalRead(switchPin) == LOW)
  {  // switch is pressed - pullup keeps pin high normally
    delay(100);                        // delay to debounce switch
    running = !running;                // toggle running variable
    digitalWrite(LEDpin, running)      // indicate via LED
  }
}

```

#### char

A data type that takes up 1 byte of memory that stores a character value. Character literals are written in single quotes, like this: 'A' (for multiple characters - strings - use double quotes: "ABC").
Characters are stored as numbers however. You can see the specific encoding in the ASCII chart. This means that it is possible to do arithmetic on characters, in which the ASCII value of the character is used (e.g. 'A' + 1 has the value 66, since the ASCII value of the capital letter A is 65). See Serial.println reference for more on how characters are translated to numbers.
The char datatype is a signed type, meaning that it encodes numbers from -128 to 127. For an unsigned, one-byte (8 bit) data type, use the `byte` data type.

```cpp
//EXAMPLE

char myChar = 'A';
char myChar = 65;      // both are equivalent
```

#### unsigned char

An unsigned data type that occupies 1 byte of memory. Same as the `byte` datatype.
The unsigned char datatype encodes numbers from 0 to 255.
For consistency of Arduino programming style, the `byte` data type is to be preferred.

```cpp
//EXAMPLE

unsigned char myChar = 240;
```

#### byte

A byte stores an 8-bit unsigned number, from 0 to 255.

```cpp
//EXAMPLE

byte b = 0x11;
```

#### int

Integers are your primary data-type for number storage. On the Core/Photon/Electron, an int stores a 32-bit (4-byte) value. This yields a range of -2,147,483,648 to 2,147,483,647 (minimum value of -2^31 and a maximum value of (2^31) - 1).
int's store negative numbers with a technique called 2's complement math. The highest bit, sometimes referred to as the "sign" bit, flags the number as a negative number. The rest of the bits are inverted and 1 is added.

Other variations:

  * `int32_t` : 32 bit signed integer
  * `int16_t` : 16 bit signed integer
  * `int8_t`  : 8 bit signed integer

#### unsigned int

The Core/Photon/Electron stores a 4 byte (32-bit) value, ranging from 0 to 4,294,967,295 (2^32 - 1).
The difference between unsigned ints and (signed) ints, lies in the way the highest bit, sometimes referred to as the "sign" bit, is interpreted.

Other variations:

  * `uint32_t`  : 32 bit unsigned integer
  * `uint16_t`  : 16 bit unsigned integer
  * `uint8_t`   : 8 bit unsigned integer

#### word

`word` stores a 32-bit unsigned number, from 0 to 4,294,967,295.

#### long

Long variables are extended size variables for number storage, and store 32 bits (4 bytes), from -2,147,483,648 to 2,147,483,647.

#### unsigned long

Unsigned long variables are extended size variables for number storage, and store 32 bits (4 bytes). Unlike standard longs unsigned longs won't store negative numbers, making their range from 0 to 4,294,967,295 (2^32 - 1).

#### short

A short is a 16-bit data-type. This yields a range of -32,768 to 32,767 (minimum value of -2^15 and a maximum value of (2^15) - 1).

#### float

Datatype for floating-point numbers, a number that has a decimal point. Floating-point numbers are often used to approximate analog and continuous values because they have greater resolution than integers. Floating-point numbers can be as large as 3.4028235E+38 and as low as -3.4028235E+38. They are stored as 32 bits (4 bytes) of information.

Floating point numbers are not exact, and may yield strange results when compared. For example 6.0 / 3.0 may not equal 2.0. You should instead check that the absolute value of the difference between the numbers is less than some small number.
Floating point math is also much slower than integer math in performing calculations, so should be avoided if, for example, a loop has to run at top speed for a critical timing function. Programmers often go to some lengths to convert floating point calculations to integer math to increase speed.

#### double

Double precision floating point number. On the Core/Photon/Electron, doubles have 8-byte (64 bit) precision.

#### string - char array

A string can be made out of an array of type `char` and null-terminated.

```cpp
// EXAMPLES

char Str1[15];
char Str2[8] = {'a', 'r', 'd', 'u', 'i', 'n', 'o'};
char Str3[8] = {'a', 'r', 'd', 'u', 'i', 'n', 'o', '\0'};
char Str4[ ] = "arduino";
char Str5[8] = "arduino";
char Str6[15] = "arduino";
```

Possibilities for declaring strings:

  * Declare an array of chars without initializing it as in Str1
  * Declare an array of chars (with one extra char) and the compiler will add the required null character, as in Str2
  * Explicitly add the null character, Str3
  * Initialize with a string constant in quotation marks; the compiler will size the array to fit the string constant and a terminating null character, Str4
  * Initialize the array with an explicit size and string constant, Str5
  * Initialize the array, leaving extra space for a larger string, Str6

*Null termination:*
Generally, strings are terminated with a null character (ASCII code 0). This allows functions (like Serial.print()) to tell where the end of a string is. Otherwise, they would continue reading subsequent bytes of memory that aren't actually part of the string.
This means that your string needs to have space for one more character than the text you want it to contain. That is why Str2 and Str5 need to be eight characters, even though "arduino" is only seven - the last position is automatically filled with a null character. Str4 will be automatically sized to eight characters, one for the extra null. In Str3, we've explicitly included the null character (written '\0') ourselves.
Note that it's possible to have a string without a final null character (e.g. if you had specified the length of Str2 as seven instead of eight). This will break most functions that use strings, so you shouldn't do it intentionally. If you notice something behaving strangely (operating on characters not in the string), however, this could be the problem.

*Single quotes or double quotes?*
Strings are always defined inside double quotes ("Abc") and characters are always defined inside single quotes('A').

Wrapping long strings

```cpp
//You can wrap long strings like this:
char myString[] = "This is the first line"
" this is the second line"
" etcetera";
```

*Arrays of strings:*
It is often convenient, when working with large amounts of text, such as a project with an LCD display, to setup an array of strings. Because strings themselves are arrays, this is in actually an example of a two-dimensional array.
In the code below, the asterisk after the datatype char "char*" indicates that this is an array of "pointers". All array names are actually pointers, so this is required to make an array of arrays. Pointers are one of the more esoteric parts of C for beginners to understand, but it isn't necessary to understand pointers in detail to use them effectively here.


```cpp
//EXAMPLE

char* myStrings[] = {"This is string 1", "This is string 2",
"This is string 3", "This is string 4", "This is string 5",
"This is string 6"};

void setup(){
  Serial.begin(9600);
}

void loop(){
  for (int i = 0; i < 6; i++) {
    Serial.println(myStrings[i]);
    delay(500);
  }
}

```

#### String - object

More info can be found [here.](#string-class)

#### array

An array is a collection of variables that are accessed with an index number.

*Creating (Declaring) an Array:*
All of the methods below are valid ways to create (declare) an array.

```cpp
int myInts[6];
int myPins[] = {2, 4, 8, 3, 6};
int mySensVals[6] = {2, 4, -8, 3, 2};
char message[6] = "hello";
```

You can declare an array without initializing it as in myInts.

In myPins we declare an array without explicitly choosing a size. The compiler counts the elements and creates an array of the appropriate size.
Finally you can both initialize and size your array, as in mySensVals. Note that when declaring an array of type char, one more element than your initialization is required, to hold the required null character.

*Accessing an Array:*
Arrays are zero indexed, that is, referring to the array initialization above, the first element of the array is at index 0, hence

`mySensVals[0] == 2, mySensVals[1] == 4`, and so forth.
It also means that in an array with ten elements, index nine is the last element. Hence:
```cpp
int myArray[10] = {9,3,2,4,3,2,7,8,9,11};
//  myArray[9]    contains the value 11
//  myArray[10]   is invalid and contains random information (other memory address)
```

For this reason you should be careful in accessing arrays. Accessing past the end of an array (using an index number greater than your declared array size - 1) is reading from memory that is in use for other purposes. Reading from these locations is probably not going to do much except yield invalid data. Writing to random memory locations is definitely a bad idea and can often lead to unhappy results such as crashes or program malfunction. This can also be a difficult bug to track down.
Unlike BASIC or JAVA, the C compiler does no checking to see if array access is within legal bounds of the array size that you have declared.

*To assign a value to an array:*
`mySensVals[0] = 10;`

*To retrieve a value from an array:*
`x = mySensVals[4];`

*Arrays and FOR Loops:*
Arrays are often manipulated inside `for` loops, where the loop counter is used as the index for each array element. To print the elements of an array over the serial port, you could do something like the following code example.  Take special note to a MACRO called `arraySize()` which is used to determine the number of elements in `myPins`.  In this case, arraySize() returns 5, which causes our `for` loop to terminate after 5 iterations.  Also note that `arraySize()` will not return the correct answer if passed a pointer to an array.

```cpp
int myPins[] = {2, 4, 8, 3, 6};
for (int i = 0; i < arraySize(myPins); i++) {
  Serial.println(myPins[i]);
}
```

## Other Functions

Note that most of the functions in newlib described at [https://sourceware.org/newlib/libc.html](https://sourceware.org/newlib/libc.html) are available for use in addition to the functions outlined above.

## Preprocessor

`#pragma SPARK_NO_PREPROCESSOR`

```cpp
//Example
class ABC
{
   int abc;
};

void doSomethingWithABC(const ABC& abc)
{
}

/*
//Compiler error

/spark/compile_service/shared/workspace/6_hal_12_0/firmware-privatest.cpp:6:31: error: 'ABC' does not name a type
void doSomethingWithABC(const ABC& abc);
^
/spark/compile_service/shared/workspace/6_hal_12_0/firmware-privatest.cpp:6:36: error: ISO C++ forbids declaration of 'abc' with no type [-fpermissive]
void doSomethingWithABC(const ABC& abc);
^
make[1]: *** [../build/target/user/platform-6test.o] Error 1
make: *** [user] Error 2
*/
```

When you are using the Particle Cloud to compile your `.ino` source code, a preprocessor comes in to modify the code into C++ requirements before producing the binary file used to flash onto your devices.

However, there might be instances where the preprocessor causes issues in your code. One example is the use of class/structs in your function parameters.



So when you see the `ABC does not name a type` error, yet you know the type is defined, consider disabling the preprocessor using `#pragma SPARK_NO_PREPROCESSOR` at the top of your code.
