
#include <string.h>
#include <stdio.h>
#include "wiced.h"
#include "hci_usart_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "delay_hal.h"
#include "stm32f2xx.h"
#include "btstack.h"
#include "hci_dump.h"
#include "btstack_debug.h"

#define TIMEOUT_COUNT    0x03FF

/* Private typedef -----------------------------------------------------------*/
typedef enum HCI_USART_Num_Def {
    BT_UART = 0
} HCI_USART_Num_Def;

typedef enum
{
    HCI_CMD_OPCODE_RESET               = 0x0C03,
    HCI_CMD_OPCODE_DOWNLOAD_MINIDRIVER = 0xFC2E,
    HCI_CMD_OPCODE_WRITE_RAM           = 0xFC4C,
    HCI_CMD_OPCODE_LAUNCH_RAM          = 0xFC4E,
}hci_opcode_t;

typedef struct STM32_USART_Info {
    USART_TypeDef* usart_peripheral;

    __IO uint32_t* usart_apbReg;
    uint32_t usart_clock_en;

    int32_t usart_int_n;

    uint16_t usart_tx_pin;
    uint16_t usart_rx_pin;

    uint8_t usart_tx_pinsource;
    uint8_t usart_rx_pinsource;

    uint8_t usart_af_map;

    // Buffer pointers. These need to be global for IRQ handler access
    HCI_USART_Ring_Buffer* usart_tx_buffer;
    HCI_USART_Ring_Buffer* usart_rx_buffer;

    bool usart_enabled;
    bool usart_transmitting;
} STM32_USART_Info;

/* Private variables ---------------------------------------------------------*/
/*
 * USART mapping
 */
STM32_USART_Info HCI_USART_MAP[TOTAL_HCI_USARTS] =
{
    /*
     * USART_peripheral (USARTx/UARTx; not using others)
     * clock control register (APBxENR)
     * clock enable bit value (RCC_APBxPeriph_USARTx/RCC_APBxPeriph_UARTx)
     * interrupt number (USARTx_IRQn/UARTx_IRQn)
     * TX pin
     * RX pin
     * TX pin source
     * RX pin source
     * GPIO AF map (GPIO_AF_USARTx/GPIO_AF_UARTx)
     * <tx_buffer pointer> used internally and does not appear below
     * <rx_buffer pointer> used internally and does not appear below
     * <usart enabled> used internally and does not appear below
     * <usart transmitting> used internally and does not appear below
     */
    { USART6, &RCC->APB2ENR, RCC_APB2Periph_USART6, USART6_IRQn, BT_TX, BT_RX, GPIO_PinSource6, GPIO_PinSource7, GPIO_AF_USART6 } // USART 6
};

static USART_InitTypeDef USART_InitStructure;
static STM32_USART_Info *usartMap[TOTAL_HCI_USARTS]; // pointer to USART_MAP[] containing USART peripheral register locations (etc)

/*
 * HCI cmd and event for download firmware.
 */
static const uint8_t hci_cmd_reset[4]                 = {0x01,0x03,0x0C,0x00};
static const uint8_t hci_cmd_download_minidriver[4]   = {0x01,0x2E,0xFC,0x00};
static const uint8_t hci_cmd_write_ram[4]             = {0x01,0x4C,0xFC,0x00};
static const uint8_t hci_cmd_launch_ram[4]            = {0x01,0x4E,0xFC,0x00};

static const uint8_t hci_event_reset[7]               = {0x04,0x0E,0x04,0x01,0x03,0x0C,0x00};
static const uint8_t hci_event_download_minidriver[7] = {0x04,0x0E,0x04,0x01,0x2E,0xFC,0x00};
static const uint8_t hci_event_write_ram[7]           = {0x04,0x0E,0x04,0x01,0x4C,0xFC,0x00};
static const uint8_t hci_event_launch_ram[7]          = {0x04,0x0E,0x04,0x01,0x4E,0xFC,0x00};
/*
 * HCI firmware.
 */
extern const char    brcm_patch_version[];
extern const uint8_t brcm_patchram_buf[];
extern const int     brcm_patch_ram_length;

/*
 * HCI event handler.
 */
static ReceiveHandler_t receiveHandler=NULL;

/* Function Definitions -------------------------------------------------------*/

void HAL_HCI_USART_registerReceiveHandler(ReceiveHandler_t handler)
{
    receiveHandler = handler;
}

void HAL_HCI_USART_receiveEvent(void)
{
    if(receiveHandler != NULL)
        receiveHandler();
}

