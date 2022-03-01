/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
 
 #ifndef SDSPI_HOST_H
 #define SDSPI_HOST_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "Particle.h"

#if PLATFORM_ID == PLATFORM_TRACKER

typedef int32_t esp_err_t;

// #define MIN(x, y)  ((x) < (y) ? (x) : (y))

/// Transfer format in SPI mode. See section 7.3.1.1 of SD simplified spec.
typedef struct {
    // These fields form the command sent from host to the card (6 bytes)
    uint8_t cmd_index : 6;
    uint8_t transmission_bit : 1;
    uint8_t start_bit : 1;
    uint8_t arguments[4];
    uint8_t stop_bit : 1;
    uint8_t crc7 : 7;
    /// Ncr is the dead time between command and response; should be 0xff
    uint8_t ncr;
    /// Response data, should be set by host to 0xff for read operations
    uint8_t r1;
    /// Up to 16 bytes of response. Luckily, this is aligned on 4 byte boundary.
    uint32_t response[4];
    /// response timeout, in milliseconds
    //int timeout_ms;
} sdspi_hw_cmd_t;

#define MMC_GO_IDLE_STATE               0       /* R0 */
/* SD IO commands */
#define SD_IO_SEND_OP_COND              5       /* R4 */
#define SD_IO_RW_DIRECT                 52      /* R5 */
#define SD_IO_RW_EXTENDED               53      /* R5 */
#define SD_CRC_ON_OFF                   59      /* R1 */

/* CMD53 arguments */
#define SD_ARG_CMD53_READ           ((uint32_t)0<<31)
#define SD_ARG_CMD53_WRITE          ((uint32_t)1<<31)
#define SD_ARG_CMD53_BLOCK_MODE     ((uint32_t)1<<27)

#define SDSPI_CMD_SIZE      6
#define SDSPI_NCR_MIN_SIZE  1
#define SDSPI_NCR_MAX_SIZE  8

#define MMC_OCR_3_3V_3_4V               ((uint32_t)1<<21)
//#define MMC_OCR_3_2V_3_3V               ((uint32_t)1<<20)

//the size here contains 6 bytes of CMD, 1 bytes of dummy and the actual response
#define SDSPI_CMD_NORESP_SIZE   (SDSPI_CMD_SIZE+0)   //!< Size of the command without any response
#define SDSPI_CMD_R1_SIZE       (SDSPI_CMD_SIZE+SDSPI_NCR_MIN_SIZE+1)   //!< Size of the command with R1 response
#define SDSPI_CMD_R4_SIZE       (SDSPI_CMD_SIZE+SDSPI_NCR_MIN_SIZE+5)  //!< Size of the command with R4 response
#define SDSPI_CMD_R5_SIZE       (SDSPI_CMD_SIZE+SDSPI_NCR_MIN_SIZE+2)   //!< Size of the command with R5 response

#define SDSPI_CMD_FLAG_DATA     BIT(0)  //!< Command has data transfer
#define SDSPI_CMD_FLAG_WRITE    BIT(1)  //!< Data is written to the card
#define SDSPI_CMD_FLAG_RSP_R1   BIT(2)  //!< Response format R1 (1 byte)
#define SDSPI_CMD_FLAG_RSP_R4   BIT(5)  //!< Response format R4 (5 bytes)
#define SDSPI_CMD_FLAG_RSP_R5   BIT(6)  //!< Response format R5 (2 bytes)
#define SDSPI_CMD_FLAG_NORSP    BIT(8)  //!< Don't expect response (used when sending CMD0 first time).
#define SDSPI_CMD_FLAG_MULTI_BLK BIT(9)  //!< This indicates the start token should be a multiblock one

