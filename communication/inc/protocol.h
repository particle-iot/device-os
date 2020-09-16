#pragma once

#include "message_channel.h"
#include "service_debug.h"
#include "protocol_defs.h"
#include "ping.h"
#include "chunked_transfer.h"
#include "firmware_update.h"
#include "spark_descriptor.h"
#include "spark_protocol_functions.h"
#include "functions.h"
#include "events.h"
#include "publisher.h"
#include "subscriptions.h"
#include "variables.h"
#include "hal_platform.h"
#include "timesyncmanager.h"

namespace particle
{
namespace protocol
{

/**
 * Tie ALL the bits together.
 */
class Protocol
{
	/**
	 * The message channel that sends and receives message packets.
	 */
	MessageChannel& channel;

	/**
	 * The tick time of the last communication with the cloud.
	 * todo - move this into the message channel?
	 */
	system_tick_t last_message_millis;

	/**
	 * The product_id represented by this device. set_product_id()
	 */
	product_id_t product_id;

	/**
	 * The product version for this device.
	 */
	product_firmware_version_t product_firmware_version;

	/**
	 * Descriptor callbacks that provide externally hosted functions and variables.
	 */
	SparkDescriptor descriptor;

	/**
	 * Functional callbacks that provide key system services to this communications layer.
	 */
	SparkCallbacks callbacks;

	/**
	 * Application-level callbacks.
	 */
	CommunicationsHandlers handlers;

#if HAL_PLATFORM_OTA_PROTOCOL_V3
	FirmwareUpdate firmwareUpdate;
#else
	/**
	 * Manages chunked file transfer functionality.
	 */
	ChunkedTransfer chunkedTransfer;
	class ChunkedTransferCallbacks : public ChunkedTransfer::Callbacks {
		SparkCallbacks* callbacks;
	public:
		void init(SparkCallbacks* callbacks) {
			this->callbacks = callbacks;
		}

		  virtual int prepare_for_firmware_update(FileTransfer::Descriptor& data, uint32_t flags, void*);

		  virtual int save_firmware_chunk(FileTransfer::Descriptor& descriptor, const unsigned char* chunk, void*);

		  virtual int finish_firmware_update(FileTransfer::Descriptor& data, uint32_t flags, void*);

		  virtual uint32_t calculate_crc(const unsigned char *buf, uint32_t buflen);

		  virtual system_tick_t millis();

	} chunkedTransferCallbacks;
#endif // !HAL_PLATFORM_OTA_PROTOCOL_V3

	/**
	 * Manages device-hosted variables.
	 */
	Variables variables;

	/**
	 * Manages device-hosted functions.
	 */
	Functions functions;

	/**
	 * Manages subscriptions from this device.
	 */
	Subscriptions subscriptions;

	/**
	 * Manages published events from this device.
	 */
	Publisher publisher;

	/**
	 * Manages time sync requests
	 */
	TimeSyncManager timesync_;

	Vector<message_handle_t> subscription_msg_ids;
	message_handle_t app_describe_msg_id;
	message_handle_t system_describe_msg_id;

	/**
	 * Completion handlers for messages with confirmable delivery.
	 */
	system_tick_t last_ack_handlers_update;

	uint32_t protocol_flags;

	uint8_t initialized;

protected:
	/**
	 * Protocol flags.
	 */
	enum ProtocolFlag
	{
		/**
		 * Set when the protocol expects a hello response from the server.
		 */
		REQUIRE_HELLO_RESPONSE = 0x01,
		/**
		 * Send ping as an empty message - this functions as a keep-alive for UDP.
		 */
		PING_AS_EMPTY_MESSAGE = 0x04,
		/**
		 * Application/system describe messages are initiated by the device.
		 */
		DEVICE_INITIATED_DESCRIBE = 0x08,
		/**
		 * Support for compressed/combined OTA updates.
		 */
		COMPRESSED_OTA = 0x10
	};

	/**
	 * Manages Ping functionality.
	 */
	Pinger pinger;

