/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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



extern "C" {
// TODO: this is for Realtek pin, e.g. _PB_28, delete below code and use Particle pin
#undef REALTEK_AMBD_SDK
#define REALTEK_AMBD_SDK 1
#include "audio_hal.h"
#include "ameba_soc.h"
#include "delay_hal.h"
#include "rl6548.h"
}

#include "check.h"
#include "i2s_hal.h"

/*

TODO:
1. SRAM_NOCACHE_DATA_SECTION 被使用了
SRAM_NOCACHE_DATA_SECTION static u8 sp_rx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM]__attribute__((aligned(32)));
2. tx rx buffer 需要调用 DCache API
*/

#define SP_DMA_PAGE_SIZE        512ul   // 2 ~ 4096
#define SP_DMA_PAGE_NUM         4
#define SP_ZERO_BUF_SIZE        128
#define SP_FULL_BUF_SIZE        128

typedef struct {
	u8 tx_gdma_own;
	u32 tx_addr;
	u32 tx_length;

}TX_BLOCK, *pTX_BLOCK;

typedef struct {
	TX_BLOCK tx_block[SP_DMA_PAGE_NUM];
	TX_BLOCK tx_zero_block;
	u8 tx_gdma_cnt;
	u8 tx_usr_cnt;
	u8 tx_empty_flag;

}SP_TX_INFO, *pSP_TX_INFO;

typedef struct {
	u8 rx_gdma_own;
	u32 rx_addr;
	u32 rx_length;

}RX_BLOCK, *pRX_BLOCK;

typedef struct {
	RX_BLOCK rx_block[SP_DMA_PAGE_NUM];
	RX_BLOCK rx_full_block;
	u8 rx_gdma_cnt;
	u8 rx_usr_cnt;
	u8 rx_full_flag;

}SP_RX_INFO, *pSP_RX_INFO;

extern "C" void PLL_Div(u32 div);
extern "C" BOOL AUDIO_SP_TXGDMA_Restart(u8 GDMA_Index, u8 GDMA_ChNum, u32 tx_addr, u32 tx_length);
extern "C" BOOL AUDIO_SP_RXGDMA_Restart(u8 GDMA_Index, u8 GDMA_ChNum, u32 rx_addr,u32 rx_length);