/* SPI mode R1 response type bits */
#define SD_SPI_R1_IDLE_STATE            (1<<0)
#define SD_SPI_R1_ERASE_RST             (1<<1)
#define SD_SPI_R1_ILLEGAL_CMD           (1<<2)
#define SD_SPI_R1_CMD_CRC_ERR           (1<<3)
#define SD_SPI_R1_ERASE_SEQ_ERR         (1<<4)
#define SD_SPI_R1_ADDR_ERR              (1<<5)
#define SD_SPI_R1_PARAM_ERR             (1<<6)
#define SD_SPI_R1_NO_RESPONSE           (1<<7)

#define SDSPI_MAX_DATA_LEN      512     //!< Max size of single block transfer

#define SCF_CMD(flags)   ((flags) & 0x00f0)

/* CMD52 arguments */
#define SD_ARG_CMD52_READ           ((uint32_t)0<<31)
#define SD_ARG_CMD52_WRITE          ((uint32_t)1<<31)
#define SD_ARG_CMD52_FUNC_SHIFT     28
#define SD_ARG_CMD52_FUNC_MASK      0x7
#define SD_ARG_CMD52_EXCHANGE       ((uint32_t)1<<27)
#define SD_ARG_CMD52_REG_SHIFT      9
#define SD_ARG_CMD52_REG_MASK       0x1ffff
#define SD_ARG_CMD52_DATA_SHIFT     0
#define SD_ARG_CMD52_DATA_MASK      0xff
#define SD_R5_DATA(resp)            ((resp)[0] & 0xff)

/// Maximum number of dummy bytes between the request and response (minimum is 1)
#define SDSPI_RESPONSE_MAX_DELAY  8

#define SDSPI_MOSI_IDLE_VAL     0xff    //!< Data value which causes MOSI to stay high

/// Token sent before single/multi block reads and single block writes
#define TOKEN_BLOCK_START                   0xFE
/// Token sent before multi block writes
#define TOKEN_BLOCK_START_WRITE_MULTI       0xFC
/// Token used to stop multi block write (for reads, CMD12 is used instead)
#define TOKEN_BLOCK_STOP_WRITE_MULTI        0

#define ESP_SLAVE_CMD53_END_ADDR    0x1f800

#define TX_BUFFER_MAX   0x1000
#define TX_BUFFER_MASK  0xFFF
#define RX_BYTE_MAX     0x100000
#define RX_BYTE_MASK    0xFFFFF

#define SD_ARG_CMD53_FUNC_SHIFT     28

#define SD_ARG_CMD53_REG_SHIFT      9

#define SD_ARG_CMD53_LENGTH_SHIFT   0

#define SD_ARG_CMD53_INCREMENT      ((uint32_t)1<<26)

#define SD_ARG_CMD53_REG_MASK       0x1ffff
#define SD_ARG_CMD53_LENGTH_MASK    0x1ff

#define MMC_STOP_TRANSMISSION           12      /* R1B */
#define SD_ARG_CMD53_FUNC_MASK      0x7

#define SDSPI_BLOCK_BUF_SIZE    (512 + 4)

#define ESP_ERR_NOT_FINISHED    0x201

#define BIT(x)  (uint32_t)1<<(x)

// #define assert(x)   do { if (!(x)) { printf("SDIO SPI error %s %u\r\n", __FILE__, __LINE__); while(1);}} while (0)

/**
 * SD/MMC command response buffer
 */
typedef uint32_t spi_cmd_response_t[4];

/**
 * SD/MMC command information
 */