	/**
	 * Completion handlers for messages with confirmable delivery.
	 */
	CompletionHandlerMap<message_id_t> ack_handlers;

	/**
	 * The token ID for the next request made.
	 * If we have a bone-fide CoAP layer this will eventually disappear into that layer, just like message-id has.
	 */
	token_t next_token;

	void set_protocol_flags(uint32_t flags)
	{
		protocol_flags = flags;
	}

	/**
	 * Retrieves the next token.
	 */
	token_t get_next_token()
	{
		return next_token++;
	}

	ProtocolError handle_key_change(Message& message);

	/**
	 * Send the hello message over the channel.
	 * @param was_ota_upgrade_successful {@code true} if the previous OTA update was successful.
	 */
	ProtocolError hello(bool was_ota_upgrade_successful);

	/**
	 * Send a hello response
	 */
	ProtocolError hello_response();

	virtual size_t build_hello(Message& message, uint8_t flags)=0;

	/**
	 * Send a Ping message over the channel.
	 */
	ProtocolError ping(bool forceCoAP=false)
	{
		Message message;
		channel.create(message);
		size_t len = 0;
		if (!forceCoAP && (protocol_flags & ProtocolFlag::PING_AS_EMPTY_MESSAGE)) {
			len = Messages::keep_alive(message.buf());
		}
		else {
			len = Messages::ping(message.buf(), 0);
		}
		last_message_millis = callbacks.millis();
		message.set_length(len);
		return channel.send(message);
	}

	/**
	 * Send a Goodbye message over the channel.
	 */
	ProtocolError send_goodbye(cloud_disconnect_reason cloud_reason, network_disconnect_reason network_reason,
			System_Reset_Reason reset_reason, unsigned sleep_duration)
	{
		Message msg;
		channel.create(msg);
		const size_t n = Messages::goodbye(msg.buf(), msg.capacity(), 0, cloud_reason, network_reason, reset_reason,
				sleep_duration, channel.is_unreliable());
		if (n > msg.capacity()) {
			return INSUFFICIENT_STORAGE;
		}
		msg.set_length(n);
		return channel.send(msg);
	}

	/**
	 * Background processing when there are no messages to handle.
	 */
	ProtocolError event_loop_idle()
	{
#if HAL_PLATFORM_OTA_PROTOCOL_V3
		if (firmwareUpdate.isRunning())
		{
			return firmwareUpdate.process();
		}
#else
		if (chunkedTransfer.is_updating())
		{
			return chunkedTransfer.idle(channel);
		}
#endif // !HAL_PLATFORM_OTA_PROTOCOL_V3
		else
		{
			ProtocolError error = pinger.process(callbacks.millis() - last_message_millis, [this] {
				return ping();
			});
			if (error)
			{
				return error;
			}
		}
		return NO_ERROR;
	}

	/**
	 * @brief Generates and sends describe message
	 *
	 * @param channel The message channel used to send the message
	 * @param message The message buffer used to store the message
	 * @param header_size The offset at which to place the message payload
	 * @param desc_flags The information description flags
	 * @arg \p DESCRIBE_APPLICATION
	 * @arg \p DESCRIBE_METRICS
	 * @arg \p DESCRIBE_SYSTEM
	 *
	 * @returns \s ProtocolError result value
	 * @retval \p particle::protocol::NO_ERROR
	 *
	 * @sa particle::protocol::ProtocolError
	 */
	ProtocolError generate_and_send_description(MessageChannel& channel, Message& message,
												size_t header_size, int desc_flags);

	/**
	 * Produces a describe message and transmits it as a separate response.
	 * @param desc_flags Flags describing the information to provide. A combination of {@code DESCRIBE_APPLICATION) and {@code DESCRIBE_SYSTEM) flags.
	 */
	ProtocolError send_description_response(token_t token, message_id_t msg_id, int desc_flags);

	/**
	 * Decodes and dispatches a received message to its handler.
	 */
	ProtocolError handle_received_message(Message& message, CoAPMessageType::Enum& message_type);