namespace {
    int checkParams(u32 sampleRate, u32 wordLen, u32 monoStereo, u32 direction) {
        CHECK_TRUE(sampleRate == SR_8K  || \
                   sampleRate == SR_16K || \
                   sampleRate == SR_32K || \
                   sampleRate == SR_48K || \
                   sampleRate == SR_96K, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(wordLen == WL_8  || \
                   wordLen == WL_16 || \
                   wordLen == WL_24, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(monoStereo == CH_MONO || monoStereo == CH_STEREO, SYSTEM_ERROR_INVALID_ARGUMENT);
        // TODO: check direction
        // CHECK_TRUE(direction != APP_DMIC_IN && direction != APP_AMIC_IN, SYSTEM_ERROR_INVALID_ARGUMENT);
        return 0;
    }
} // Anonymous namespace

class AudioSerialPort {
public:
    AudioSerialPort() {}

    int init(u32 sampleRate, u32 wordLen, u32 monoStereo, u32 mode) {
        CHECK(checkParams(sampleRate, wordLen, monoStereo, mode));
        sampleRate_ = sampleRate;
        wordLen_ = wordLen;
        monoStereo_ = monoStereo;
        appMode_ = mode;
        LOG(INFO, "AudioSerialPort init, %d, %d, %d", sampleRate_, wordLen_, monoStereo_);

        // Enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
        PLL_I2S_Set(ENABLE);

        // Enable Audio clock
        RCC_PeriphClockCmd(APBPeriph_AUDIOC, APBPeriph_AUDIOC_CLOCK, ENABLE);
        RCC_PeriphClockCmd(APBPeriph_SPORT, APBPeriph_SPORT_CLOCK, ENABLE);

        u32 div = 48;
        switch (sampleRate_) {
            case SR_96K: div = 4; break;
            case SR_48K: div = 8; break;
            case SR_32K: div = 12; break;
            case SR_16K: div = 24; break;
            case SR_8K: div = 48; break;
            default: LOG(ERROR, "Invalid sample rate: %d", sampleRate_); break;
        }
        PLL_Div(div);

        PAD_CMD(_PB_1, DISABLE); // TODO: _PB_1 -> Particle pin
        PAD_CMD(_PB_2, DISABLE);
        Pinmux_Config(_PB_1, PINMUX_FUNCTION_DMIC);
        Pinmux_Config(_PB_2, PINMUX_FUNCTION_DMIC);

        // Enable pin for audio codec function
        if ((appMode_ & APP_LINE_OUT) == APP_LINE_OUT) {
            PAD_CMD(_PB_28, DISABLE);
            PAD_CMD(_PB_29, DISABLE);
            PAD_CMD(_PB_30, DISABLE);
            PAD_CMD(_PB_31, DISABLE);
        }

	    // Codec init
        CODEC_Init(sampleRate_, wordLen_, monoStereo_, appMode_);
        CODEC_SetVolume(0xAF, 0xAF);
        u16 volume = 0;
        CODEC_GetVolume(&volume);
        LOG(INFO, "codec volume, left: %d, right: %d", volume & 0xFF, volume >> 8);

        // TODO:
        sp_init_tx_variables();
        sp_init_rx_variables();

        AUDIO_SP_StructInit(&spInitStruct_);
        spInitStruct_.SP_MonoStereo= monoStereo_;
        spInitStruct_.SP_WordLen = wordLen_;
	    AUDIO_SP_Init(AUDIO_SPORT_DEV, &spInitStruct_);

        AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, ENABLE);
        AUDIO_SP_TxStart(AUDIO_SPORT_DEV, ENABLE);
        AUDIO_SP_RdmaCmd(AUDIO_SPORT_DEV, ENABLE);
        AUDIO_SP_RxStart(AUDIO_SPORT_DEV, ENABLE);

        u32 rx_addr = (u32)sp_get_free_rx_page();
        u32 rx_length = sp_get_free_rx_length();
        AUDIO_SP_RXGDMA_Init(0, &rxDmaInitStruct_, this, (IRQ_FUN)sp_rx_complete, (u8*)rx_addr, rx_length);
        u32 tx_addr = (u32)sp_get_ready_tx_page();
        u32 tx_length = sp_get_ready_tx_length();
        AUDIO_SP_TXGDMA_Init(0, &txDmaInitStruct_, this, (IRQ_FUN)sp_tx_complete, (u8*)tx_addr, tx_length);

        return 0;
    }

    void flush() {
        while(sp_get_ready_rx_page() != NULL) {
            sp_read_rx_page(NULL, 0);
        }
    }

    int read(void* data, size_t size) {
        static u32 buf[SP_DMA_PAGE_SIZE / 4] __attribute__((aligned(32)));
        size_t copiedSize = 0;
        size_t copyLength = 0;
        uint8_t* p = (uint8_t*)data;

        flush();

        while (copiedSize < size) {
            if (sp_get_ready_rx_page()) {
                sp_read_rx_page((u8 *)buf, SP_DMA_PAGE_SIZE);
                copyLength = size - copiedSize;
                copyLength = copyLength > SP_DMA_PAGE_SIZE ? SP_DMA_PAGE_SIZE : copyLength;
                memcpy(&p[copiedSize], buf, copyLength);
                copiedSize += copyLength;
                // LOG(INFO, "copiedSize: %ld, copyLength: %ld", copiedSize, copyLength);
            } else {
                HAL_Delay_Milliseconds(1);
            }
        }

        return 0;
    }

    int write(const void* data, size_t size) {
        CHECK_TRUE(appMode_ & APP_LINE_OUT, SYSTEM_ERROR_INVALID_STATE);

        static u32 buf[SP_DMA_PAGE_SIZE / 4] __attribute__((aligned(32)));
        size_t sentSize = 0;
        size_t sendLength = 0;
        const uint8_t* p = (const uint8_t*)data;

        while (sentSize < size) {
            if (sp_get_free_tx_page()) {
                sendLength = size - sentSize;
                sendLength = sendLength > SP_DMA_PAGE_SIZE ? SP_DMA_PAGE_SIZE : sendLength;
                memcpy(buf, &p[sentSize], sendLength); // TODO: delete
                sp_write_tx_page((u8 *)buf, SP_DMA_PAGE_SIZE);
                sentSize += sendLength;
                // LOG(INFO, "sentSize: %ld, sendLength: %ld", sentSize, sendLength);
                // HAL_Delay_Milliseconds(1);
            } else {
                HAL_Delay_Milliseconds(1);
            }
        }

        return 0;
    }

    void loopback() {
        static u32 buf[SP_DMA_PAGE_SIZE>>2] __attribute__((aligned(32)));
        while (1) {
            if (sp_get_free_tx_page() && sp_get_ready_rx_page()){
                sp_read_rx_page((u8 *)buf, SP_DMA_PAGE_SIZE);
                sp_write_tx_page((u8 *)buf, SP_DMA_PAGE_SIZE);
            }
        }
    }

private:
    void sp_init_tx_variables(void) {
        int i;

        for (i = 0; i < SP_ZERO_BUF_SIZE; i++) {
            sp_zero_buf[i] = 0;
        }
        sp_tx_info.tx_zero_block.tx_addr = (u32)sp_zero_buf;
        sp_tx_info.tx_zero_block.tx_length = (u32)SP_ZERO_BUF_SIZE;

        sp_tx_info.tx_gdma_cnt = 0;
        sp_tx_info.tx_usr_cnt = 0;
        sp_tx_info.tx_empty_flag = 0;

        for (i = 0; i < SP_DMA_PAGE_NUM; i++) {
            sp_tx_info.tx_block[i].tx_gdma_own = 0;
            sp_tx_info.tx_block[i].tx_addr = (u32)sp_tx_buf + i * SP_DMA_PAGE_SIZE;
            sp_tx_info.tx_block[i].tx_length = SP_DMA_PAGE_SIZE;
        }
    }

    void sp_init_rx_variables(void) {
        int i;

        sp_rx_info.rx_full_block.rx_addr = (u32)sp_full_buf;
        sp_rx_info.rx_full_block.rx_length = (u32)SP_FULL_BUF_SIZE;

        sp_rx_info.rx_gdma_cnt = 0;
        sp_rx_info.rx_usr_cnt = 0;
        sp_rx_info.rx_full_flag = 0;

        for (i = 0; i < SP_DMA_PAGE_NUM; i++) {
            sp_rx_info.rx_block[i].rx_gdma_own = 1;
            sp_rx_info.rx_block[i].rx_addr = (u32)sp_rx_buf + i * SP_DMA_PAGE_SIZE;
            sp_rx_info.rx_block[i].rx_length = SP_DMA_PAGE_SIZE;
        }
    }

    u8* sp_get_free_tx_page(void) {
        pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);

        if (ptx_block->tx_gdma_own)
            return NULL;
        else {
            return (u8*)ptx_block->tx_addr;
        }
    }