int32_t HAL_HCI_USART_downloadFirmeare(HAL_HCI_USART_Serial serial)
{
    uint8_t  buf[12];
    uint16_t timeout, index;

    // Backup receive handler.
    ReceiveHandler_t backup_handler = receiveHandler;

    receiveHandler = NULL;
    // Send reset command.
    timeout = 0;
    HAL_HCI_USART_Write_Buffer(serial, hci_cmd_reset, sizeof(hci_cmd_reset));
    while( (HAL_HCI_USART_Available_Data(serial)<sizeof(hci_event_reset)) && (timeout<TIMEOUT_COUNT) )
    {
        timeout++;
        HAL_Delay_Milliseconds(1);
    }

    if(timeout >= TIMEOUT_COUNT)
    {
        while( HAL_HCI_USART_Available_Data(serial) )
            HAL_HCI_USART_Read_Data(serial);
        return 1;
    }

    for(index=0; index<sizeof(hci_event_reset); index++)
        buf[index] = HAL_HCI_USART_Read_Data(serial);

    if( 0x00 != memcmp(buf, hci_event_reset, sizeof(hci_event_reset)) )
        return 1;

    // Send download minidriver command.
    timeout = 0;
    HAL_HCI_USART_Write_Buffer(serial, hci_cmd_download_minidriver, sizeof(hci_cmd_download_minidriver));
    while( (HAL_HCI_USART_Available_Data(serial)<sizeof(hci_event_download_minidriver)) && (timeout<TIMEOUT_COUNT) )
    {
        timeout++;
        HAL_Delay_Milliseconds(1);
    }

    if(timeout >= TIMEOUT_COUNT)
    {
        while( HAL_HCI_USART_Available_Data(serial) )
            HAL_HCI_USART_Read_Data(serial);
        return 1;
    }

    for(index=0; index<sizeof(hci_event_download_minidriver); index++)
        buf[index] = HAL_HCI_USART_Read_Data(serial);

    if( 0x00 != memcmp(buf, hci_event_download_minidriver, sizeof(hci_event_download_minidriver)) )
        return 1;

    const uint8_t*      firmware;
    uint32_t            data_length;
    uint32_t            firmware_size;
    hci_opcode_t        command_opcode;

    firmware = (uint8_t*)brcm_patchram_buf;
    firmware_size = brcm_patch_ram_length;

    while(firmware_size)
    {
        // content of data length + 2 bytes of opcode and 1 byte of data length
        data_length    = firmware[2] + 3;
        command_opcode = *(hci_opcode_t*)firmware;

        // Send hci_write_ram command. The length of the data immediately follows the command opcode
        timeout = 0;
        // 43438 requires the packet type before each write RAM command
        HAL_HCI_USART_Write_Data(serial, 0x01);
        HAL_HCI_USART_Write_Buffer(serial, firmware, data_length);
        while( (HAL_HCI_USART_Available_Data(serial)<0x07) && (timeout<TIMEOUT_COUNT) )
        {
            timeout++;
            HAL_Delay_Milliseconds(1);
        }

        if(timeout >= TIMEOUT_COUNT)
        {
            while( HAL_HCI_USART_Available_Data(serial) )
                HAL_HCI_USART_Read_Data(serial);
            return 1;
        }

        for(index=0; index<0x07; index++)
            buf[index] = HAL_HCI_USART_Read_Data(serial);

        switch(command_opcode)
        {
            case HCI_CMD_OPCODE_WRITE_RAM:
                if( 0x00 != memcmp(buf, hci_event_write_ram, sizeof(hci_event_write_ram)) )
                    return 1;
                // Update remaining length and data pointer
                firmware += data_length;
                firmware_size -= data_length;
            break;

            case HCI_CMD_OPCODE_LAUNCH_RAM:
                if( 0x00 != memcmp(buf, hci_event_launch_ram, sizeof(hci_event_launch_ram)) )
                    return 1;

                while( HAL_HCI_USART_Available_Data(serial) )
                    HAL_HCI_USART_Read_Data(serial);

                firmware_size = 0;
            break;

            default:
                return 1;
            break;
        }
        HAL_Delay_Milliseconds(1);
    }

    timeout=0;
    while( (0x01==HAL_GPIO_Read(BT_CTS)) && (timeout<TIMEOUT_COUNT) )
    {
        timeout++;
        HAL_Delay_Milliseconds(1);
    }

    if(timeout >= TIMEOUT_COUNT)
        return 1;

    memset(usartMap[serial]->usart_rx_buffer, 0, sizeof(HCI_USART_Ring_Buffer));
    memset(usartMap[serial]->usart_tx_buffer, 0, sizeof(HCI_USART_Ring_Buffer));

    receiveHandler = backup_handler;

    return 0;
}