	/**
	 * Sends an empty acknoweldgement for the given message.
	 */
	ProtocolError send_empty_ack(Message& request, message_id_t msg_id);

	/**
	 * Handles the time delivered from the cloud.
	 */
	void handle_time_response(uint32_t time);

	/**
	 * Copy an initialize a block of memory from a source to a target, where the source may be smaller than the target.
	 * This handles the case where the caller was compiled using a smaller version of the struct memory than what is the current.
	 *
	 * @param target			The destination structure
	 * @param target_size 	The size of the destination structure in bytes
	 * @param source			The source structure
	 * @param source_size	The size of the source structure in bytes
	 */
	static void copy_and_init(void* target, size_t target_size, const void* source, size_t source_size);

	void init(const SparkCallbacks &callbacks, const SparkDescriptor &descriptor);

	/**
	 * Returns a descriptor of the current application state.
	 */
	AppStateDescriptor app_state_descriptor(uint32_t stateFlags = AppStateDescriptor::ALL);

public:
	Protocol(MessageChannel& channel) :
			channel(channel),
			product_id(PRODUCT_ID),
			product_firmware_version(PRODUCT_FIRMWARE_VERSION),
			variables(this),
			publisher(this),
			last_ack_handlers_update(0),
			protocol_flags(0),
			initialized(false)
	{
	}

	virtual void init(const char *id,
	          const SparkKeys &keys,
	          const SparkCallbacks &callbacks,
	          const SparkDescriptor &descriptor)=0;

	void initialize_ping(system_tick_t interval, system_tick_t timeout)
	{
		pinger.init(interval, timeout);
	}

	void set_keepalive(system_tick_t interval, keepalive_source_t source)
	{
		pinger.set_interval(interval, source);
	}

	void set_fast_ota(unsigned data)
	{
#if !HAL_PLATFORM_OTA_PROTOCOL_V3
		chunkedTransfer.set_fast_ota(data);
#endif
	}

	void enable_device_initiated_describe()
	{
		protocol_flags |= ProtocolFlag::DEVICE_INITIATED_DESCRIBE;
	}

	void enable_compressed_ota()
	{
		protocol_flags |= ProtocolFlag::COMPRESSED_OTA;
	}

	void set_handlers(CommunicationsHandlers& handlers)
	{
		copy_and_init(&this->handlers, sizeof(this->handlers), &handlers, handlers.size);
	}

	void add_ack_handler(message_id_t msg_id, CompletionHandler handler, unsigned timeout)
	{
		ack_handlers.addHandler(msg_id, std::move(handler), timeout);
	}

	/**
	 * Establish a secure connection and send and process the hello message.
	 */
	int begin();

	/**
	 * Reset the protocol state and free all allocated resources.
	 */
	void reset();

	/**
	 * Wait for a specific message type to be received.
	 * @param message_type		The type of message wait for
	 * @param timeout			The duration to wait for the message before giving up.
	 *
	 * @returns NO_ERROR if the message was successfully matched within the timeout.
	 * Returns MESSAGE_TIMEOUT if the message wasn't received within the timeout.
	 * Other protocol errors may return additional error values.
	 */
	ProtocolError event_loop(CoAPMessageType::Enum message_type, system_tick_t timeout);

	/**
	 * Processes one event. Retrieves the type of the event processed, or NONE if no event processed.
	 * If an error occurs, the event type is undefined.
	 */
	ProtocolError event_loop(CoAPMessageType::Enum& message_type);

	/**
	 * no-arg version of event loop for those callers that don't care about the message.
	 */
	bool event_loop()
	{
		CoAPMessageType::Enum message;
		return !event_loop(message);
	}

	/**
	 * @brief Produces and transmits a describe message (POST request)
	 *
	 * @param desc_flags The information description flags
	 * @arg \p DESCRIBE_APPLICATION
	 * @arg \p DESCRIBE_METRICS
	 * @arg \p DESCRIBE_SYSTEM
	 *
	 * @param force Ignore cached application state.
	 *
	 * @returns \s ProtocolError result value
	 * @retval \p particle::protocol::NO_ERROR
	 *
	 * @sa particle::protocol::ProtocolError
	 */
	ProtocolError post_description(int desc_flags, bool force);

