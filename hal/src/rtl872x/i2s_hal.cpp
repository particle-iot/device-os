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

#include "i2s_hal.h"
#include "delay_hal.h"
#include "platforms.h"
#include "FreeRTOS.h"
#include <cstring>
#include <task.h>

extern "C" {

#undef REALTEK_AMBD_SDK
#define REALTEK_AMBD_SDK 1

#include "rtl8721d.h"
#include "rtl8721d_pinmux_defines.h"


// TODO: this is for Realtek pin, e.g. _PB_28, delete below code and use Particle pin
// #undef REALTEK_AMBD_SDK
// #define REALTEK_AMBD_SDK 1
// #include "ameba_soc.h"
// #include "rl6548.h"
// #include "rtl8721d_pinmap.h"

// void i2s_init(i2s_t *obj, u8 sck, u8 ws, u8 sd_tx, u8 sd_rx, u8 mck);

}




/** @addtogroup i2s I2S
 *  @ingroup    hal
 *  @brief      i2s functions
 *  @{
 */


enum {
	SR_8KHZ = I2S_SR_8KHZ,
	SR_12KHZ = I2S_SR_12KHZ,
	SR_16KHZ = I2S_SR_16KHZ,
	SR_24KHZ = I2S_SR_24KHZ,
	SR_32KHZ = I2S_SR_32KHZ,
	SR_48KHZ = I2S_SR_48KHZ,
	SR_64KHZ = I2S_SR_64KHZ,
	SR_96KHZ = I2S_SR_96KHZ,
	SR_192KHZ = I2S_SR_192KHZ,
	SR_384KHZ = I2S_SR_384KHZ,
	SR_7p35KHZ = I2S_SR_7p35KHZ,
	SR_11p025KHZ = I2S_SR_11p025KHZ,
	SR_14p7KHZ = I2S_SR_14p7KHZ,
	SR_22p05KHZ = I2S_SR_22p05KHZ,
	SR_29p4KHZ = I2S_SR_29p4KHZ,
	SR_44p1KHZ = I2S_SR_44p1KHZ,
	SR_58p8KHZ = I2S_SR_58p8KHZ,
	SR_88p2KHZ = I2S_SR_88p2KHZ,
	SR_176p4KHZ = I2S_SR_176p4KHZ
};

enum {
	CH_STEREO = I2S_CH_STEREO,
    CH_5p1 = I2S_CH_5p1,
	CH_MONO = I2S_CH_MONO
};

enum {
	WL_16b = I2S_WL_16,
	WL_24b = I2S_WL_24,
	WL_32b = I2S_WL_32
};


enum {
    I2S_DIR_RX   = I2S_ONLY_RX,    // Rx Only
    I2S_DIR_TX   = I2S_ONLY_TX,    // Tx Only
    I2S_DIR_TXRX = I2S_TXRX     // Tx & Rx (BiDirection)
};

typedef void (*i2s_irq_handler)(uint32_t id, char *pbuf);

typedef struct i2s_s i2s_t;

struct i2s_s {
	uint8_t i2s_idx;
	uint8_t sampling_rate;
	uint8_t channel_num;
	uint8_t word_length;
	uint8_t direction;
};


void i2s_init(i2s_t *obj, u8 sck, u8 ws, u8 sd_tx, u8 sd_rx, u8 mck);
void i2s_deinit(i2s_t *obj);
void i2s_set_dma_buffer(i2s_t *obj, char *tx_buf, char *rx_buf, uint32_t page_num, uint32_t page_size);
void i2s_tx_irq_handler(i2s_t *obj, i2s_irq_handler handler, uint32_t id);
void i2s_rx_irq_handler(i2s_t *obj, i2s_irq_handler handler, uint32_t id);
void i2s_set_direction(i2s_t *obj, int trx_type);
void i2s_set_param(i2s_t *obj, int channel_num, int rate, int word_len);
int* i2s_get_tx_page(i2s_t *obj);
void i2s_send_page(i2s_t *obj, uint32_t *pbuf);
void i2s_recv_page(i2s_t *obj);
void i2s_enable(i2s_t *obj);
void i2s_disable(i2s_t *obj);



typedef struct {
	VOID (*TxCCB)(uint32_t id, char *pbuf);
	u32 TxCBId;
	VOID (*RxCCB)(uint32_t id, char *pbuf);
	u32 RxCBId;
} I2S_USER_CB;

static I2S_InitTypeDef I2SInitStruct;
static I2S_USER_CB I2SUserCB; //Pointer to I2S User Callback
static u32 next_tx_complete = 0;
extern u32 i2s_cur_tx_page;
extern u32 i2s_cur_rx_page;