    void sp_write_tx_page(u8* src, u32 length) {
        pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);

        memcpy((void*)ptx_block->tx_addr, src, length);
        ptx_block->tx_gdma_own = 1;
        sp_tx_info.tx_usr_cnt++;
        if (sp_tx_info.tx_usr_cnt == SP_DMA_PAGE_NUM) {
            sp_tx_info.tx_usr_cnt = 0;
        }
    }

    void sp_release_tx_page(void) {
        pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);

        if (sp_tx_info.tx_empty_flag) {
        } else {
            ptx_block->tx_gdma_own = 0;
            sp_tx_info.tx_gdma_cnt++;
            if (sp_tx_info.tx_gdma_cnt == SP_DMA_PAGE_NUM) {
                sp_tx_info.tx_gdma_cnt = 0;
            }
        }
    }

    u8* sp_get_ready_tx_page(void) {
        pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);

        if (ptx_block->tx_gdma_own) {
            sp_tx_info.tx_empty_flag = 0;
            return (u8*)ptx_block->tx_addr;
        } else {
            sp_tx_info.tx_empty_flag = 1;
            return (u8*)sp_tx_info.tx_zero_block.tx_addr; // for audio buffer empty case
        }
    }

    u32 sp_get_ready_tx_length(void) {
        pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);

        if (sp_tx_info.tx_empty_flag) {
            return sp_tx_info.tx_zero_block.tx_length;
        } else {
            return ptx_block->tx_length;
        }
    }

    u8* sp_get_ready_rx_page(void) {
        pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);

        if (prx_block->rx_gdma_own)
            return NULL;
        else {
            return (u8*)prx_block->rx_addr;
        }
    }

    void sp_read_rx_page(u8* dst, u32 length) {
        pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);

        if (dst) {
            memcpy(dst, (void const*)prx_block->rx_addr, length);
        }
        prx_block->rx_gdma_own = 1;
        sp_rx_info.rx_usr_cnt++;
        if (sp_rx_info.rx_usr_cnt == SP_DMA_PAGE_NUM) {
            sp_rx_info.rx_usr_cnt = 0;
        }
    }

    void sp_release_rx_page(void) {
        pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);

        if (sp_rx_info.rx_full_flag) {
        } else {
            prx_block->rx_gdma_own = 0;
            sp_rx_info.rx_gdma_cnt++;
            if (sp_rx_info.rx_gdma_cnt == SP_DMA_PAGE_NUM) {
                sp_rx_info.rx_gdma_cnt = 0;
            }
        }
    }

    u8* sp_get_free_rx_page(void) {
        pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);

        if (prx_block->rx_gdma_own) {
            sp_rx_info.rx_full_flag = 0;
            return (u8*)prx_block->rx_addr;
        } else {
            sp_rx_info.rx_full_flag = 1;
            return (u8*)sp_rx_info.rx_full_block.rx_addr; // for audio buffer full case
        }
    }

    u32 sp_get_free_rx_length(void) {
        pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);

        if (sp_rx_info.rx_full_flag) {
            return sp_rx_info.rx_full_block.rx_length;
        } else {
            return prx_block->rx_length;
        }
    }

    void sp_tx_complete_impl() {
        PGDMA_InitTypeDef GDMA_InitStruct = &txDmaInitStruct_;

        u32 tx_addr;
        u32 tx_length;

        /* Clear Pending ISR */
        GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);

        sp_release_tx_page();
        tx_addr = (u32)sp_get_ready_tx_page();
        tx_length = sp_get_ready_tx_length();
        // GDMA_SetSrcAddr(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, tx_addr);
        // GDMA_SetBlkSize(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, tx_length>>2);

        // GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, ENABLE);
        AUDIO_SP_TXGDMA_Restart(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, tx_addr, tx_length);
    }

    void sp_rx_complete_impl() {
        PGDMA_InitTypeDef GDMA_InitStruct = &rxDmaInitStruct_; // TODO:

        u32 rx_addr;
        u32 rx_length;

        DCache_Invalidate(GDMA_InitStruct->GDMA_DstAddr, GDMA_InitStruct->GDMA_BlockSize << 2);
        /* Clear Pending ISR */
        GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);

        sp_release_rx_page();
        rx_addr = (u32)sp_get_free_rx_page();
        rx_length = sp_get_free_rx_length();
        // GDMA_SetDstAddr(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_addr);
        // GDMA_SetBlkSize(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_length>>2);

        // GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, ENABLE);
        AUDIO_SP_RXGDMA_Restart(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_addr, rx_length);
    }

    static void sp_tx_complete(void* data) {
        AudioSerialPort* asp = (AudioSerialPort*)data;
        asp->sp_tx_complete_impl();
    }

    static void sp_rx_complete(void* data) {
        AudioSerialPort* asp = (AudioSerialPort*)data;
        asp->sp_rx_complete_impl();
    }

