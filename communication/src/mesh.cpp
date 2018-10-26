#include "mesh.h"
#if HAL_PLATFORM_MESH
#include "messages.h"
#include "coap.h"
#include "bytes2hexbuf.h"
#include "protocol.h"

namespace particle { namespace protocol {

namespace {

const unsigned MESH_COMMAND_TIMEOUT = 3 * 60 * 1000;

} // unnamed

uint8_t* buildNetworkUpdateMessage(uint8_t token, Message& message, MessageChannel& channel, MeshCommand::NetworkUpdate& update)
{
	channel.create(message);

	uint8_t* const buf = message.buf();
	uint8_t* p = buf;
	p += CoAP::header(p, CoAPType::CON, CoAPCode::PUT, 1, &token);

	// add Uri-Path options
	p += CoAP::uri_path(p, CoAPOption::NONE, "m");
	p += CoAP::uri_path(p, CoAPOption::URI_PATH, "n");
	p += CoAP::uri_path(p, CoAPOption::URI_PATH, update.id);

	return p;
}

ProtocolError registerCompletionHandler(Protocol& protocol, ProtocolError result, Message& message, completion_handler_data* c) {
	CompletionHandler handler(c->handler_callback, c->handler_data);
	if (result == NO_ERROR) {
		if (message.has_id()) {
			protocol.add_ack_handler(message.get_id(), std::move(handler), MESH_COMMAND_TIMEOUT);
		} else {
			handler.setResult();
		}
	}
	else {
		handler.setError(toSystemError(result));
	}
	return result;
}

ProtocolError sendNetworkUpdate(uint8_t* p, Protocol& protocol, Message& message, MessageChannel& channel, completion_handler_data* c) {
	message.set_length(p-message.buf());
	ProtocolError result = channel.send(message);
	LOG(INFO, "mesh network update result %d", result);
	return registerCompletionHandler(protocol, result, message, c);
}

ProtocolError Mesh::network_update(Protocol& protocol, uint8_t token, MessageChannel& channel, bool created, MeshCommand::NetworkInfo& networkInfo, completion_handler_data* c)
{
	Message message;
	uint8_t* p = buildNetworkUpdateMessage(token, message, channel, networkInfo.update);

	// the server is expecting everything from flags onwards.
	p += CoAP::payload(p, &networkInfo.flags, sizeof(MeshCommand::NetworkInfo)-offsetof(MeshCommand::NetworkInfo,flags));
	return sendNetworkUpdate(p, protocol, message, channel, c);
}

ProtocolError Mesh::device_joined(Protocol& protocol, uint8_t token, MessageChannel& channel, bool joined, MeshCommand::NetworkUpdate& update, completion_handler_data* c)
{
	Message message;
	uint8_t* p = buildNetworkUpdateMessage(token, message, channel, update);
	p += CoAP::uri_query(p, CoAPOption::URI_PATH, joined ? "j=1" : "j=0");
	return sendNetworkUpdate(p, protocol, message, channel, c);
}

ProtocolError Mesh::device_gateway(Protocol& protocol, uint8_t token, MessageChannel& channel, bool active, MeshCommand::NetworkUpdate& update, completion_handler_data* c)
{
	Message message;
	uint8_t* p = buildNetworkUpdateMessage(token, message, channel, update);
	p += CoAP::uri_query(p, CoAPOption::URI_PATH, active ? "br=1" : "br=0");
	return sendNetworkUpdate(p, protocol, message, channel, c);
}


}}
#endif