/**
  * @}
  */

/** @defgroup MBED_I2S_Exported_Functions MBED_I2S Exported Functions
  * @{
  */

static void i2s_sw_reset(void)
{
	i2s_cur_tx_page = 0;
	i2s_cur_rx_page = 0;
	next_tx_complete = 0;
}

static void i2s_isr(void *Data)
{
	/* To avoid gcc warnings */
	( void ) Data;

	u32 I2STxIsr, I2SRxIsr;
	u8  I2SPageNum = I2SInitStruct.I2S_PageNum;
	u32 i;
	u32 pbuf;
	u32 cur_rx_page = 0;

	I2S_ISRGet(I2S_DEV, &I2STxIsr, &I2SRxIsr);
	I2S_INTClear(I2S_DEV, I2STxIsr, I2SRxIsr);

	for (i=0 ; i<I2SPageNum; i++) { // page 0, 1, 2, 3
		cur_rx_page = I2S_GetRxPage(I2S_DEV);
		if (I2SRxIsr & (1<<cur_rx_page)) {
			pbuf = I2S_GetRxPageAddr(cur_rx_page);
			I2SRxIsr &= ~(1<<cur_rx_page);

			I2SUserCB.RxCCB(I2SUserCB.RxCBId, (char*)pbuf);
		} else {
			break;
		}
	}

	for (i=0 ; i<I2SPageNum; i++) { // page 0, 1, 2, 3
		if (I2STxIsr & (1<<next_tx_complete)) {
			pbuf = I2S_GetTxPageAddr(next_tx_complete);
			I2STxIsr &= ~(1<<next_tx_complete);

			next_tx_complete++;
			next_tx_complete &= 0x03;

			I2SUserCB.TxCCB(I2SUserCB.TxCBId, (char*)pbuf);
		} else {
			break;
		}


	}
}

/**
  * @brief  Initializes the I2S device, include clock/function/interrupt/I2S registers.
  * @param  obj: i2s object define in application software.
  * @param  sck: Serial clock u8 according to pinmux spec.
  * @param  ws: Word select u8 according to pinmux spec.
  * @param  sd_tx: Tx u8 according to pinmux spec.
  * @param  sd_rx: Rx u8 according to pinmux spec.
  * @param  mck: Master clock u8 according to pinmux spec.
  * @retval none
  */
void i2s_init(i2s_t *obj, u8 sck, u8 ws, u8 sd_tx, u8 sd_rx, u8 mck)
{
	obj->i2s_idx = 0;

	// DBG_PRINTF(MODULE_I2S, LEVEL_INFO, "%s: Use I2S%d \n", __func__, obj->i2s_idx);

	// Load user defined parameters
	I2S_StructInit(&I2SInitStruct);

	/*I2S Interrupt Initialization*/
	InterruptRegister((IRQ_FUN) i2s_isr, I2S0_PCM0_IRQ, 0, 5);
	InterruptEn(I2S0_PCM0_IRQ, 5);

	/* enable system pll */
	PLL_I2S_Set(ENABLE);
	PLL_PCM_Set(ENABLE);

	/*I2S Pin Mux Initialization*/
	RCC_PeriphClockCmd(APBPeriph_I2S0, APBPeriph_I2S0_CLOCK, ENABLE);
	Pinmux_Config(sck, PINMUX_FUNCTION_I2S);
	Pinmux_Config(ws, PINMUX_FUNCTION_I2S);
	Pinmux_Config(sd_tx, PINMUX_FUNCTION_I2S);
	Pinmux_Config(sd_rx, PINMUX_FUNCTION_I2S);
	Pinmux_Config(mck, PINMUX_FUNCTION_I2S);

	next_tx_complete = 0;
	I2SInitStruct.I2S_TRxAct = obj->direction;
	I2SInitStruct.I2S_ChNum = obj->channel_num;
	I2SInitStruct.I2S_Rate = obj->sampling_rate;
	I2SInitStruct.I2S_WordLen = obj->word_length;
	/*I2S HAL Initialization*/
	I2S_Init(I2S_DEV, &I2SInitStruct);

	/*I2S Enable Module*/
	I2S_Cmd(I2S_DEV, ENABLE);
	I2S_INTConfig(I2S_DEV, (I2S_TX_INT_PAGE0_OK|I2S_TX_INT_PAGE1_OK| I2S_TX_INT_PAGE2_OK|I2S_TX_INT_PAGE3_OK),
		(I2S_RX_INT_PAGE0_OK|I2S_RX_INT_PAGE1_OK | I2S_RX_INT_PAGE2_OK|I2S_RX_INT_PAGE3_OK));
}

