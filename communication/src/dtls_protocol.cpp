#include "dtls_protocol.h"
#include "eckeygen.h"

namespace particle { namespace protocol {

void DTLSProtocol::init(const char *id,
		  const SparkKeys &keys,
		  const SparkCallbacks &callbacks,
		  const SparkDescriptor &descriptor)
{
	set_protocol_flags(0);
	memcpy(device_id, id, sizeof(device_id));
	// send pings once per hour
	initialize_ping(23*60*1000,30000);
	DTLSMessageChannel::Callbacks channelCallbacks;
	memset(&channelCallbacks,0, sizeof(channelCallbacks));
	channelCallbacks.millis = callbacks.millis;
	channelCallbacks.handle_seed = handle_seed;
	channelCallbacks.receive = callbacks.receive;
	channelCallbacks.send = callbacks.send;
	if (callbacks.size>=52) {
		channelCallbacks.save = callbacks.save;
		channelCallbacks.restore = callbacks.restore;
	}

	channel.set_millis(callbacks.millis);

	uint8_t core_public[128];
	int len = extract_public_ec_key_length(core_public, sizeof(core_public), keys.core_private, determine_der_length(keys.core_private, MAX_DEVICE_PRIVATE_KEY_LENGTH));
	if (len<0)
	{
		WARN("Error extracting public key");
		return;
	}
	uint8_t* extracted_core_public = core_public + sizeof(core_public) - len;

	ProtocolError error = channel.init(keys.core_private, determine_der_length(keys.core_private, MAX_DEVICE_PRIVATE_KEY_LENGTH),
			extracted_core_public, len,
		keys.server_public, determine_der_length(keys.server_public, MAX_SERVER_PUBLIC_KEY_LENGTH),
		(const uint8_t*)id, channelCallbacks, &channel.next_id_ref());
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
