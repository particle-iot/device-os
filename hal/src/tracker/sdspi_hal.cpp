/*
 * Copyright (c) 2020 Particle Industries, Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "interrupts_hal.h"
#include "sdspi_hal.h"
#include "gpio_hal.h"
#include "delay_hal.h"
#include "logging.h"
#include "system_error.h"
#include "service_debug.h"
#include <algorithm>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <memory>
#include "check.h"
#include "timer_hal.h"

namespace {

// currently ESP32 driver don't support block size other than 512
#define ESP32_SDIO_BLOCK_SIZE           512
#define ESP32_SDIO_BUF_SIZE             512

#define TX_BUFFER_MAX                   0x1000
#define TX_BUFFER_MASK                  0xFFF
#define RX_BYTE_MAX                     0x100000
#define RX_BYTE_MASK                    0xFFFFF

const uint32_t SDSPI_MAX_DATA_LEN = 512;  // Max size of single block transfer

// Port layer of sdspi driver
os_semaphore_t spiLock = NULL;

HAL_SPI_Interface spi_ = HAL_SPI_INTERFACE2;
hal_spi_info_t spi_info_cache_;

// SPIClass* gSpi = &SPI1;
pin_t csPin_ = WIFI_CS;
pin_t intPin = WIFI_INT;

const hal_spi_info_t SDSPI_DEFAULT_CONFIG = {
    .version = HAL_SPI_INFO_VERSION_2,
    .system_clock = 0,
    .default_settings = 0,
    .enabled = true,
    .mode = SPI_MODE_MASTER,
    .clock = 8000000,  // 8 * MHZ
    .bit_order = MSBFIRST,
    .data_mode = SPI_MODE0,
    .ss_pin = PIN_INVALID
};

inline uint8_t calculateClockDivider (uint32_t system_clock, uint32_t clock) {
    uint8_t result;

    // Integer division results in clean values
    switch ((clock > 0) ? (system_clock / clock) : 0) {
    case 2:
        result = SPI_CLOCK_DIV2;
        break;
    case 4:
        result = SPI_CLOCK_DIV4;
        break;
    case 8:
        result = SPI_CLOCK_DIV8;
        break;
    case 16:
        result = SPI_CLOCK_DIV16;
        break;
    case 32:
        result = SPI_CLOCK_DIV32;
        break;
    case 64:
        result = SPI_CLOCK_DIV64;
        break;
    case 128:
        result = SPI_CLOCK_DIV128;
        break;
    case 256:
    default:
        result = SPI_CLOCK_DIV256;
        break;
    }

    return result;
}

inline hal_spi_info_t __attribute__ ((always_inline)) spiConfigure(HAL_SPI_Interface spi, const hal_spi_info_t* conf) {
    hal_spi_info_t info = { .version = HAL_SPI_INFO_VERSION_2 };
    HAL_SPI_Info(spi, &info, nullptr);

    SPARK_ASSERT(info.mode == SPI_MODE_MASTER);

    if (!info.enabled || info.ss_pin != conf->ss_pin) {
        HAL_SPI_Begin_Ext(spi, SPI_MODE_MASTER, PIN_INVALID, nullptr);
    }

    if (conf->default_settings != info.default_settings ||
            conf->bit_order != info.bit_order ||
            conf->data_mode != info.data_mode ||
            conf->clock != info.clock) {
        HAL_SPI_Set_Settings(spi, conf->default_settings, calculateClockDivider(info.system_clock, conf->clock), conf->bit_order, conf->data_mode, nullptr);
    }

    return info;
}

// Set CS low
void at_cs_low() {
    HAL_SPI_Acquire(spi_, nullptr);
    spi_info_cache_ = spiConfigure(spi_, &SDSPI_DEFAULT_CONFIG);
    HAL_GPIO_Write(csPin_, 0);
}

// Set CS high
void at_cs_high() {
    HAL_GPIO_Write(csPin_, 1);
    spiConfigure(spi_, &spi_info_cache_);
    HAL_SPI_Release(spi_, nullptr);
}

esp_err_t at_spi_transmit(void* tx_buff, void* rx_buff, uint32_t len) {
    // HAL_SPI_DMA_Transfer(spi_, tx_buff, rx_buff, len, NULL);
    // SPI1.transfer(tx_buff, rx_buff, len, NULL);
    HAL_SPI_DMA_Transfer(spi_, tx_buff, rx_buff, len, nullptr);
    HAL_SPI_TransferStatus st;
    do
    {
        HAL_SPI_DMA_Transfer_Status(spi_, &st);
    } while (st.transfer_ongoing);

    // LOG(INFO, "at_spi_transmit: %d", len);
    return SYSTEM_ERROR_NONE;
}

esp_err_t at_spi_slot_init() {
    // Initialize gSpi bus
    // For SD cards, CS must stay low during the whole read/write operation,
    // rather than a single SPI transaction.
    HAL_SPI_Init(spi_);
    HAL_SPI_Acquire(spi_, nullptr);
    HAL_SPI_Begin_Ext(spi_, SPI_MODE_MASTER, SPI_DEFAULT_SS, nullptr);
    HAL_SPI_Release(spi_, nullptr);

    // Configure CS pin
    HAL_Pin_Mode(csPin_, OUTPUT);
    HAL_GPIO_Write(csPin_, 1);

    HAL_Pin_Mode(intPin, INPUT_PULLUP);

    // NOTE: Don't enable interrupt here on ATSoM
    // The unexpected interrupt casuses a SPI read on IO Expander,
    // which interferes with initialization sequence of SDIO,
    // attach the interrupt after initialization sequence.

    return SYSTEM_ERROR_NONE;
}

esp_err_t at_spi_wait_int(uint32_t wait_ms) {
    system_tick_t startTime = hal_timer_millis(nullptr);
    do {
        if (HAL_GPIO_Read(WIFI_INT) == 0) {
            return SYSTEM_ERROR_NONE;
        }
    } while (hal_timer_millis(nullptr) - startTime < wait_ms);

    return SYSTEM_ERROR_TIMEOUT;
}

void at_do_delay(uint32_t wait_ms) {
    HAL_Delay_Milliseconds(wait_ms);
}

os_semaphore_t at_mutex_init() {
    os_mutex_t mutex = NULL;
    SPARK_ASSERT(os_mutex_create(&mutex) == 0);
    return mutex;
}

void at_mutex_lock(os_semaphore_t pxMutex) {
    os_mutex_lock(pxMutex);
}

void at_mutex_unlock(os_semaphore_t pxMutex) {
    os_mutex_unlock(pxMutex);
}

// void at_mutex_free(os_semaphore_t pxMutex) {
//     os_mutex_destroy(pxMutex);
//     pxMutex = NULL;
// }

} // Anonymous

static void make_hw_cmd(uint32_t opcode, uint32_t arg, sdspi_hw_cmd_t* hw_cmd) {
    hw_cmd->start_bit = 0;
    hw_cmd->transmission_bit = 1;
    hw_cmd->cmd_index = opcode;
    hw_cmd->stop_bit = 1;
    hw_cmd->r1 = 0xff;
    memset(hw_cmd->response, 0xff, sizeof(hw_cmd->response));
    hw_cmd->ncr = 0xff;
    uint32_t arg_s = __builtin_bswap32(arg);
    memcpy(hw_cmd->arguments, &arg_s, sizeof(arg_s));
    hw_cmd->crc7 = 0x0;
}

// Clock out 80 cycles (10 bytes) before GO_IDLE command
static void go_idle_clockout() {
    uint8_t data[10];
    memset(data, 0xFF, sizeof(data));
    int ret = at_spi_transmit(data, data, 10);
    if (ret != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "SPI transmit error\n");
    }
}

// the r1 respond could appear 1-8 clocks after the command token is sent
// this function search buffer after 1 clocks to max 8 clocks for r1
// and shift the data after R1, to 1 clocks after the command to match the definition of sdspi_hw_cmd_t.
static int shift_cmd_response(sdspi_hw_cmd_t* cmd, int sent_bytes) {
    uint8_t* pr1 = &cmd->r1;
    int ncr_cnt = 1;
    for (;;) {
        if ((*pr1 & SD_SPI_R1_NO_RESPONSE) == 0) {
            break;
        }

        pr1++;
        if (++ncr_cnt > 8) {
            // LOG(ERROR, "Not found response");
            return SYSTEM_ERROR_NOT_FOUND;
        }
    }

    int copy_bytes = sent_bytes - SDSPI_CMD_SIZE - ncr_cnt;
    if (copy_bytes > 0) {
        memcpy(&cmd->r1, pr1, copy_bytes);
    }

    return SYSTEM_ERROR_NONE;
}

static int start_command_default(int flags, sdspi_hw_cmd_t* cmd) {
    size_t cmd_size = SDSPI_CMD_R1_SIZE;
    if ((flags & SDSPI_CMD_FLAG_RSP_R1) || (flags & SDSPI_CMD_FLAG_NORSP)) {
        cmd_size = SDSPI_CMD_R1_SIZE;
    } else if (flags & SDSPI_CMD_FLAG_RSP_R4) {
        cmd_size = SDSPI_CMD_R4_SIZE;
    } else if (flags & SDSPI_CMD_FLAG_RSP_R5) {
        cmd_size = SDSPI_CMD_R5_SIZE;
    }

    // add extra clocks to avoid polling
    cmd_size += (SDSPI_NCR_MAX_SIZE - SDSPI_NCR_MIN_SIZE);

    int ret = at_spi_transmit(cmd, cmd, cmd_size);
    if (ret != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "%s: spi_device_transmit returned 0x%x", __func__, ret);
        return ret;
    }

    if (flags & SDSPI_CMD_FLAG_NORSP) {
        /* no (correct) response expected from the card, so skip polling loop */
        // LOG(INFO, "%s: ignoring response byte", __func__);
        cmd->r1 = 0x00;
    }

    ret = shift_cmd_response(cmd, cmd_size);
    if (ret != SYSTEM_ERROR_NONE) {
        return SYSTEM_ERROR_TIMEOUT;
    }

    return SYSTEM_ERROR_NONE;
}

