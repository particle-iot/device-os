#pragma once
#include "hal_platform.h"
#if HAL_PLATFORM_MESH

#include "protocol_defs.h"
#include "message_channel.h"
#include "messages.h"
#include "spark_protocol_functions.h"

namespace particle
{
namespace protocol
{

class Protocol;

class Mesh
{
public:

	ProtocolError network_update(Protocol& protocol, uint8_t token, MessageChannel& channel, bool created, MeshCommand::NetworkInfo& networkInfo, completion_handler_data* c);
	ProtocolError device_joined(Protocol& protocol, uint8_t token, MessageChannel& channel, bool joined, MeshCommand::NetworkUpdate& update, completion_handler_data* c);
	ProtocolError device_gateway(Protocol& protocol, uint8_t token, MessageChannel& channel, bool active, MeshCommand::NetworkUpdate& update, completion_handler_data* c);

};


}}

#endif