typedef struct {
        uint32_t opcode;            /*!< SD or MMC command index */
        uint32_t arg;               /*!< SD/MMC command argument */
        spi_cmd_response_t response;  /*!< response buffer */
        void* data;                 /*!< buffer to send or read into */
        size_t datalen;             /*!< length of data buffer */
        size_t blklen;              /*!< block length */
        uint32_t flags;                  /*!< see below */
/** @cond */

#define SCF_CMD(flags)   ((flags) & 0x00f0)
#define SCF_CMD_AC       0x0000
#define SCF_CMD_ADTC     0x0010
#define SCF_CMD_BC       0x0020
#define SCF_CMD_BCR      0x0030
#define SCF_CMD_READ     0x0040     /*!< read command (data expected) */
//#define SCF_RSP_BSY      0x0100
//#define SCF_RSP_136      0x0200
#define SCF_RSP_CRC      0x0400
#define SCF_RSP_IDX      0x0800
#define SCF_RSP_PRESENT  0x1000
/* response types */
#define SCF_RSP_R0       0 /*!< none */
#define SCF_RSP_R1       (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)
//#define SCF_RSP_R1B      (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX|SCF_RSP_BSY)
#define SCF_RSP_R4       (SCF_RSP_PRESENT)
#define SCF_RSP_R5       (SCF_RSP_PRESENT|SCF_RSP_CRC|SCF_RSP_IDX)

#define SD_IO_CCCR_FN_ENABLE        0x02
#define SD_IO_CCCR_INT_ENABLE       0x04
#define SD_IO_CCCR_BUS_WIDTH        0x07

#define  CCCR_BUS_WIDTH_ECSI        (1<<5)

#define SD_IO_CCCR_BLKSIZEL         0x10
#define SD_IO_CCCR_BLKSIZEH         0x11

#define SD_IO_CCCR_CTL              0x06
#define  CCCR_CTL_RES               (1<<3)

/** @endcond */
        int8_t error;            /*!< error returned from transfer */
        uint32_t timeout_ms;             /*!< response timeout, in milliseconds */
} spi_command_t;

typedef struct {
    uint16_t        buffer_size;
                        ///< All data that do not fully fill a buffer is still counted as one buffer. E.g. 10 bytes data costs 2 buffers if the size is 8 bytes per buffer.
                        ///< Buffer size of the slave pre-defined between host and slave before communication.
    uint16_t        block_size;
                        ///< If this is too large, it takes time to send stuff bits; while if too small, intervals between blocks cost much.
                        ///< Should be set according to length of data, and larger than ``TRANS_LEN_MAX/511``.
                        ///< Block size of the SDIO function 1. After the initialization this will hold the value the slave really do. Valid value is 1-2048.
    size_t          tx_sent_buffers;    ///< Counter hold the amount of buffers already sent to ESP32 slave. Should be set to 0 when initialization.
    size_t          rx_got_bytes;       ///< Counter hold the amount of bytes already received from ESP32 slave. Should be set to 0 when initialization.
} spi_context_t;

#define DR_REG_SLCHOST_BASE                     0x3ff55000
#define HOST_SLC0HOST_TOKEN_RDATA_REG          (DR_REG_SLCHOST_BASE + 0x44)
#define HOST_SLC0HOST_INT_CLR_REG          (DR_REG_SLCHOST_BASE + 0xD4)
#define HOST_SLC0HOST_INT_RAW_REG          (DR_REG_SLCHOST_BASE + 0x50)
#define HOST_SLC0HOST_INT_ST_REG           (DR_REG_SLCHOST_BASE + 0x58)
#define HOST_SLCHOST_PKT_LEN_REG          (DR_REG_SLCHOST_BASE + 0x60)

#define HOST_SLC0_RX_NEW_PACKET_INT_ST  (BIT(23))

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t at_sdspi_get_packet(spi_context_t* context, void* out_data, size_t size, size_t *out_length);

esp_err_t at_sdspi_send_packet(spi_context_t* context, const void* start, size_t length, uint32_t wait_ms);

esp_err_t at_sdspi_clear_intr(uint32_t intr_mask);

//esp_err_t at_sdspi_get_intr(uint32_t *intr_raw, uint32_t *intr_st);
esp_err_t at_sdspi_get_intr(uint32_t *intr_raw);

esp_err_t at_sdspi_init(void);

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_ID == PLATFORM_TRACKER

#endif // SDSPI_HOST_H