// Wait until MISO goes high
static int poll_busy() {
    int ret;
    uint8_t t_rx, t_tx;
    uint32_t nonzero_count = 0;

    do {
        t_tx = SDSPI_MOSI_IDLE_VAL;
        t_rx = 0;
        ret = at_spi_transmit(&t_tx, &t_rx, 1);
        if (ret != SYSTEM_ERROR_NONE) {
            return ret;
        }

        if (t_rx == 0XFF) {
            return SYSTEM_ERROR_NONE;
        }

    } while (++nonzero_count < UINT32_MAX);

    LOG(ERROR, "%s: timeout error", __func__);

    return SYSTEM_ERROR_NONE;
}

// For CMD53, we can send in byte mode, or block mode
// The data start token is different, and cannot be determined by the length
// That's why we need ``multi_block``.
static int start_command_write_blocks(sdspi_hw_cmd_t* cmd, uint8_t* data, uint32_t tx_length, bool multi_block) {
    int ret;
    // Send the minimum length that is sure to get the complete response
    // SD cards always return R1 (1bytes), SDIO returns R5 (2 bytes)
    int send_bytes = SDSPI_CMD_R5_SIZE + SDSPI_NCR_MAX_SIZE - SDSPI_NCR_MIN_SIZE;

    at_spi_transmit(cmd, cmd, send_bytes);
    // check for command response validate
    ret = shift_cmd_response(cmd, send_bytes);

    if (ret != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "%s: check_cmd_response returned 0x%x", __func__, ret);
        return ret;
    }

    uint8_t start_token = // tx_length <= SDSPI_MAX_DATA_LEN ?
        (!multi_block) ? TOKEN_BLOCK_START : TOKEN_BLOCK_START_WRITE_MULTI;

    while (tx_length > 0) {
        // LOG(TRACE, "start_command_write_blocks: Remained len:%d", tx_length);
        // Write block start token
        ret = at_spi_transmit(&start_token, NULL, sizeof(start_token));

        if (ret != SYSTEM_ERROR_NONE) {
            return ret;
        }

        // Prepare data to be sent
        size_t will_send = std::min(tx_length, SDSPI_MAX_DATA_LEN);

        // Write data
        ret = at_spi_transmit(data, NULL, will_send);

        if (ret != SYSTEM_ERROR_NONE) {
            return ret;
        }

        uint8_t crc_tx[4];
        uint8_t crc_rx[4];

        memset(crc_tx, 0xff, 4);
        ret = at_spi_transmit(crc_tx, crc_rx, 3);
        if (ret != SYSTEM_ERROR_NONE) {
            return ret;
        }

        // LOG(TRACE, "crc_rx[2] = %x\n", crc_rx[2]);
        // CHECK_TRUE((crc_rx[2] & 0xf) == 5);

        // Wait for the card to finish writing data
        ret = poll_busy();

        if (ret != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "poll_busy returned 0x%x", ret);
            return ret;
        }

        tx_length -= will_send;
        data += will_send;
    }

    return SYSTEM_ERROR_NONE;
}