private:
    u32 sampleRate_;
    u32 wordLen_;
    u32 monoStereo_;
	u32 appMode_;

    SP_InitTypeDef spInitStruct_;
    GDMA_InitTypeDef rxDmaInitStruct_;
    GDMA_InitTypeDef txDmaInitStruct_;

    SP_TX_INFO sp_tx_info;
    SP_RX_INFO sp_rx_info;
    u8 sp_tx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM] __attribute__((aligned(32)));
    u8 sp_zero_buf[SP_ZERO_BUF_SIZE] __attribute__((aligned(32)));
    u8 sp_rx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM] __attribute__((aligned(32)));
    u8 sp_full_buf[SP_FULL_BUF_SIZE] __attribute__((aligned(32)));
};

AudioSerialPort audioSP;

int hal_audio_init(hal_audio_out_device_t outDevice, hal_audio_mode_t monoStereo, hal_audio_sample_rate_t sampleRate, hal_audio_word_len_t wordLen) {
    u32 appMode = (outDevice == HAL_AUDIO_OUT_DEVICE_LINEOUT) ? (APP_DMIC_IN | APP_LINE_OUT) : APP_DMIC_IN;
    u32 rtlAudioMode = 0, rtlWordLen = 0, rtlSampleRate = 0;
    switch (monoStereo) {
        case HAL_AUDIO_MODE_MONO: rtlAudioMode = CH_MONO; break;
        case HAL_AUDIO_MODE_STEREO: rtlAudioMode = CH_STEREO; break;
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    switch (wordLen) {
        case HAL_AUDIO_WORD_LEN_8: rtlWordLen = WL_8; break;
        case HAL_AUDIO_WORD_LEN_16: rtlWordLen = WL_16; break;
        case HAL_AUDIO_WORD_LEN_24: rtlWordLen = WL_24; break;
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    switch (sampleRate) {
        case HAL_AUDIO_SAMPLE_RATE_8K: rtlSampleRate = SR_8K; break;
        case HAL_AUDIO_SAMPLE_RATE_16K: rtlSampleRate = SR_16K; break;
        case HAL_AUDIO_SAMPLE_RATE_32K: rtlSampleRate = SR_32K; break;
        case HAL_AUDIO_SAMPLE_RATE_44P1K: rtlSampleRate = SR_44P1K; break;
        case HAL_AUDIO_SAMPLE_RATE_48K: rtlSampleRate = SR_48K; break;
        case HAL_AUDIO_SAMPLE_RATE_88P2K: rtlSampleRate = SR_88P2K; break;
        case HAL_AUDIO_SAMPLE_RATE_96K: rtlSampleRate = SR_96K; break;
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return audioSP.init(rtlSampleRate, rtlWordLen, rtlAudioMode, appMode);
}

int hal_audio_deinit() {
    return -1;
}

int hal_dmic_loopback() {
    audioSP.loopback();
    return 0;
}

// block mode to read data
int hal_audio_read_dmic(void* data, size_t size) {
    return audioSP.read(data, size);
}

int hal_audio_write_lineout(const void* data, size_t size) {
    return audioSP.write(data, size);
}