inline void store_char(unsigned char c, HCI_USART_Ring_Buffer *buffer) __attribute__((always_inline));
inline void store_char(unsigned char c, HCI_USART_Ring_Buffer *buffer)
{
    unsigned int i = (unsigned int)(buffer->head + 1) % HCI_USART_BUFFER_SIZE;

    if (i != buffer->tail)
    {
        buffer->buffer[buffer->head] = c;
        buffer->head = i;
    }
    else
    {
    	log_info("rx_buffer is full!!");
    }
}

void HAL_HCI_USART_Init(HAL_HCI_USART_Serial serial, HCI_USART_Ring_Buffer *rx_buffer, HCI_USART_Ring_Buffer *tx_buffer)
{

    if(serial == HAL_HCI_USART_SERIAL6)
    {
        usartMap[serial] = &HCI_USART_MAP[BT_UART];
    }

    usartMap[serial]->usart_rx_buffer = rx_buffer;
    usartMap[serial]->usart_tx_buffer = tx_buffer;

    memset(usartMap[serial]->usart_rx_buffer, 0, sizeof(HCI_USART_Ring_Buffer));
    memset(usartMap[serial]->usart_tx_buffer, 0, sizeof(HCI_USART_Ring_Buffer));

    usartMap[serial]->usart_enabled = false;
    usartMap[serial]->usart_transmitting = false;
}

void HAL_HCI_USART_Begin(HAL_HCI_USART_Serial serial, uint32_t baud)
{
    USART_DeInit(usartMap[serial]->usart_peripheral);

    // Configure USART Rx and Tx as alternate function push-pull, and enable GPIOA clock
    HAL_Pin_Mode(usartMap[serial]->usart_rx_pin, AF_OUTPUT_PUSHPULL);
    HAL_Pin_Mode(usartMap[serial]->usart_tx_pin, AF_OUTPUT_PUSHPULL);

    // Enable USART Clock
    *usartMap[serial]->usart_apbReg |=  usartMap[serial]->usart_clock_en;

    // Connect USART pins to AFx
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->usart_rx_pin].gpio_peripheral, usartMap[serial]->usart_rx_pinsource, usartMap[serial]->usart_af_map);
    GPIO_PinAFConfig(PIN_MAP[usartMap[serial]->usart_tx_pin].gpio_peripheral, usartMap[serial]->usart_tx_pinsource, usartMap[serial]->usart_af_map);

    // NVIC Configuration
    NVIC_InitTypeDef NVIC_InitStructure;
    // Enable the USART Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = usartMap[serial]->usart_int_n;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // USART default configuration
    // USART configured as follow:
    // - BaudRate = (set baudRate as 9600 baud)
    // - Word Length = 8 Bits
    // - One Stop Bit
    // - No parity
    // - Hardware flow control disabled for Serial1/2/4/5
    // - Hardware flow control enabled (RTS and CTS signals) for Serial3
    // - Receive and transmit enabled
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    // Configure USART
    USART_Init(usartMap[serial]->usart_peripheral, &USART_InitStructure);

    // Enable the USART
    USART_Cmd(usartMap[serial]->usart_peripheral, ENABLE);

    usartMap[serial]->usart_enabled = true;
    usartMap[serial]->usart_transmitting = false;

    // Enable USART Receive and Transmit interrupts
    USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, ENABLE);
    USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_RXNE, ENABLE);
}

void HAL_HCI_USART_End(HAL_HCI_USART_Serial serial)
{
    // wait for transmission of outgoing data
    while (usartMap[serial]->usart_tx_buffer->head != usartMap[serial]->usart_tx_buffer->tail);

    // Disable the USART
    USART_Cmd(usartMap[serial]->usart_peripheral, DISABLE);

    // Deinitialise USART
    USART_DeInit(usartMap[serial]->usart_peripheral);

    // Disable USART Receive and Transmit interrupts
    USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_RXNE, DISABLE);
    USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, DISABLE);

    NVIC_InitTypeDef NVIC_InitStructure;

    // Disable the USART Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = usartMap[serial]->usart_int_n;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

    NVIC_Init(&NVIC_InitStructure);

    // Disable USART Clock
    *usartMap[serial]->usart_apbReg &= ~usartMap[serial]->usart_clock_en;

    // clear any received data
    usartMap[serial]->usart_rx_buffer->head = usartMap[serial]->usart_rx_buffer->tail;

    // Undo any pin re-mapping done for this USART
    // ...

    memset(usartMap[serial]->usart_rx_buffer, 0, sizeof(HCI_USART_Ring_Buffer));
    memset(usartMap[serial]->usart_tx_buffer, 0, sizeof(HCI_USART_Ring_Buffer));

    usartMap[serial]->usart_enabled = false;
    usartMap[serial]->usart_transmitting = false;
}