// Wait for data token, reading 8 bytes at a time.
// If the token is found, write all subsequent bytes to extra_ptr,
// and store the number of bytes written to extra_size.
static int poll_data_token(uint8_t* extra_ptr, size_t* extra_size) {
    uint8_t t_rx[8];
    int ret;
    uint8_t count_time = 0;

    do {
        memset(t_rx, SDSPI_MOSI_IDLE_VAL, sizeof(t_rx));
        ret = at_spi_transmit(t_rx, t_rx, sizeof(t_rx));

        if (ret != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "SPI trans error %d", ret);
            return ret;
        }

        bool found = false;

        for (uint8_t byte_idx = 0; byte_idx < sizeof(t_rx); byte_idx++) {
            uint8_t rd_data = t_rx[byte_idx];

            if (rd_data == TOKEN_BLOCK_START) {
                found = true;
                memcpy(extra_ptr, t_rx + byte_idx + 1, sizeof(t_rx) - byte_idx - 1);
                *extra_size = sizeof(t_rx) - byte_idx - 1;
                break;
            }

            if ((rd_data != 0xff) && (rd_data != 0)) {
                LOG(ERROR, "%s: received 0x%02x while waiting for data", __func__, rd_data);
                return SYSTEM_ERROR_BAD_DATA;
            }
        }

        if (found) {
            return SYSTEM_ERROR_NONE;
        }

    } while (++count_time < 1000);

    LOG(ERROR, "%s: timeout", __func__);
    return SYSTEM_ERROR_TIMEOUT;
}

/**
 * Receiving one or more blocks of data happens as follows:
 * 1. send command + receive r1 response (SDSPI_CMD_R1_SIZE bytes total)
 * 2. keep receiving bytes until TOKEN_BLOCK_START is encountered (this may
 *    take a while, depending on card's read speed)
 * 3. receive up to SDSPI_MAX_DATA_LEN = 512 bytes of actual data
 * 4. receive 2 bytes of CRC
 * 5. for multi block transfers, go to step 2
 *
 * These steps can be done separately, but that leads to a less than optimal
 * performance on large transfers because of delays between each step.
 * For example, if steps 3 and 4 are separate SPI transactions queued one after
 * another, there will be ~16 microseconds of dead time between end of step 3
 * and the beginning of step 4. A delay between two blocking SPI transactions
 * in step 2 is even higher (~60 microseconds).
 *
 * To improve read performance the following sequence is adopted:
 * 1. Do the first transfer: command + r1 response + 8 extra bytes.
 *    Set pre_scan_data_ptr to point to the 8 extra bytes, and set
 *    pre_scan_data_size to 8.
 * 2. Search pre_scan_data_size bytes for TOKEN_BLOCK_START.
 *    If found, the rest of the bytes contain part of the actual data.
 *    Store pointer to and size of that extra data as extra_data_{ptr,size}.
 *    If not found, fall back to polling for TOKEN_BLOCK_START, 8 bytes at a
 *    time (in poll_data_token function). Deal with extra data in the same way,
 *    by setting extra_data_{ptr,size}.
 * 3. Receive the remaining 512 - extra_data_size bytes, plus 4 extra bytes
 *    (i.e. 516 - extra_data_size). Of the 4 extra bytes, first two will capture
 *    the CRC value, and the other two will capture 0xff 0xfe sequence
 *    indicating the start of the next block. Actual scanning is done by
 *    setting pre_scan_data_ptr to point to these last 2 bytes, and setting
 *    pre_scan_data_size = 2, then going to step 2 to receive the next block.
 *    When the final block is being received, the number of extra bytes is 2
 *    (only for CRC), because we don't need to wait for start token of the
 *    next block, and some cards are getting confused by these two extra bytes.
 *
 * With this approach the delay between blocks of a multi-block transfer is
 * ~95 microseconds, out of which 35 microseconds are spend doing the CRC check.
 * Further speedup is possible by pipelining transfers and CRC checks, at an
 * expense of one extra temporary buffer.
 */
