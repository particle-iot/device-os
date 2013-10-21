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
  unsigned char *core_private;
  unsigned char *server_public;
};

struct SparkCallbacks
{
  int (*send)(const unsigned char *buf, int buflen);
  int (*receive)(unsigned char *buf, int buflen);
  void (*prepare_for_firmware_update)(void);
  void (*finish_firmware_update)(void);
  long unsigned int (*calculate_crc)(unsigned char *buf, long unsigned int buflen);
  void (*save_firmware_chunk)(unsigned char *buf, long unsigned int buflen);
};

class SparkProtocol
{
  public:
    static int presence_announcement(unsigned char *buf, const char *id);

    SparkProtocol();

    void init(const char *id,
              const SparkKeys &keys,
              const SparkCallbacks &callbacks,
              const SparkDescriptor &descriptor);
    int handshake(void);
    bool event_loop(void);
    bool is_initialized();

    int set_key(const unsigned char *signed_encrypted_credentials);
    int blocking_send(const unsigned char *buf, int length);
    int blocking_receive(unsigned char *buf, int length);

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
    void ping(unsigned char *buf);

    /********** Queue **********/
    const int QUEUE_SIZE;
    int queue_bytes_available();
    int queue_push(const char *src, int length);
    int queue_pop(char *dst, int length);

    /********** State Machine **********/
    ProtocolState::Enum state();

  private:
    char device_id[12];
    unsigned char server_public_key[294];
    unsigned char core_private_key[612];
    aes_context aes;
    int (*callback_send)(const unsigned char *buf, int buflen);
    int (*callback_receive)(unsigned char *buf, int buflen);
    void (*callback_prepare_for_firmware_update)(void);
    void (*callback_finish_firmware_update)(void);
    long unsigned int (*callback_calculate_crc)(unsigned char *buf, long unsigned int buflen);
    void (*callback_save_firmware_chunk)(unsigned char *buf, long unsigned int buflen);
    SparkDescriptor descriptor;
    unsigned char key[16];
    unsigned char iv_send[16];
    unsigned char iv_receive[16];
    unsigned char salt[8];
    unsigned short _message_id;
    int no_op_cycles;
    bool initialized;

    unsigned short next_message_id();
    void encrypt(unsigned char *buf, int length);
    void separate_response(unsigned char *buf, unsigned char token, unsigned char code);
    inline void empty_ack(unsigned char *buf,
                          unsigned char message_id_msb,
                          unsigned char message_id_lsb);
    inline void coded_ack(unsigned char *buf,
                          unsigned char token,
                          unsigned char code,
                          unsigned char message_id_msb,
                          unsigned char message_id_lsb);

    /********** Queue **********/
    unsigned char queue[640];
    const unsigned char *queue_mem_boundary;
    unsigned char *queue_front;
    unsigned char *queue_back;
    void queue_init(void);
};