/**
  * @brief  Sets page number, page size, page address.
  * @param  obj: i2s object define in application software.
  * @param  tx_buf: pointer to the start address of Tx page.
  * @param  rx_buf: pointer to the start address of Rx page.
  * @param  page_num: page number. This parameter must be set to a value in the 2~4 range
  * @param  page_size: page size. This parameter must be set to a value in the 4~16384 bytes range
  * @retval none
  */
void i2s_set_dma_buffer(i2s_t *obj, char *tx_buf, char *rx_buf,
	uint32_t page_num, uint32_t page_size)
{
	/* To avoid gcc warnings */
	( void ) obj;

	u32 i;
	//uint8_t i2s_idx = obj->i2s_idx;

	if ((page_num < 2) || (page_num > 4) || (page_size < 8)) {
		// DBG_PRINTF(MODULE_I2S, LEVEL_INFO, "%s: PageNum(%d) valid value is 2~4; PageSize(%d must > 8)\r\n", __FUNCTION__, page_num, page_size);
		return;
	}

	I2SInitStruct.I2S_PageNum = page_num;
	I2SInitStruct.I2S_PageSize = page_size/4;    // unit is 4-bytes
	I2S_SetPageSize(I2S_DEV, (page_size/4));
	I2S_SetPageNum(I2S_DEV, (page_num));
	I2S_SetDMABuf(I2S_DEV, (u8*)tx_buf, (u8*)rx_buf);

	for (i=0;i<page_num;i++) {
		I2S_SetTxPageAddr(i, (uint32_t)(tx_buf + ((page_size) * i)));
		I2S_SetRxPageAddr(i, (uint32_t)(rx_buf + ((page_size) * i)));
	}
}

/**
  * @brief  Sets TX interrupt handler.
  * @param  obj: i2s object define in application software.
  * @param  handler: TX interrupt callback function.
  * @param  id: TX interrupt callback function parameter.
  * @retval none
  */
void i2s_tx_irq_handler(i2s_t *obj, i2s_irq_handler handler, uint32_t id)
{
	/* To avoid gcc warnings */
	( void ) obj;

	//uint8_t i2s_idx = obj->i2s_idx;

	I2SUserCB.TxCCB = handler;
	I2SUserCB.TxCBId = id;
}

/**
  * @brief  Sets RX interrupt handler.
  * @param  obj: i2s object define in application software.
  * @param  handler: RX interrupt callback function.
  * @param  id: RX interrupt callback function parameter.
  * @retval none
  */
void i2s_rx_irq_handler(i2s_t *obj, i2s_irq_handler handler, uint32_t id)
{
	/* To avoid gcc warnings */
	( void ) obj;

	//uint8_t i2s_idx = obj->i2s_idx;

	I2SUserCB.RxCCB = handler;
	I2SUserCB.RxCBId = id;
}

/**
  * @brief  Sets i2s data transfer direction.
  * @param  obj: i2s object define in application software.
  * @param  trx_type: transfer direction.
  *          This parameter can be one of the following values:
  *            @arg I2S_DIR_RX: Rx receive direction
  *            @arg I2S_DIR_TX: Tx transmission direction
  *            @arg I2S_DIR_TXRX: Tx & Rx bi-direction
  * @retval none
  */
void i2s_set_direction(i2s_t *obj, int trx_type)
{
	obj->direction = trx_type;

	I2SInitStruct.I2S_TRxAct = trx_type;
	I2S_SetDirection(I2S_DEV, trx_type);

	if (trx_type == I2S_DIR_TX) {
		I2S_RxDmaCmd(I2S_DEV, DISABLE);
	} else if ((trx_type == I2S_DIR_TXRX) ||(trx_type == I2S_DIR_RX) ) {
		I2S_RxDmaCmd(I2S_DEV, ENABLE);
	}
}