static int start_command_read_blocks(sdspi_hw_cmd_t* cmd, uint8_t* data, uint32_t rx_length) {
    int ret;
    std::unique_ptr<uint8_t[]> buf(new(std::nothrow) uint8_t[SDSPI_BLOCK_BUF_SIZE]);
    if (!buf) {
        LOG(ERROR, "No memory for rx data!");
        return SYSTEM_ERROR_NO_MEMORY;
    }

    uint8_t* rx_data = buf.get();

    bool need_stop_command = rx_length > SDSPI_MAX_DATA_LEN;

    ret = at_spi_transmit(cmd, cmd, (SDSPI_CMD_R1_SIZE + SDSPI_RESPONSE_MAX_DELAY));

    if (ret != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "SPI transmit error");
        return ret;
    }

    uint8_t* cmd_u8 = (uint8_t*)cmd;
    size_t pre_scan_data_size = SDSPI_RESPONSE_MAX_DELAY;
    uint8_t* pre_scan_data_ptr = cmd_u8 + SDSPI_CMD_R1_SIZE;

    /* R1 response is delayed by 1-8 bytes from the request.
     * This loop searches for the response and writes it to cmd->r1.
     */
    while ((cmd->r1 & SD_SPI_R1_NO_RESPONSE) != 0 && pre_scan_data_size > 0) {
        cmd->r1 = *pre_scan_data_ptr;
        ++pre_scan_data_ptr;
        --pre_scan_data_size;
    }

    if (cmd->r1 & SD_SPI_R1_NO_RESPONSE) {
        LOG(ERROR, "no response token found");
        return SYSTEM_ERROR_TIMEOUT;
    }

    while (rx_length > 0) {
        size_t extra_data_size = 0;
        const uint8_t* extra_data_ptr = NULL;
        bool need_poll = true;

        for (size_t i = 0; i < pre_scan_data_size; ++i) {
            if (pre_scan_data_ptr[i] == TOKEN_BLOCK_START) {
                extra_data_size = pre_scan_data_size - i - 1;
                extra_data_ptr = pre_scan_data_ptr + i + 1;
                need_poll = false;
                break;
            }
        }

        if (need_poll) {
            // Wait for data to be ready
            ret = poll_data_token(cmd_u8 + SDSPI_CMD_R1_SIZE, &extra_data_size);

            // release_transaction(slot);
            if (ret != SYSTEM_ERROR_NONE) {
                LOG(ERROR, "poll_data_token return %d", ret);
                return ret;
            }

            if (extra_data_size) {
                extra_data_ptr = cmd_u8 + SDSPI_CMD_R1_SIZE;
            }
        }

        // Arrange RX buffer
        size_t will_receive = std::min(rx_length, SDSPI_MAX_DATA_LEN) - extra_data_size;

        // receive actual data
        const size_t receive_extra_bytes = (rx_length > SDSPI_MAX_DATA_LEN) ? 4 : 2;
        memset(rx_data, 0xff, will_receive + receive_extra_bytes);

        ret = at_spi_transmit(rx_data, rx_data, (will_receive + receive_extra_bytes));

        if (ret != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "SPI transmit error");
            return ret;
        }

        // CRC bytes need to be received even if CRC is not enabled
        uint16_t crc = UINT16_MAX;
        memcpy(&crc, rx_data + will_receive, sizeof(crc));

        // Bytes to scan for the start token
        pre_scan_data_size = receive_extra_bytes - sizeof(crc);
        pre_scan_data_ptr = rx_data + will_receive + sizeof(crc);

        // Copy data to the destination buffer
        memcpy(data + extra_data_size, rx_data, will_receive);

        if (extra_data_size) {
            memcpy(data, extra_data_ptr, extra_data_size);
        }

        data += will_receive + extra_data_size;
        rx_length -= will_receive + extra_data_size;
        extra_data_size = 0;
        extra_data_ptr = NULL;
    }

    if (need_stop_command) {
        // To end multi block transfer, send stop command and wait for the
        // card to process it
        sdspi_hw_cmd_t stop_cmd;
        make_hw_cmd(MMC_STOP_TRANSMISSION, 0, &stop_cmd);
        ret = start_command_default(SDSPI_CMD_FLAG_RSP_R1, &stop_cmd);

        if (ret != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "%s: start_command_default error", __func__);
            return ret;
        }

        ret = poll_busy();

        if (ret != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "%s: poll_busy error", __func__);
            return ret;
        }
    }

    return SYSTEM_ERROR_NONE;
}

int sdspi_host_start_command(sdspi_hw_cmd_t* cmd, void* data, uint32_t data_size, int flags) {
    int ret = SYSTEM_ERROR_NONE;
    // char tx_byte = 0xff;

    at_mutex_lock(spiLock);
    at_cs_low();

    if (flags & SDSPI_CMD_FLAG_DATA) {
        if (flags & SDSPI_CMD_FLAG_WRITE) {
            ret = start_command_write_blocks(cmd, (uint8_t*)data, data_size, flags & SDSPI_CMD_FLAG_MULTI_BLK);
        } else {
            ret = start_command_read_blocks(cmd, (uint8_t*)data, data_size);

            if (ret != SYSTEM_ERROR_NONE) {
                LOG(ERROR, "read blocks error");
            }
        }
    } else {
        ret = start_command_default(flags, cmd);
    }

    at_cs_high();
    at_mutex_unlock(spiLock);

    return ret;
}

