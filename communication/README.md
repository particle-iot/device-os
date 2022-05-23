# Particle Communication Library

This directory holds the firmware libraries used for the communication between Particle devices like the Argon/Boron/Tracker and the Particle Cloud service.

## SECURITY OVERVIEW

The following applies to the UDP connection used by default on Gen3+ devices.

The initial handshaking process creates an encrypted session using DTLS over UDP (datagram TLS) on Gen 3+ devices. This assures that your data cannot be monitored or tampered with in transit. The Particle cloud connection uses the CoAP (constrained application protocol) over DTLS. All features like Publish, Subscribe, Particle Functions and Variables, and OTA firmware updates occur over a single CoAP connection.

* [DTLS encrypted Initial handshake message to cloud](src/protocol.cpp#L349)
* [Decrypt return message from Cloud with an RSA private key on the Core](src/handshake.cpp#L56)
* [Verify HMAC signature](src/handshake.cpp#L86)
* If everything checks out, AES-128-CBC session key is saved and IV is rotated with every message exchanged
  * [encrypt](src/core_protocol.cpp#L1558-L1563)
  * [decrypt](src/core_protocol.cpp#L306-L312)

## VERSION HISTORY

Latest Version: v1.1.0

- v1.1.0 - DTLS/UDP transport supported. [docs](dtls.md)



