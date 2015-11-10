#include "dtls_protocol.h"
#include "eckeygen.h"

namespace particle { namespace protocol {


size_t keylen(uint8_t* key, size_t max_len)
{
	size_t len;
	const uint8_t* end = key+max_len;
	uint8_t* p = key;
	if (mbedtls_asn1_get_tag( &p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) {
		return 0;
	}
	return p-key+len;
}


void DTLSProtocol::init(const char *id,
		  const SparkKeys &keys,
		  const SparkCallbacks &callbacks,
		  const SparkDescriptor &descriptor)
{
	DTLSMessageChannel::Callbacks channelCallbacks;
	channelCallbacks.millis = callbacks.millis;
	channelCallbacks.handle_seed = handle_seed;
	channelCallbacks.receive = callbacks.receive;
	channelCallbacks.send = callbacks.send;

	uint8_t core_public[128];
	int len = extract_public_ec_key(core_public, sizeof(core_public), keys.core_private, keylen(keys.core_private, MAX_DEVICE_PRIVATE_KEY_LENGTH));
	if (len<0)
	{
		WARN("Error extracting public key");
		return;
	}
	uint8_t* extracted_core_public = core_public + sizeof(core_public) - len;

	ProtocolError error = channel.init(keys.core_private, keylen(keys.core_private, MAX_DEVICE_PRIVATE_KEY_LENGTH),
			extracted_core_public, len,
		keys.server_public, keylen(keys.server_public, MAX_SERVER_PUBLIC_KEY_LENGTH),
		(const uint8_t*)id, channelCallbacks);
	if (error)
	{
		WARN("error initializing DTLS channel: %d", error);
	}
	else
	{
		INFO("channel inited");
		Protocol::init(callbacks, descriptor);
	}

}


}}