int spi_send_cmd(spi_command_t* cmdinfo) {
    if (spiLock == NULL) {
        spiLock = at_mutex_init();
    }

    WORD_ALIGNED_ATTR sdspi_hw_cmd_t hw_cmd;
    make_hw_cmd(cmdinfo->opcode, cmdinfo->arg, &hw_cmd);

    if (cmdinfo->opcode == 0) {
        // LOG(TRACE, "CMD0");
        hw_cmd.crc7 = 0x4a;
    }

    // Flags indicate which of the transfer types should be used
    int flags = 0;

    if (SCF_CMD(cmdinfo->flags) == SCF_CMD_ADTC) {
        flags = SDSPI_CMD_FLAG_DATA | SDSPI_CMD_FLAG_WRITE;
    } else if (SCF_CMD(cmdinfo->flags) == (SCF_CMD_ADTC | SCF_CMD_READ)) {
        flags = SDSPI_CMD_FLAG_DATA;
    }

    // The max block size is 512, when larger than 512, the data must send in multi blocks
    if (cmdinfo->datalen > SDSPI_MAX_DATA_LEN) {
        flags |= SDSPI_CMD_FLAG_MULTI_BLK;
    }

    // In fact, most of the commands
    // use R1 response. Therefore, instead of adding another parallel set of
    // response flags for the SPI mode, response format is determined here:
    if (cmdinfo->opcode == MMC_GO_IDLE_STATE && !(cmdinfo->flags & SCF_RSP_R1)) {
        /* used to send CMD0 without expecting a response */
        flags |= SDSPI_CMD_FLAG_NORSP;
    } else if (cmdinfo->opcode == SD_IO_SEND_OP_COND) {
        flags |= SDSPI_CMD_FLAG_RSP_R4;
    } else if (cmdinfo->opcode == SD_IO_RW_DIRECT) {
        flags |= SDSPI_CMD_FLAG_RSP_R5;
    } else if (cmdinfo->opcode == SD_IO_RW_EXTENDED) {
        flags |= SDSPI_CMD_FLAG_RSP_R5 | SDSPI_CMD_FLAG_DATA;

        if (cmdinfo->arg & SD_ARG_CMD53_WRITE) {
            flags |= SDSPI_CMD_FLAG_WRITE;
        }

        // The CMD53 can assign block mode in the arg when the length is exactly 512 bytes
        if (cmdinfo->arg & SD_ARG_CMD53_BLOCK_MODE) {
            flags |= SDSPI_CMD_FLAG_MULTI_BLK;
        }
    } else {
        flags |= SDSPI_CMD_FLAG_RSP_R1;
    }

    // Send the command and get the response.
    int ret = sdspi_host_start_command(&hw_cmd, cmdinfo->data, cmdinfo->datalen, flags);

    // Extract response bytes and store them into cmdinfo structure
    if (ret == SYSTEM_ERROR_NONE) {
        // LOG(TRACE, "r1 = 0x%02x hw_cmd.r[0]=0x%08x", hw_cmd.r1, hw_cmd.response[0]);

        // Some errors should be reported using return code
        if (flags & SDSPI_CMD_FLAG_RSP_R1) {
            cmdinfo->response[0] = hw_cmd.r1;
        } else if (flags & SDSPI_CMD_FLAG_RSP_R5) {
            cmdinfo->response[0] = hw_cmd.response[0];
        } else if (flags & SDSPI_CMD_FLAG_RSP_R4) {
            cmdinfo->response[0] = 0xff;
        }
    }

    return ret;
}

int at_spi_init_io() {
    // IO_SEND_OP_COND(CMD5), Determine if the card is an IO card.
    // Non-IO cards will not respond to this command.
    uint32_t ocr = MMC_OCR_3_3V_3_4V;
    spi_command_t cmd = {};
    cmd.flags = SCF_CMD_BCR | SCF_RSP_R4;
    cmd.arg = ocr;
    cmd.opcode = SD_IO_SEND_OP_COND;

    int err = spi_send_cmd(&cmd);
    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "%s: spi_send_cmd error", __func__);
        return err;
    }

    at_do_delay(20);

    err = spi_send_cmd(&cmd);
    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "%s: spi_send_cmd error", __func__);
        return err;
    }

    return err;
}

int at_spi_cmd_go_idle_state() {
    go_idle_clockout();

    spi_command_t cmd = {};
    cmd.opcode = 0; // MMC_GO_IDLE_STATE,
    cmd.flags = SCF_CMD_BC | SCF_RSP_R0;

    int err = spi_send_cmd(&cmd);
    at_do_delay(20);

    cmd.flags |= SCF_RSP_R1;
    err = spi_send_cmd(&cmd);
    at_do_delay(20);

    // Response 0x1 means enter into SPI mode
    if (cmd.response[0] != 0x1) {
        LOG(ERROR, "CMD0 response error, expect 0x1, response %x", cmd.response[0]);
        return SYSTEM_ERROR_BAD_DATA;
    }

    return err;
}

int at_spi_io_rw_direct(int func, uint32_t reg, uint32_t arg, uint8_t* byte) {
    int err;
    spi_command_t cmd = {};
    cmd.flags = SCF_CMD_AC | SCF_RSP_R5;
    cmd.arg = 0;
    cmd.opcode = SD_IO_RW_DIRECT;

    arg |= (func & SD_ARG_CMD52_FUNC_MASK) << SD_ARG_CMD52_FUNC_SHIFT;
    arg |= (reg & SD_ARG_CMD52_REG_MASK) << SD_ARG_CMD52_REG_SHIFT;
    arg |= (*byte & SD_ARG_CMD52_DATA_MASK) << SD_ARG_CMD52_DATA_SHIFT;
    cmd.arg = arg;

    err = spi_send_cmd(&cmd);
    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "%s: sdmmc_send_cmd returned 0x%x", __func__, err);
        return err;
    }

    *byte = SD_R5_DATA(cmd.response);

    return SYSTEM_ERROR_NONE;
}

