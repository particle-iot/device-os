[![Build Status](https://travis-ci.org/spark/core-communication-lib.svg)](https://travis-ci.org/spark/core-communication-lib)

# Core Communication Library

This repository holds the firmware libraries used for the communication between the Spark Core and the Cloud service.

Follow [this link](https://github.com/spark/core-firmware/blob/master/README.md) to find out how to build and use this repository.

### SECURITY OVERVIEW

* [RSA encrypt initial handshake message to cloud](https://github.com/spark/core-communication-lib/blob/master/src/spark_protocol.cpp#L102)
* [Decrypt return message from Cloud with an RSA private key on the Core](https://github.com/spark/core-communication-lib/blob/master/src/handshake.cpp#L53)
* [Verify HMAC signature](https://github.com/spark/core-communication-lib/blob/master/src/spark_protocol.cpp#L1022)
* If everything checks out, AES-128-CBC session key is saved and IV is rotated with every message exchanged
  * [encrypt](https://github.com/spark/core-communication-lib/blob/master/src/spark_protocol.cpp#L989)
  * [decrypt](https://github.com/spark/core-communication-lib/blob/master/src/spark_protocol.cpp#L267)

### CREDITS AND ATTRIBUTIONS 

The Spark application team: Zachary Crockett, Satish Nair, Zach Supalla, David Middlecamp and Mohit Bhoite.

The core-communication-lib uses the GNU GCC toolchain for ARM Cortex-M processors and XySSL/TropicSSL libraries.

### LICENSE
Unless stated elsewhere, file headers or otherwise, the license as stated in the LICENSE file.

### CONTRIBUTE

Want to contribute to the Spark Core project? Follow [this link]() to find out how.

### CONNECT

Having problems or have awesome suggestions? Connect with us [here.](https://community.sparkdevices.com/)

### VERSION HISTORY

Latest Version: v1.0.0


