#include "spark_descriptor.h"
#include "coap.h"
#include "tropicssl/rsa.h"
#include "tropicssl/aes.h"

namespace ProtocolState {
  enum Enum {
    READ_NONCE
  };
}

namespace ChunkReceivedCode {
  enum Enum {
    OK = 0x44,
    BAD = 0x80
  };
}

struct SparkKeys
{
  const unsigned char *core_private;
  const unsigned char *server_public;
};

struct SparkCallbacks
{
  int (*send)(const unsigned char *buf, int buflen);
  int (*receive)(unsigned char *buf, int buflen);
};

class SparkProtocol
{
  public:
    SparkProtocol();

    void init(const char *id,
              const SparkKeys &keys,
              const SparkCallbacks &callbacks,
              SparkDescriptor *descriptor);
    int handshake(void);
    void event_loop(void);

    int set_key(const unsigned char *signed_encrypted_credentials);
    void blocking_send(const unsigned char *buf, int length);
    void blocking_receive(unsigned char *buf, int length);

    CoAPMessageType::Enum received_message(unsigned char *buf, int length);
    void hello(unsigned char *buf);
    void key_changed(unsigned char *buf, unsigned char token);
    void function_return(unsigned char *buf, unsigned char token,
                         int return_value);
    void variable_value(unsigned char *buf, unsigned char token,
                        bool return_value);
    void variable_value(unsigned char *buf, unsigned char token,
                        int return_value);
    void variable_value(unsigned char *buf, unsigned char token,
                        unsigned char message_id_msb, unsigned char message_id_lsb,
                        double return_value);
    void variable_value(unsigned char *buf, unsigned char token,
                        const void *return_value, int length);
    void event(unsigned char *buf,
               const char *event_name,
               int event_name_length);
    void event(unsigned char *buf,
               const char *event_name,
               int event_name_length,
               const char *data,
               int data_length);
    void chunk_received(unsigned char *buf, unsigned char token,
                        ChunkReceivedCode::Enum code);
    void update_ready(unsigned char *buf, unsigned char token);
    int description(unsigned char *buf, unsigned char token,
                     const char **function_names, int num_functions);

    /********** Queue **********/
    const int QUEUE_SIZE;
    int queue_bytes_available();
    int queue_push(const char *src, int length);
    int queue_pop(char *dst, int length);

    /********** State Machine **********/
    ProtocolState::Enum state();

  private:
    const SparkKeys *rsa_keys;
    aes_context aes;
    int (*callback_send)(const unsigned char *buf, int buflen);
    int (*callback_receive)(unsigned char *buf, int buflen);
    SparkDescriptor *descriptor;
    unsigned char key[16];
    unsigned char iv_send[16];
    unsigned char iv_receive[16];
    unsigned char salt[8];
    unsigned short _message_id;

    unsigned short next_message_id();
    void encrypt(unsigned char *buf, int length);
    void separate_response(unsigned char *buf, unsigned char token, unsigned char code);
    inline void empty_ack(unsigned char *buf,
                          unsigned char message_id_msb,
                          unsigned char message_id_lsb);

    /********** Queue **********/
    unsigned char queue[640];
    const unsigned char *queue_mem_boundary;
    unsigned char *queue_front;
    unsigned char *queue_back;
    void queue_init(void);
};
