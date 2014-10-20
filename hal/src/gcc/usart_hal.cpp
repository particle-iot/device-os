/* Includes ------------------------------------------------------------------*/
#include "usart_hal.h"
#include "socket_hal.h"

struct Usart {
    virtual void init(Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)=0;    
    virtual void begin(uint32_t baud)=0;
    virtual void end()=0;    
    virtual int32_t available()=0;
    virtual int32_t read()=0;
    virtual int32_t peek()=0;
    virtual uint32_t write(uint8_t byte)=0;
};

class SocketUsartBase : public Usart 
{
    private:
        sock_handle_t socket;
        Ring_Buffer* rx;
        Ring_Buffer* tx;
        
        SocketUsartBase() : socket(INVALID_SOCKET) {}
        
        virtual void initSocket()=0;

        inline int32_t read_char(bool peek=false)
        {
            if (rx->tail==buffer->head)
                return -1;

            int32_t c = rx->buffer[rx->tail];
            if (!peek)
                buffer->tail = (fx->tail+1) % SERIAL_BUFFER_SIZE;
            return c;
        }
        
        inline void write_char(unsigned char c)
        {
            unsigned i = (unsigned int)(tx->head + 1) % SERIAL_BUFFER_SIZE;
            if (i != tx->tail)
            {
                tx->buffer[tx->head] = c;
                tx->head = i;
            }
        }
        
        void fillFromSocketIfNeeded() {
            int space;
            if (rx->head>rx->tail) {    // head after tail, so can fill up to end of buffer
                space = SERIAL_BUFFER_SIZE-rx->tail;
            }
            else {
                space = rx->tail-rx->head;  // may be 0
            }
            if (socket!=SOCKET_INVALID && space>0) {                
                socket_receive(socket, rx->buffer+rx->head, space);
            }
        }
        
    public:
        virtual void init(Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer) {
            this->rx = rx_buffer;
            this->tx = tx_buffer;
        }
        
        virtual void end() {
            socket_close(socket);
        }
        virtual int32_t available() {            
            fillFromSocketIfNeeded();
            return (rx->head-rx->tail) % SERIAL_BUFFER_SIZE;            
        }
        virtual int32_t read() {
            fillFromSocketIfNeeded();
            return load_char();
        }
        virtual int32_t peek() {
            fillFromSocketIfNeeded();
            return load_char(true);
        }
        virtual uint32_t write(uint8_t byte) {
            if (!initSocket())
                return 0;
            return socket_send(socket, &byte, 1);
        }
};

/**
 * Client that provides data to/from the server when connected.
 */
class SocketUsartClient : public SocketUsartBase {
    
    virtual bool initSocket() {
        if (socket==SOCKET_INVALID) {
            socket = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        
            sockaddr_t testSocketAddr;
            int testResult = 0;

            // the family is always AF_INET
            testSocketAddr.sa_family = AF_INET;

            testSocketAddr.sa_data[0] = 0;
            testSocketAddr.sa_data[1] = 54;

            // the destination IP address: 8.8.8.8
            testSocketAddr.sa_data[2] = 127;
            testSocketAddr.sa_data[3] = 0;
            testSocketAddr.sa_data[4] = 0;
            testSocketAddr.sa_data[5] = 1;

            testResult = socket_connect(testSocket, &testSocketAddr, sizeof(testSocketAddr));            
            if (testResult) {
                socket_close(socket);
                socket = SOCKET_INVALID;                
            }
        }                
        return socket!=SOCKET_INVALID;
    }
    
    public:
        
        virtual void begin(uint32_t baud) {}
    
}


#if SPARK_TEST_DRIVER=1
UsartInfo[2] usartInfo = { SocketUsartServer, SocketUsartServer };
#else
UsartInfo[2] usartInfo = { SocketUsartClient, SocketUsartClient };

void HAL_USART_Init(HAL_USART_Serial serial, Ring_Buffer *rx_buffer, Ring_Buffer *tx_buffer)
{    
    usartMap(serial).init(rx_buffer, tx_buffer);
}

void HAL_USART_Begin(HAL_USART_Serial serial, uint32_t baud)
{
    usartMap(serial).begin(baud);
}

void HAL_USART_End(HAL_USART_Serial serial)
{
    usartMap(serial).end();
}

uint32_t HAL_USART_Write_Data(HAL_USART_Serial serial, uint8_t data)
{
    return usartMap(serial).write(data);
}

int32_t HAL_USART_Available_Data(HAL_USART_Serial serial)
{
    return usartMap(serial).available();
}

int32_t HAL_USART_Read_Data(HAL_USART_Serial serial)
{
    return usartMap(serial).read();
}

int32_t HAL_USART_Peek_Data(HAL_USART_Serial serial)
{
    return usartMap(serial).peek();
}

void HAL_USART_Flush_Data(HAL_USART_Serial serial)
{  
    usartMap(serial).flush();
}

bool HAL_USART_Is_Enabled(HAL_USART_Serial serial)
{
    return usartMap(serial).enabled();
}