int32_t HAL_HCI_USART_Write_Data(HAL_HCI_USART_Serial serial, uint8_t data)
{
    unsigned int status;

    unsigned int i = HCI_USART_BUFFER_SIZE - ((HCI_USART_BUFFER_SIZE + usartMap[serial]->usart_tx_buffer->head - usartMap[serial]->usart_tx_buffer->tail) % HCI_USART_BUFFER_SIZE);
    if(i > 1)
    {
        usartMap[serial]->usart_tx_buffer->buffer[usartMap[serial]->usart_tx_buffer->head] = data;
        usartMap[serial]->usart_tx_buffer->head = (usartMap[serial]->usart_tx_buffer->head + 1) % HCI_USART_BUFFER_SIZE;;
        usartMap[serial]->usart_transmitting = true;
        status = 1;
    }
    else
    {
    	log_error("tx_buffer is full!!");
        status = 0;
    }

    if( 0==HAL_GPIO_Read(BT_CTS) )
        USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, ENABLE);
    else
        USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, DISABLE);

    return status;
}

int32_t HAL_HCI_USART_Write_Buffer(HAL_HCI_USART_Serial serial, const uint8_t *buf, uint16_t len)
{
    unsigned int status, n=0;

    while(len--)
    {
        status = HAL_HCI_USART_Write_Data(serial, *buf++);
        if(status > 0)
            n++;
        else
            break;
    }
    return n;
}

int32_t HAL_HCI_USART_Available_Data(HAL_HCI_USART_Serial serial)
{
    return (unsigned int)(HCI_USART_BUFFER_SIZE + usartMap[serial]->usart_rx_buffer->head - usartMap[serial]->usart_rx_buffer->tail) % HCI_USART_BUFFER_SIZE;
}

void HAL_HCI_USART_RestartSend(HAL_HCI_USART_Serial serial)
{
    if( (0==HAL_GPIO_Read(BT_CTS)) &&
        (USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_TXE) == RESET) &&
        (usartMap[serial]->usart_tx_buffer->head!=usartMap[serial]->usart_tx_buffer->tail) )
    {
        USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, ENABLE);
    }
}

int32_t HAL_HCI_USART_Read_Data(HAL_HCI_USART_Serial serial)
{
    // if the head isn't ahead of the tail, we don't have any characters
    if (usartMap[serial]->usart_rx_buffer->head == usartMap[serial]->usart_rx_buffer->tail)
    {
        return -1;
    }
    else
    {
        unsigned char c = usartMap[serial]->usart_rx_buffer->buffer[usartMap[serial]->usart_rx_buffer->tail];
        usartMap[serial]->usart_rx_buffer->tail = (unsigned int)(usartMap[serial]->usart_rx_buffer->tail + 1) % HCI_USART_BUFFER_SIZE;
        return c;
    }
}


// Serial6 interrupt handler
static void HAL_HCI_USART_IRQHandler(HAL_HCI_USART_Serial serial)
{
    if(USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_RXNE) != RESET)
    {
        // Read byte from the receive data register
        unsigned char c = USART_ReceiveData(usartMap[serial]->usart_peripheral);
        store_char(c, usartMap[serial]->usart_rx_buffer);
        if( ((HCI_USART_BUFFER_SIZE + usartMap[serial]->usart_rx_buffer->head - usartMap[serial]->usart_rx_buffer->tail) % HCI_USART_BUFFER_SIZE) > (HCI_USART_BUFFER_SIZE-5) )
        	HAL_GPIO_Write(BT_RTS, 1);
    }

    if(USART_GetITStatus(usartMap[serial]->usart_peripheral, USART_IT_TXE) != RESET)
    {
        // Write byte to the transmit data register
        if( ( 0==HAL_GPIO_Read(BT_CTS)) && (usartMap[serial]->usart_tx_buffer->head!=usartMap[serial]->usart_tx_buffer->tail))
        {
            // There is more data in the output buffer. Send the next byte
            USART_SendData(usartMap[serial]->usart_peripheral, usartMap[serial]->usart_tx_buffer->buffer[usartMap[serial]->usart_tx_buffer->tail++]);
            usartMap[serial]->usart_tx_buffer->tail %= HCI_USART_BUFFER_SIZE;
        }
        else
        {
            // Buffer empty or CTS is HIGH, so disable the USART Transmit interrupt
            USART_ITConfig(usartMap[serial]->usart_peripheral, USART_IT_TXE, DISABLE);
        }
    }
}
/*******************************************************************************
 * Function Name  : HAL_USART6_Handler
 * Description    : This function handles UART6 global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_USART6_Handler(void)
{
    HAL_HCI_USART_IRQHandler(HAL_HCI_USART_SERIAL6);
}


