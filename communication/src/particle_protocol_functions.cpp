
#include "particle_protocol_functions.h"
#include "particle_protocol.h"
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


void particle_protocol_communications_handlers(ParticleProtocol* protocol, CommunicationsHandlers* handlers)
{
    protocol->set_handlers(*handlers);
}

void particle_protocol_init(ParticleProtocol* protocol, const char *id,
          const ParticleKeys &keys,
          const ParticleCallbacks &callbacks,
          const ParticleDescriptor &descriptor, void* reserved)
{
    protocol->init(id, keys, callbacks, descriptor);
}

int particle_protocol_handshake(ParticleProtocol* protocol, void* reserved) {
    protocol->reset_updating();
    return protocol->handshake();
}

bool particle_protocol_event_loop(ParticleProtocol* protocol, void* reserved) {
    return protocol->event_loop();
}

bool particle_protocol_is_initialized(ParticleProtocol* protocol) {
    return protocol->is_initialized();
}

int particle_protocol_presence_announcement(ParticleProtocol* protocol, unsigned char *buf, const char *id, void*) {
    return protocol->presence_announcement(buf, id);
}

bool particle_protocol_send_event(ParticleProtocol* protocol, const char *event_name, const char *data,
                int ttl, EventType::Enum event_type, void*) {
    return protocol->send_event(event_name, data, ttl, event_type);
}

bool particle_protocol_send_subscription_device(ParticleProtocol* protocol, const char *event_name, const char *device_id, void* reserved) {
    return protocol->send_subscription(event_name, device_id);
}

bool particle_protocol_send_subscription_scope(ParticleProtocol* protocol, const char *event_name, SubscriptionScope::Enum scope, void* reserved) {
    return protocol->send_subscription(event_name, scope);
}

bool particle_protocol_add_event_handler(ParticleProtocol* protocol, const char *event_name,
    EventHandler handler, SubscriptionScope::Enum scope, const char* device_id, void* handler_data) {
    return protocol->add_event_handler(event_name, handler, handler_data, scope, device_id);
}

bool particle_protocol_send_time_request(ParticleProtocol* protocol, void* reserved) {
    return protocol->send_time_request();
}

void particle_protocol_send_subscriptions(ParticleProtocol* protocol, void* reserved) {
    protocol->send_subscriptions();
}

void particle_protocol_remove_event_handlers(ParticleProtocol* protocol, const char* event_name, void* reserved) {
    protocol->remove_event_handlers(event_name);
}

void particle_protocol_set_product_id(ParticleProtocol* protocol, product_id_t product_id, unsigned, void*) {
    protocol->set_product_id(product_id);
}

void particle_protocol_set_product_firmware_version(ParticleProtocol* protocol, product_firmware_version_t product_firmware_version, unsigned, void*) {
    protocol->set_product_firmware_version(product_firmware_version);
}

void particle_protocol_get_product_details(ParticleProtocol* protocol, product_details_t* details, void* reserved) {
    protocol->get_product_details(*details);
}