int spi_io_rw_extended(int func, uint32_t reg, int arg, void* datap, size_t datalen) {
    int err;
    const size_t max_byte_transfer_size = 512;
    spi_command_t cmd = {};
    cmd.flags = SCF_CMD_AC | SCF_RSP_R5;
    cmd.arg = 0;
    cmd.opcode = SD_IO_RW_EXTENDED;
    cmd.data = datap;
    cmd.datalen = datalen;
    cmd.blklen = max_byte_transfer_size; /* TODO: read max block size from CIS */

    uint32_t count; /* number of bytes or blocks, depending on transfer mode */

    if (arg & SD_ARG_CMD53_BLOCK_MODE) {
        if (cmd.datalen % cmd.blklen != 0) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        count = cmd.datalen / cmd.blklen;
    } else {
        if (datalen > max_byte_transfer_size) {
            /* TODO: split into multiple operations? */
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }

        if (datalen == max_byte_transfer_size) {
            count = 0; // See 5.3.1 SDIO simplifed spec
        } else {
            count = datalen;
        }

        cmd.blklen = datalen;
    }

    arg |= (func & SD_ARG_CMD53_FUNC_MASK) << SD_ARG_CMD53_FUNC_SHIFT;
    arg |= (reg & SD_ARG_CMD53_REG_MASK) << SD_ARG_CMD53_REG_SHIFT;
    arg |= (count & SD_ARG_CMD53_LENGTH_MASK) << SD_ARG_CMD53_LENGTH_SHIFT;
    cmd.arg = arg;

    if ((arg & SD_ARG_CMD53_WRITE) == 0) {
        cmd.flags |= SCF_CMD_READ;
    }

    err = spi_send_cmd(&cmd);

    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "%s: sdmmc_send_cmd returned 0x%x", __func__, err);
        return err;
    }

    return SYSTEM_ERROR_NONE;
}

int spi_io_read_bytes(uint32_t function, uint32_t addr, void* dst, size_t size) {
    /* host quirk: SDIO transfer with length not divisible by 4 bytes
     * has to be split into two transfers: one with aligned length,
     * the other one for the remaining 1-3 bytes.
     */

#ifdef FOUR_BYTE_ALIGNMENT
    uint8_t* pc_dst = dst;

    while (size > 0) {
        size_t size_aligned = size & (~3);
        size_t will_transfer = size_aligned > 0 ? size_aligned : size;
        LOG(TRACE, "%s, size: %d, will_transfer:%d\n", __func__, size, will_transfer);
        int err = spi_io_rw_extended(function, addr, SD_ARG_CMD53_READ | SD_ARG_CMD53_INCREMENT, pc_dst, will_transfer);

        if (err != SYSTEM_ERROR_NONE) {
            return err;
        }

        pc_dst += will_transfer;
        size -= will_transfer;
        addr += will_transfer;
    }

#else
    // LOG(TRACE, "%s, will transfer size: %d\n", __func__, size);
    int err = spi_io_rw_extended(function, addr, SD_ARG_CMD53_READ | SD_ARG_CMD53_INCREMENT, dst, size);
    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "Read bytes spi_io_rw_extended return %d", err);
        return err;
    }

#endif
    return SYSTEM_ERROR_NONE;
}

int spi_io_write_bytes(uint32_t function, uint32_t addr, void* src, size_t size) {
    /* same host quirk as in sdmmc_io_read_bytes */
#ifdef FOUR_BYTE_ALIGNMENT
    const uint8_t* pc_src = (const uint8_t*)src;

    while (size > 0) {
        size_t size_aligned = size & (~3);
        size_t will_transfer = size_aligned > 0 ? size_aligned : size;

        int err = spi_io_rw_extended(function, addr, SD_ARG_CMD53_WRITE | SD_ARG_CMD53_INCREMENT, (void*)pc_src, will_transfer);

        if (err != SYSTEM_ERROR_NONE) {
            return err;
        }

        pc_src += will_transfer;
        size -= will_transfer;
        addr += will_transfer;
    }

#else
    // LOG(TRACE, "%s, will transfer size: %d\n", __func__, size);
    int err = spi_io_rw_extended(function, addr, SD_ARG_CMD53_WRITE | SD_ARG_CMD53_INCREMENT, src, size);

    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "Write bytes spi_io_rw_extended return %d", err);
        return err;
    }

#endif
    return SYSTEM_ERROR_NONE;
}

int spi_io_write_byte(uint32_t function, uint32_t addr, uint8_t in_byte, uint8_t* out_byte) {
    uint8_t tmp_byte = in_byte;
    int ret = at_spi_io_rw_direct(function, addr, SD_ARG_CMD52_WRITE | SD_ARG_CMD52_EXCHANGE, &tmp_byte);

    if (ret != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "%s: sdmmc_io_rw_direct (write 0x%x) returned 0x%x", __func__, addr, ret);
        return ret;
    }

    if (out_byte != NULL) {
        *out_byte = tmp_byte;
    }

    return SYSTEM_ERROR_NONE;
}

int spi_io_read_byte(uint32_t function, uint32_t addr, uint8_t* out_byte) {
    int ret = at_spi_io_rw_direct(function, addr, SD_ARG_CMD52_READ, out_byte);

    if (ret != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "%s: sdmmc_io_rw_direct (read 0x%x) returned 0x%x", __func__, addr, ret);
    }

    return ret;
}

static inline int esp_slave_write_bytes(uint32_t addr, uint8_t* val, int len) {
    return spi_io_write_bytes(1, addr & 0x3FF, val, len);
}

static inline int esp_slave_read_bytes(uint32_t addr, uint8_t* val_o, int len) {
    return spi_io_read_bytes(1, addr & 0x3FF, val_o, len);
}

int hal_sdspi_clear_intr(uint32_t intr_mask) {
    // LOG(TRACE, "clear_intr: %08X", intr_mask);
    return esp_slave_write_bytes(HOST_SLC0HOST_INT_CLR_REG, (uint8_t*)&intr_mask, 4);
}

// int hal_sdspi_get_intr(uint32_t* intr_raw, uint32_t* intr_st)
int hal_sdspi_get_intr(uint32_t* intr_raw) {
    int r;

    if (intr_raw == NULL) {
        LOG(ERROR, "intr_raw is NULL!");
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    if (intr_raw != NULL) {
        r = esp_slave_read_bytes(HOST_SLC0HOST_INT_RAW_REG, (uint8_t*)intr_raw, 4);

        if (r != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "esp_slave_read_bytes: %d", r);
            return r;
        }
    }

    return SYSTEM_ERROR_NONE;
}

