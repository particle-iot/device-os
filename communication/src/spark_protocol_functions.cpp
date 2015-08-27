
#include "spark_protocol_functions.h"
#include "spark_protocol.h"
#include "handshake.h"
#include <stdlib.h>

/**
 * Handle the cryptographically secure random seed from the cloud by using
 * it to seed the stdlib PRNG.
 * @param seed  A random value from a cryptographically secure random number generator.
 */
void default_random_seed_from_cloud(unsigned int seed)
{
    srand(seed);
}


int decrypt_rsa(const uint8_t* ciphertext, const uint8_t* private_key, uint8_t* plaintext, int plaintext_len)
{
    rsa_context rsa;
    init_rsa_context_with_private_key(&rsa, private_key);
    int err = rsa_pkcs1_decrypt(&rsa, RSA_PRIVATE, &plaintext_len, ciphertext, plaintext, plaintext_len);
    rsa_free(&rsa);
    return err ? -abs(err) : plaintext_len;
}


void spark_protocol_communications_handlers(SparkProtocol* protocol, CommunicationsHandlers* handlers)
{
    protocol->set_handlers(*handlers);
}

void spark_protocol_init(SparkProtocol* protocol, const char *id,
          const SparkKeys &keys,
          const SparkCallbacks &callbacks,
          const SparkDescriptor &descriptor, void* reserved)
{
    protocol->init(id, keys, callbacks, descriptor);
}

int spark_protocol_handshake(SparkProtocol* protocol, void* reserved) {
    protocol->reset_updating();
    return protocol->handshake();
}

bool spark_protocol_event_loop(SparkProtocol* protocol, void* reserved) {
    return protocol->event_loop();
}

bool spark_protocol_is_initialized(SparkProtocol* protocol) {
    return protocol->is_initialized();
}

int spark_protocol_presence_announcement(SparkProtocol* protocol, unsigned char *buf, const char *id, void*) {
    return protocol->presence_announcement(buf, id);
}

bool spark_protocol_send_event(SparkProtocol* protocol, const char *event_name, const char *data,
                int ttl, EventType::Enum event_type, void*) {
    return protocol->send_event(event_name, data, ttl, event_type);
}

bool spark_protocol_send_subscription_device(SparkProtocol* protocol, const char *event_name, const char *device_id, void* reserved) {
    return protocol->send_subscription(event_name, device_id);
}

bool spark_protocol_send_subscription_scope(SparkProtocol* protocol, const char *event_name, SubscriptionScope::Enum scope, void* reserved) {
    return protocol->send_subscription(event_name, scope);
}

bool spark_protocol_add_event_handler(SparkProtocol* protocol, const char *event_name,
    EventHandler handler, SubscriptionScope::Enum scope, const char* device_id, void* handler_data) {
    return protocol->add_event_handler(event_name, handler, handler_data, scope, device_id);
}

bool spark_protocol_send_time_request(SparkProtocol* protocol, void* reserved) {
    return protocol->send_time_request();
}

void spark_protocol_send_subscriptions(SparkProtocol* protocol, void* reserved) {
    protocol->send_subscriptions();
}

void spark_protocol_remove_event_handlers(SparkProtocol* protocol, const char* event_name, void* reserved) {
    protocol->remove_event_handlers(event_name);
}

void spark_protocol_set_product_id(SparkProtocol* protocol, product_id_t product_id, unsigned, void*) {
    protocol->set_product_id(product_id);
}

void spark_protocol_set_product_firmware_version(SparkProtocol* protocol, product_firmware_version_t product_firmware_version, unsigned, void*) {
    protocol->set_product_firmware_version(product_firmware_version);
}

void spark_protocol_get_product_details(SparkProtocol* protocol, product_details_t* details, void* reserved) {
    protocol->get_product_details(*details);
}