/**
  * @brief  Sets i2s channel number, sample rate, word length.
  * @param  obj: i2s object define in application software.
  * @param  channel_num: this parameter can be one of the following values:
  *            @arg CH_STEREO: stereo channel
  *            @arg CH_MONO: mono channel
  * @param  rate: this parameter can be one of the following values:
  *            @arg SR_8KHZ: sample rate is 8kHz
  *            @arg SR_12KHZ: sample rate is 12kHz
  *            @arg SR_16KHZ: sample rate is 16kHz
  *            @arg SR_24KHZ: sample rate is 24kHz
  *            @arg SR_32KHZ: sample rate is 32kHz
  *            @arg SR_48KHZ: sample rate is 48kHz
  *            @arg SR_64KHZ: sample rate is 64kHz
  *            @arg SR_96KHZ: sample rate is 96kHz
  *            @arg SR_192KHZ: sample rate is 192kHz
  *            @arg SR_384KHZ: sample rate is 384kHz
  *            @arg SR_7p35KHZ: sample rate is 7.35kHz
  *            @arg SR_11p025KHZ: sample rate is 11.025kHz
  *            @arg SR_14p7KHZ: sample rate is 14.7kHz
  *            @arg SR_22p05KHZ: sample rate is 22.05kHz
  *            @arg SR_29p4KHZ: sample rate is 29.4kHz
  *            @arg SR_44p1KHZ: sample rate is 44.1kHz
  *            @arg SR_58p8KHZ: sample rate is 58.8kHz
  *            @arg SR_88p2KHZ: sample rate is 88.2kHz
  *            @arg SR_176p4KHZ: sample rate is 176.4kHz
  * @param  word_len: this parameter can be one of the following values:
  *            @arg WL_16b: sample bit is 16 bit
  *            @arg WL_24b: sample bit is 24 bit
  *            @arg WL_32b: sample bit is 32 bit
  * @retval none
  */
void i2s_set_param(i2s_t *obj, int channel_num, int rate, int word_len)
{
	obj->channel_num = channel_num;
	obj->sampling_rate = rate;
	obj->word_length = word_len;

	I2SInitStruct.I2S_ChNum = channel_num;
	I2SInitStruct.I2S_Rate = rate;
	I2SInitStruct.I2S_WordLen = word_len;
	I2S_SetChNum(I2S_DEV, I2SInitStruct.I2S_ChNum);
	I2S_SetRate(I2S_DEV, rate);
	I2S_SetWordLen(I2S_DEV, I2SInitStruct.I2S_WordLen);
}

/**
  * @brief  Deinitializes the I2S device, include function/interrupt/I2S registers.
  * @param  obj: i2s object define in application software.
  * @retval none
  */
void i2s_deinit(i2s_t *obj)
{
	/* To avoid gcc warnings */
	( void ) obj;

	//uint8_t i2s_idx = obj->i2s_idx;
	IRQn_Type IrqNum = I2S0_PCM0_IRQ;

	/*I2S Interrupt DeInitialization*/
	InterruptDis(IrqNum);
	InterruptUnRegister(IrqNum);

	/*I2S Disable Module*/
	I2S_INTConfig(I2S_DEV, 0, 0);
	I2S_Cmd(I2S_DEV, DISABLE);

	i2s_sw_reset();
}

/**
  * @brief  Gets current tx page address.
  * @param  obj: i2s object define in application software.
  * @retval address of current tx page or NULL
  * @note current page own by cpu, return address of current tx page
  * @note current page own by i2s, return NULL
  */
int* i2s_get_tx_page(i2s_t *obj)
{
	/* To avoid gcc warnings */
	( void ) obj;

	//uint8_t i2s_idx = obj->i2s_idx;
	u8 cur_tx_page;

	cur_tx_page = I2S_GetTxPage(I2S_DEV);
	if (!I2S_TxPageBusy(I2S_DEV, cur_tx_page)) {
		return ((int*)I2S_GetTxPageAddr(cur_tx_page));
	} else {
		return NULL;
	}
}

/**
  * @brief  Sets current tx page own by i2s.
  * @param  obj: i2s object define in application software.
  * @param  pbuf: tx buffer adderss.
  * @retval none
  */
void i2s_send_page(i2s_t *obj, uint32_t *pbuf)
{
	/* To avoid gcc warnings */
	( void ) obj;

	u32 cur_tx_page;

	cur_tx_page = I2S_GetTxPage(I2S_DEV);

	if (I2S_GetTxPageAddr(cur_tx_page) != (u32)pbuf)
		DBG_8195A("%s: tx buffer not match cur_tx_page \n", __func__);

	I2S_TxPageDMA_EN(I2S_DEV, cur_tx_page);
}

/**
  * @brief  Sets current rx page own by i2s.
  * @param  obj: i2s object define in application software.
  * @retval none
  */
void i2s_recv_page(i2s_t *obj)
{
	/* To avoid gcc warnings */
	( void ) obj;

	u32 cur_rx_page;

	cur_rx_page = I2S_GetRxPage(I2S_DEV);

	if ((I2S_DEV->IS_RX_PAGE_OWN[cur_rx_page] & BIT(31)) == 0) {
		I2S_RxPageDMA_EN(I2S_DEV,cur_rx_page);
	} else {
		//DBG_8195A("i2s_recv_page: re-enable\r\n");
	}
}

/**
  * @brief  Enable i2s interrupt and function.
  * @param  obj: i2s object define in application software.
  * @retval none
  */
