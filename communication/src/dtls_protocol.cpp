#include "dtls_protocol.h"

#if HAL_PLATFORM_CLOUD_UDP && PARTICLE_PROTOCOL

#include "eckeygen.h"

namespace particle { namespace protocol {

void DTLSProtocol::init(const char *id,
		  const SparkKeys &keys,
		  const SparkCallbacks &callbacks,
		  const SparkDescriptor &descriptor)
{
	set_protocol_flags(0);
	memcpy(device_id, id, sizeof(device_id));
	// Sets ping interval by default to 23 minutes. This is Electron-specific default
	// value, however all the other platforms will override this currently depending
	// on various conditions.
	// IMPORTANT: the second argument is the timeout waiting for the ping
	// acknowledgment. This SHOULD be set to MAX_TRANSMIT_WAIT, which is 93s
	// for default transmission parameters defined in RFC7252. If required we might lower it.
	initialize_ping(23 * 60 * 1000, CoAPMessage::MAX_TRANSMIT_WAIT);
	DTLSMessageChannel::Callbacks channelCallbacks = {0};
	channelCallbacks.millis = callbacks.millis;
	channelCallbacks.handle_seed = handle_seed;
	channelCallbacks.receive = callbacks.receive;
	channelCallbacks.send = callbacks.send;
	channelCallbacks.calculate_crc = callbacks.calculate_crc;
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
		(const uint8_t*)device_id, channelCallbacks, &channel.next_id_ref());
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


int DTLSProtocol::wait_confirmable(uint32_t timeout)
{
	system_tick_t start = millis();
	LOG(INFO, "Waiting for Confirmed messages to be sent.");
	ProtocolError err = NO_ERROR;
	// FIXME: Additionally wait for 1 second before going into sleep to give
	// a chance for some requests to arrive (e.g. application describe request)
	while ((channel.has_unacknowledged_requests() && (millis()-start)<timeout) ||
			(millis() - start) <= 1000)
	{
		CoAPMessageType::Enum message;
		err = event_loop(message);
		if (err)
		{
			LOG(WARN, "error receiving acknowledgements: %d", err);
			break;
		}
	}
	LOG(INFO, "All Confirmed messages sent: client(%s) server(%s)",
		channel.client_messages().has_messages() ? "no" : "yes",
		channel.server_messages().has_unacknowledged_requests() ? "no" : "yes");

	return (int)err;
}


}}

#endif // HAL_PLATFORM_CLOUD_UDP && PARTICLE_PROTOCOL