	// Returns true on success, false on sending timeout or rate-limiting failure
	bool send_event(const char *event_name, const char *data, int ttl,
			EventType::Enum event_type, int flags, CompletionHandler handler)
	{
#if HAL_PLATFORM_OTA_PROTOCOL_V3
		const bool updating = firmwareUpdate.isRunning();
#else
		const bool updating = chunkedTransfer.is_updating();
#endif
		if (updating)
		{
			handler.setError(SYSTEM_ERROR_BUSY);
			return false;
		}
		const ProtocolError error = publisher.send_event(channel, event_name, data, ttl, event_type, flags,
				callbacks.millis(), std::move(handler));
		if (error != NO_ERROR)
		{
			handler.setError(toSystemError(error));
			return false;
		}
		return true;
	}

	void build_describe_message(Appender& appender, int desc_flags);

	inline bool add_event_handler(const char *event_name, EventHandler handler)
	{
		return add_event_handler(event_name, handler, NULL,
				SubscriptionScope::FIREHOSE, NULL);
	}

	inline bool add_event_handler(const char *event_name, EventHandler handler,
			void *handler_data, SubscriptionScope::Enum scope,
			const char* device_id)
	{
		return !subscriptions.add_event_handler(event_name, handler,
				handler_data, scope, device_id);
	}

	inline bool remove_event_handlers(const char* name)
	{
		subscriptions.remove_event_handlers(name);
		return true;
	}

	ProtocolError send_subscription(const char *event_name, const char *device_id);
	ProtocolError send_subscription(const char *event_name, SubscriptionScope::Enum scope);
	ProtocolError send_subscriptions(bool force);

	/**
	 * Process a response for the previously sent system/application describe or subscriptions message.
	 *
	 * This method updates the cached application state.
	 *
	 * @param msg_id ID of the original request message.
	 * @param code Response code.
	 * @return `true` if the message has been handled or `false` otherwise.
	 */
	bool handle_app_state_reply(message_id_t msg_id, CoAPCode::Enum code);

	inline void set_product_id(product_id_t product_id)
	{
		this->product_id = product_id;
	}

	inline void set_product_firmware_version(
			product_firmware_version_t product_firmware_version)
	{
		this->product_firmware_version = product_firmware_version;
	}

	inline void get_product_details(product_details_t& details)
	{
		if (details.size >= 4)
		{
			details.product_id = this->product_id;
			details.product_version = this->product_firmware_version;
		}
	}

	inline bool send_time_request()
	{
#if HAL_PLATFORM_OTA_PROTOCOL_V3
		const bool updating = firmwareUpdate.isRunning();
#else
		const bool updating = chunkedTransfer.is_updating();
#endif
		if (updating)
		{
			return false;
		}

		return timesync_.send_request(callbacks.millis(), [&]() {
			uint8_t token = get_next_token();
			Message message;
			channel.create(message);
			size_t len = Messages::time_request(message.buf(), 0, token);
			message.set_length(len);
			return !channel.send(message);
		});
	}

	bool time_request_pending() const { return timesync_.is_request_pending(); }
	system_tick_t time_last_synced(time_t* tm) const { return timesync_.last_sync(*tm); }

	bool is_initialized() { return initialized; }

	int presence_announcement(uint8_t* buf, const uint8_t* id)
	{
		return -1;
	}

	system_tick_t millis() { return callbacks.millis(); }

	virtual int command(ProtocolCommands::Enum command, uint32_t value, const void* data)=0;

	virtual int get_describe_data(spark_protocol_describe_data* data, void* reserved);

	virtual int get_status(protocol_status* status) const = 0;

	void notify_message_complete(message_id_t msg_id, CoAPCode::Enum responseCode);

	const SparkDescriptor& getDescriptor() const {
		return descriptor;
	}

	MessageChannel& getChannel() {
		return channel;
	}
};

}
}
