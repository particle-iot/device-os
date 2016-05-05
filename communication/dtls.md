# DTLS Notes

## Key Provisioning

### Generating a new key

openssl ecparam -name prime256v1 -genkey -out server.pem
openssl ec -in server.pem -out private.der -outform DER
openssl ec -in server.pem -out public.der -outform DER -pubout


### Padding the server public key to 192 bytes

dd if=/dev/null  of=server_key.der bs=1 count=0 seek=192

### Append a hard-coded IP Address

E.g. 127.0.0.1

First 2 bytes are 0 followed by the 4 address octets.

echo -e "\x00\x00\x7F\x00\x00\x01" >> server_key.der


# Changes for CoAP over UDP

When CoAP is sent via TCP the reliable transport means that packets are guaranteed delivered and never require retransmitting.  That's not a guarantee we get with UDP, so the provisions in the CoAP specification for dealing with
lost packets, out of order delivery and repeated delivery should be implemented:

- resending Confirmable requests to the server when the corresponding ACK hasn't been received

- re-sending responses to Confirmable requests in case the requests are duplicated/re-requested

The first part - resending unacknowledged requets, requires that each request send is maintained in a pool until it is acknowledged (or the maximum resend limit has been reached.)

The second part - resending responses - can be implemented 2 ways:
- for idempotent actions, the request can be reprocessed in full and the response sent. 
- for non-idempotent actions (or idempotent actions that are expensive to re-compute), the generated response is stored and resent.

The MessageChannel interface will be extended:
- receive() will handle retransmissions of unacknowledged messages and repeat acknowledgements 
- create() will have a flag to indicate repeat requets are made visible via receive() or handled automatically from this message instance.
- send() will detect confirmable requests, and automatically maintain these for retransmitting.
- send() will detect repeatable responses and automatically maintain these for retransmissing if re-requseted.
- process(system_tick_t now) will be added that performs housekeeping. 
 - removing cached responses that are too old
 - 

Should confirmable messages be sent synchronously? 
- send() could block and manage the entire wait for response cycle
- this ensures that messages are truly send in the order requested by the application
- application can choose to relax this, and then messages are sent in the background (e.g. transmission result is via a callback to the application)
- application can further choose to relax the confirmable message, in which case it is sent just once and not resent








