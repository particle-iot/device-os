# Particle Communication Library

This directory holds the firmware libraries used for the communication between Particle devices like the Photon/Electron and the Particle Cloud service.

## SECURITY OVERVIEW

The following applies to the TCP connection used by default on Wi-Fi devices. The Electron uses standard DTLS.

* [RSA encrypt initial handshake message to cloud](src/core_protocol.cpp#L101)
* [Decrypt return message from Cloud with an RSA private key on the Core](src/handshake.cpp#L56)
* [Verify HMAC signature](src/core_protocol.cpp#L1609-L1613)
* If everything checks out, AES-128-CBC session key is saved and IV is rotated with every message exchanged
  * [encrypt](src/core_protocol.cpp#L1558-L1563)
  * [decrypt](src/core_protocol.cpp#L306-L312)

## VERSION HISTORY

Latest Version: v1.1.0

- v1.1.0 - DTLS/UDP transport supported. [docs](dtls.md)