void i2s_enable(i2s_t *obj)
{
	/* To avoid gcc warnings */
	( void ) obj;

	//uint8_t i2s_idx = obj->i2s_idx;

	I2S_Cmd(I2S_DEV, ENABLE);
	I2S_INTConfig(I2S_DEV, (I2S_TX_INT_PAGE0_OK|I2S_TX_INT_PAGE1_OK| I2S_TX_INT_PAGE2_OK|I2S_TX_INT_PAGE3_OK),
		(I2S_RX_INT_PAGE0_OK|I2S_RX_INT_PAGE1_OK | I2S_RX_INT_PAGE2_OK|I2S_RX_INT_PAGE3_OK));
}

/**
  * @brief  Disable i2s interrupt and function.
  * @param  obj: i2s object define in application software.
  * @retval none
  */
void i2s_disable(i2s_t *obj)
{
	/* To avoid gcc warnings */
	( void ) obj;

	//uint8_t i2s_idx = obj->i2s_idx;
	I2S_INTConfig(I2S_DEV, 0, 0);

	I2S_Cmd(I2S_DEV, DISABLE);

	i2s_sw_reset();
}



// ====================
// ====================
// ====================
// ====================
// ====================
// ====================
// ====================



extern short birds_sing[48278];
int birds_sing_size=48278;

static i2s_t i2s_obj;


#define I2S_DMA_PAGE_SIZE	512   // 2 ~ 4096
#define I2S_DMA_PAGE_NUM    4   // Vaild number is 2~4

static u8 i2s_tx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
static u8 i2s_rx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];

#if PLATFORM_ID == PLATFORM_P2
#define I2S_SCLK_PIN	         _PB_29
#define I2S_WS_PIN               _PB_31
#define I2S_SD_TX_PIN	         _PB_26
#define I2S_SD_RX_PIN	         _PA_0
#define I2S_MCK_PIN              _PA_12
#elif PLATFORM_ID == PLATFORM_MSOM
#define I2S_SCLK_PIN             _PB_20
#define I2S_WS_PIN               _PB_21
#define I2S_SD_TX_PIN            _PB_19
#define I2S_SD_RX_PIN            _PA_0
#define I2S_MCK_PIN              _PA_12
#endif


void i2s_tx_complete(void* data, char* pbuf) {
    ;
}

void i2s_rx_complete(void* data, char* pbuf) {
    ;
}

bool hal_i2s_ready() {
    return i2s_get_tx_page(&i2s_obj) ? true : false;
}

void hal_i2s_play(const void* data, size_t size) {
    int* ptx_buf;
    s16* p = (s16*)data;
    u32 offset = 0;
    u32 sampleBlocks = size * 2; // 2 channel
    u32 MaxBlocksPerCycle = I2S_DMA_PAGE_SIZE / sizeof(short);
    u32 dataLen = 0;
again:
    ptx_buf = i2s_get_tx_page(&i2s_obj);
    if (ptx_buf) {
        if (sampleBlocks - offset >= MaxBlocksPerCycle)
            dataLen = MaxBlocksPerCycle * sizeof(short);
        else {
            /* if valid audio data short than a page, make sure the rest of the page set to 0*/
            memset((void*)ptx_buf, 0x00, I2S_DMA_PAGE_SIZE);
            dataLen = (sampleBlocks - offset) * sizeof(short);
        }

        _memcpy((void*)ptx_buf, (void*)(p + offset), dataLen);
        i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
        offset += dataLen / sizeof(short);
        if (offset >= sampleBlocks) {
            offset = 0;
            return;
        }
        goto again;
    }
    // else {
    //     HAL_Delay_Milliseconds(1);
    //     goto again;
    // }
    // TODO: example 里面有延迟, 但是这里加延迟会导致异响
    // else {
    //     // HAL_Delay_Milliseconds(1);
    //     vTaskDelay(1);
    //     goto again;
    // }
}

int hal_i2s_init() {
    i2s_obj.channel_num = CH_STEREO;
    i2s_obj.sampling_rate = SR_48KHZ;
    i2s_obj.word_length = WL_16b;
    i2s_obj.direction = I2S_DIR_TXRX;
    i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_TX_PIN,I2S_SD_RX_PIN,I2S_MCK_PIN);

    i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buf, (char*)i2s_rx_buf, I2S_DMA_PAGE_NUM, I2S_DMA_PAGE_SIZE);
    i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_tx_complete, (uint32_t)&i2s_obj);
    i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_rx_complete, (uint32_t)&i2s_obj);

    return 0;
}

int hal_i2s_deinit() {
	return -1;
}