static int esp_slave_get_rx_data_size(spi_context_t* context, uint32_t* rx_size) {
    uint32_t len = 0;
    int err;

    // LOG(TRACE, "get_rx_data_size: got_bytes: %d", context->rx_got_bytes);
    err = esp_slave_read_bytes(HOST_SLCHOST_PKT_LEN_REG, (uint8_t*)&len, 4);
    if (err != SYSTEM_ERROR_NONE) {
        return err;
    }

    len &= RX_BYTE_MASK;
    len = (len + RX_BYTE_MAX - context->rx_got_bytes) % RX_BYTE_MAX;
    *rx_size = len;
    return SYSTEM_ERROR_NONE;
}

static int sdmmc_io_read_blocks(uint32_t function, uint32_t addr, void* dst, size_t size) {
    if (size % 4 != 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    return spi_io_rw_extended(function, addr, SD_ARG_CMD53_READ | SD_ARG_CMD53_INCREMENT | SD_ARG_CMD53_BLOCK_MODE, dst, size);
}

static int sdmmc_io_write_blocks(uint32_t function, uint32_t addr, const void* src, size_t size) {
    if (size % 4 != 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    // LOG(TRACE, "Write blocks len: %d", size);

    return spi_io_rw_extended(function, addr, SD_ARG_CMD53_WRITE | SD_ARG_CMD53_INCREMENT | SD_ARG_CMD53_BLOCK_MODE, (void*)src, size);
}

int hal_sdspi_get_packet(spi_context_t* context, void* out_data, size_t size, size_t* out_length) {
    CHECK_TRUE(size > 0, SYSTEM_ERROR_INVALID_ARGUMENT);

    uint32_t len = hal_sdspi_available_for_read(context);
    CHECK_TRUE(len > 0, SYSTEM_ERROR_NOT_ENOUGH_DATA);
    CHECK_TRUE(size > len, SYSTEM_ERROR_LIMIT_EXCEEDED);

    int err = SYSTEM_ERROR_NONE;
    uint8_t* start = (uint8_t*)out_data;
    uint32_t len_remain = len;

    do {
        int len_to_send;

        int block_n = len_remain / ESP32_SDIO_BLOCK_SIZE;

        if (block_n != 0) {
            len_to_send = block_n * ESP32_SDIO_BLOCK_SIZE;
            err = sdmmc_io_read_blocks(1, ESP_SLAVE_CMD53_END_ADDR - len_remain, start, len_to_send);
        } else {
            len_to_send = len_remain;
            /* though the driver supports to split packet of unaligned size into length
             * of 4x and 1~3, we still get aligned size of data to get higher
             * effeciency. The length is determined by the SDIO address, and the
             * remainning will be ignored by the slave hardware.
             */
            err = spi_io_read_bytes(1, ESP_SLAVE_CMD53_END_ADDR - len_remain, start, (len_to_send + 3) & (~3));
        }

        if (err != SYSTEM_ERROR_NONE) {
            return err;
        }

        // TODO:  (len_to_send + 3) & (~3) 栈溢出
        start += len_to_send;
        len_remain -= len_to_send;
    } while (len_remain != 0);

    context->rx_got_bytes += len;
    *out_length = len;
    return err;
}

static int esp_slave_get_tx_buffer_num(spi_context_t* context, uint32_t* tx_num) {
    uint32_t len;
    int err;

    // LOG(TRACE, "get_tx_buffer_num");
    err = esp_slave_read_bytes(HOST_SLC0HOST_TOKEN_RDATA_REG, (uint8_t*)&len, 4);
    if (err != SYSTEM_ERROR_NONE) {
        return err;
    }

    len = (len >> 16) & TX_BUFFER_MASK;
    len = (len + TX_BUFFER_MAX - context->tx_sent_buffers) % TX_BUFFER_MAX;
    *tx_num = len;
    return SYSTEM_ERROR_NONE;
}

int hal_sdspi_send_packet(spi_context_t* context, const void* start, size_t length, uint32_t wait_ms) {
    int buffer_used = (length + ESP32_SDIO_BUF_SIZE - 1) / ESP32_SDIO_BUF_SIZE;
    int err;
    CHECK_TRUE(length > 0, SYSTEM_ERROR_INVALID_ARGUMENT);

    uint32_t num = 0;
    err = esp_slave_get_tx_buffer_num(context, &num);
    CHECK(err);
    if (num * ESP32_SDIO_BUF_SIZE < length) {
        LOG(ERROR, "buffer is not enough. request size: %d, available buffer num: %d, wait_ms: %d", buffer_used, num, wait_ms);
        return SYSTEM_ERROR_TOO_LARGE;
    }

    // LOG(TRACE, "esp32 available for send: %d, context->tx_sent_buffers: %d", num, context->tx_sent_buffers);
    // LOG(TRACE, "hal_sdspi_send_packet: len: %d, try time: %d", length, try_time);
    uint8_t* start_ptr = (uint8_t*)start;
    uint32_t len_remain = length;

    do {
        // const int ESP32_SDIO_BLOCK_SIZE = 512;
        /* Though the driver supports to split packet of unaligned size into
         * length of 4x and 1~3, we still send aligned size of data to get
         * higher effeciency. The length is determined by the SDIO address, and
         * the remainning will be discard by the slave hardware.
         */
        int len_to_send;

        if (len_remain > ESP32_SDIO_BLOCK_SIZE) {
            int block_n = len_remain / ESP32_SDIO_BLOCK_SIZE;
            len_to_send = block_n * ESP32_SDIO_BLOCK_SIZE;
            // LOG(INFO, "hal_sdspi_send_packet, sdmmc_io_write_blocks, addr: 0x%x, size: %d", ESP_SLAVE_CMD53_END_ADDR - len_remain, len_to_send);
            err = sdmmc_io_write_blocks(1, ESP_SLAVE_CMD53_END_ADDR - len_remain, start_ptr, len_to_send);
        } else {
            len_to_send = len_remain;
            // LOG(INFO, "hal_sdspi_send_packet, spi_io_write_bytes, addr: 0x%x, size: %d", ESP_SLAVE_CMD53_END_ADDR - len_remain, (len_to_send + 3) & (~3));
            err = spi_io_write_bytes(1, ESP_SLAVE_CMD53_END_ADDR - len_remain, start_ptr, (len_to_send + 3) & (~3));
        }

        if (err != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "Write data error %d\n", err);
            return err;
        }

        // FIXME: buffer overflow for read, (len_to_send + 3) & (~3)
        start_ptr += len_to_send;
        len_remain -= len_to_send;
    } while (len_remain);

    context->tx_sent_buffers += buffer_used;
    return SYSTEM_ERROR_NONE;
}

size_t hal_sdspi_available_for_read(spi_context_t* context) {
    uint32_t len = 0;
    int err = esp_slave_get_rx_data_size(context, &len);
    if (err) {
        LOG(ERROR, "esp_slave_get_rx_data_size: %d", err);
    }
    return len;
}

size_t hal_sdspi_available_for_write(spi_context_t* context) {
    uint32_t bufCount = 0;
    int err = esp_slave_get_tx_buffer_num(context, &bufCount);
    if (err) {
        LOG(ERROR, "esp_slave_get_tx_buffer_num: %d", err);
    }
    return bufCount * ESP32_SDIO_BUF_SIZE ;
}

static int sdspi_cmd_init() {
    int err;
    /*For ESP32 & ESP8266, the init value is 0 */
    uint8_t ioe = 0, ie = 0;

    /* GO_IDLE_STATE (CMD0) command resets the card */
    for (int retry = 5; retry > 0; retry--) {
        err = at_spi_cmd_go_idle_state();
        if (err == SYSTEM_ERROR_NONE) {
            break;
        } else if (retry == 0) {
            LOG(ERROR, "SDIO bus cannot enter idle mode, error code:%d", err);
            return err;
        }
        at_do_delay(1000);
    }

    /* IO_SEND_OP_COND(CMD5), Determine if the card is an IO card. */
    err = at_spi_init_io();

    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "Send CMD5 error, error code:%d", err);
        return err;
    }

    /* Enable CRC16 checks for data transfers in SPI mode */
    // at_spi_cmd_init_spi_crc(false);

    // enable function 1
    ioe |= BIT(1);
    err = spi_io_write_byte(0, SD_IO_CCCR_FN_ENABLE, ioe, &ioe);

    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "spi_io_write_byte(0, SD_IO_CCCR_FN_ENABLE...");
        return err;
    }

    LOG(INFO, "IOE: 0x%02x", ioe);

    // enable interrupts for function 1&2 and master enable
    ie |= BIT(0) | BIT(1);
    err = spi_io_write_byte(0, SD_IO_CCCR_INT_ENABLE, ie, &ie); // ESP32 slave 此时拉低WIFI_INT

    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "spi_io_write_byte(0, SD_IO_CCCR_INT_ENABLE...", err);
        return err;
    }

    LOG(INFO, "IE: 0x%02x", ie);

    // get bus width register
    uint8_t bus_width;
    err = spi_io_read_byte(0, SD_IO_CCCR_BUS_WIDTH, &bus_width);

    if (err != SYSTEM_ERROR_NONE) {
        LOG(ERROR, "spi_io_read_byte(0, SD_IO_CCCR_BUS_WIDTH...", err);
        return err;
    }

    LOG(INFO, "BUS_WIDTH: 0x%02x", bus_width);

    // enable continuous SPI interrupts
    bus_width |= CCCR_BUS_WIDTH_ECSI;
    err = spi_io_write_byte(0, SD_IO_CCCR_BUS_WIDTH, bus_width, &bus_width);

    if (err != SYSTEM_ERROR_NONE) {
        return err;
    }

    // LOG(INFO, "BUS_WIDTH: 0x%02x", bus_width);

    uint16_t bs = 512;
    const uint8_t* bs_u8 = (const uint8_t*)&bs;
    uint16_t bs_read = 0;
    uint8_t* bs_read_u8 = (uint8_t*)&bs_read;

    // Set block sizes for functions 1 to given value (default value = 512).
    size_t offset = 0x100;
    spi_io_read_byte(0, 0x100 + SD_IO_CCCR_BLKSIZEL, &bs_read_u8[0]);
    spi_io_read_byte(0, offset + SD_IO_CCCR_BLKSIZEH, &bs_read_u8[1]);
    // LOG(INFO, "Function 1 BS: %04x", (int)bs_read);

    spi_io_write_byte(0, offset + SD_IO_CCCR_BLKSIZEL, bs_u8[0], NULL);
    spi_io_write_byte(0, offset + SD_IO_CCCR_BLKSIZEH, bs_u8[1], NULL);
    spi_io_read_byte(0, offset + SD_IO_CCCR_BLKSIZEL, &bs_read_u8[0]);
    spi_io_read_byte(0, offset + SD_IO_CCCR_BLKSIZEH, &bs_read_u8[1]);
    // LOG(INFO, "Function 1 BS: %04x", (int)bs_read);

    LOG(INFO, "Init SDIO BUS!");

    return SYSTEM_ERROR_NONE;
}

// host use this to initialize the slave card as well as SPI mode
int hal_sdspi_init(HAL_SPI_Interface spi, uint32_t clock, pin_t csPin_, pin_t intPin) {
    CHECK(at_spi_slot_init());
    CHECK(sdspi_cmd_init());
    return SYSTEM_ERROR_NONE;
}

int hal_sdspi_uninit(HAL_SPI_Interface spi, pin_t csPin_, pin_t intPin) {
    // TODO:
    return 0;
}

int hal_sdspi_wait_intr(uint32_t wait_ms) {
    return at_spi_wait_int(wait_ms);
}
