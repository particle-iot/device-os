/* Arduino Sd2Card Library
 * Copyright (C) 2009 by William Greiman
 *
 * This file is part of the Arduino Sd2Card Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino Sd2Card Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "application.h"
#include "Sd2Card.h"

#if !defined(PLATFORM_ID)		// Core v0.3.4
#warning "CORE"
  #define pinSetFast(_pin)		PIN_MAP[_pin].gpio_peripheral->BSRR = PIN_MAP[_pin].gpio_pin
  #define pinResetFast(_pin)	PIN_MAP[_pin].gpio_peripheral->BRR = PIN_MAP[_pin].gpio_pin
  #define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif

//#include "spark_wiring_spi.h"
//#include "spark_wiring_usbserial.h"

/*#include "dma.h"
#ifdef SPI_DMA
bool dmaActive;
uint8_t mysink[1];

inline void DMAEvent(){
    dma_irq_cause event = dma_get_irq_cause(DMA1, DMA_CH3);
    switch(event) {
        case DMA_TRANSFER_COMPLETE:
            dma_detach_interrupt(DMA1, DMA_CH2);
            dma_detach_interrupt(DMA1, DMA_CH3);
            dmaActive = false;
            break;
        case DMA_TRANSFER_ERROR:
            SerialUSB.println("DMA Error - read/write data might be corrupted");
            break;
    }
}
#endif*/

//pointer to spi object
//HardwareSPI SD_SPI(SD_SPI_NUMBER);

//------------------------------------------------------------------------------
// functions for hardware SPI
// SPI1 and SPI2 are also acceptable depending on which hardware SPI you're using
#define SPI_INTERFACE SPI

#ifdef SPI_SPEED_UP
#	if SD_SPI_NUMBER   == 1
#		define __spi_dev__		SPI1
#	elif SD_SPI_NUMBER == 2
#		define __spi_dev__		SPI2
#	elif SD_SPI_NUMBER == 3
#		define __spi_dev__		SPI3
#	endif
#	define _RXNE_BIT	0x01
#	define _TXE_BIT		0x02
	extern "C" {
		inline uint8_t _spiSend(uint8_t b){
			while( !( __spi_dev__->regs->SR & _TXE_BIT) ) ;
			__spi_dev__->regs->DR = b;
			while( !( __spi_dev__->regs->SR & _RXNE_BIT) ) ;
			return __spi_dev__->regs->DR;
		}
		#define spiRec()	_spiSend(0xff)
		#define spiSend(b)	_spiSend(b)
	} /* extern C */

#else /* not SPI_SPEED_UP */
	//#ifdef SPI_DMA
		//#define spiSend(b) SPI_INTERFACE.transfer(b)
		#define spiSend(b) sparkSPISend(b)
		/** Receive a byte from the card */
		//#define spiRec() SPI_INTERFACE.transfer(0XFF)
		#define spiRec() sparkSPISend(0XFF)
	//#else
		/** Send a byte to the card */
		//#define spiSend(b) SPI_INTERFACE.send(b)
		/** Receive a byte from the card */
		//#define spiRec() SPI_INTERFACE.send(0XFF)
	//#endif
#endif	/* SPI_SPEED_UP */

//------------------------------------------------------------------------------
// send command and return error code.  Return zero for OK
uint8_t Sd2Card::cardCommand(uint8_t cmd, uint32_t arg) {
  // end read if in partialBlockRead mode
  readEnd();

  // select card
  chipSelectLow();

  // wait up to 300 ms if busy
  waitNotBusy(300);

  // send command
  spiSend(cmd | 0x40);

  // send argument
  for (int8_t s = 24; s >= 0; s -= 8) spiSend(arg >> s);

  // send CRC
  uint8_t crc = 0XFF;
  if (cmd == CMD0) crc = 0X95;  // correct crc for CMD0 with arg 0
  if (cmd == CMD8) crc = 0X87;  // correct crc for CMD8 with arg 0X1AA
  spiSend(crc);

  // wait for response
  for (uint8_t i = 0; ((status_ = spiRec()) & 0X80) && i != 0XFF; i++);
  return status_;
}
//------------------------------------------------------------------------------
/**
 * Determine the size of an SD flash memory card.
 *
 * \return The number of 512 byte data blocks in the card
 *         or zero if an error occurs.
 */
uint32_t Sd2Card::cardSize(void) {
  csd_t csd;
  if (!readCSD(&csd)) return 0;
  if (csd.v1.csd_ver == 0) {
    uint8_t read_bl_len = csd.v1.read_bl_len;
    uint16_t c_size = (csd.v1.c_size_high << 10)
                      | (csd.v1.c_size_mid << 2) | csd.v1.c_size_low;
    uint8_t c_size_mult = (csd.v1.c_size_mult_high << 1)
                          | csd.v1.c_size_mult_low;
    return (uint32_t)(c_size + 1) << (c_size_mult + read_bl_len - 7);
  } else if (csd.v2.csd_ver == 1) {
    uint32_t c_size = ((uint32_t)csd.v2.c_size_high << 16)
                      | (csd.v2.c_size_mid << 8) | csd.v2.c_size_low;
    return (c_size + 1) << 10;
  } else {
    error(SD_CARD_ERROR_BAD_CSD);
    return 0;
  }
}

//------------------------------------------------------------------------------
inline void Sd2Card::chipSelectHigh(void) {
  digitalWrite(chipSelectPin_, HIGH);
}

//------------------------------------------------------------------------------
inline void Sd2Card::chipSelectLow(void) {
  digitalWrite(chipSelectPin_, LOW);
}

//------------------------------------------------------------------------------
/** Erase a range of blocks.
 *
 * \param[in] firstBlock The address of the first block in the range.
 * \param[in] lastBlock The address of the last block in the range.
 *
 * \note This function requests the SD card to do a flash erase for a
 * range of blocks.  The data on the card after an erase operation is
 * either 0 or 1, depends on the card vendor.  The card must support
 * single block erase.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::erase(uint32_t firstBlock, uint32_t lastBlock) {
  if (!eraseSingleBlockEnable()) {
    error(SD_CARD_ERROR_ERASE_SINGLE_BLOCK);
	Serial.println("Error: Erase Single Block");
    goto fail;
  }
  if (type_ != SD_CARD_TYPE_SDHC) {
    firstBlock <<= 9;
    lastBlock <<= 9;
  }
  if (cardCommand(CMD32, firstBlock)
    || cardCommand(CMD33, lastBlock)
    || cardCommand(CMD38, 0)) {
      error(SD_CARD_ERROR_ERASE);
	  Serial.println("Error: Erase");
	  goto fail;
  }
  if (!waitNotBusy(SD_ERASE_TIMEOUT)) {
    error(SD_CARD_ERROR_ERASE_TIMEOUT);
	Serial.println("Error: Erase timeout");
	goto fail;
  }
  chipSelectHigh();
  return true;

 fail:
  chipSelectHigh();
  Serial.println("Error: Sd2Card::Erase()");
  return false;
}
//------------------------------------------------------------------------------
/** Determine if card supports single block erase.
 *
 * \return The value one, true, is returned if single block erase is supported.
 * The value zero, false, is returned if single block erase is not supported.
 */
uint8_t Sd2Card::eraseSingleBlockEnable(void) {
  csd_t csd;
  return readCSD(&csd) ? csd.v1.erase_blk_en : 0;
}
//------------------------------------------------------------------------------
/**
 * Initialize an SD flash memory card.
 *
 * \param[in] sckRateID SPI clock rate selector. See setSckRate().
 * \param[in] chipSelectPin SD chip select pin number.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.  The reason for failure
 * can be determined by calling errorCode() and errorData().
 */
uint8_t Sd2Card::init(uint8_t sckRateID, uint8_t chipSelectPin) {
  chipSelectPin_ = chipSelectPin;
  pinMode(chipSelectPin_, OUTPUT);
  SPI_INTERFACE.begin();
  SPI_INTERFACE.setDataMode(SPI_MODE0);
  SPI_INTERFACE.setBitOrder(MSBFIRST);

  SPImode_ = 1;		// Set hardware SPI mode

  if( sckRateID == SPI_FULL_SPEED ){
	  SPI_INTERFACE.setClockDivider(SPI_CLOCK_DIV4);
  }
  else
	  SPI_INTERFACE.setClockDivider(SPI_CLOCK_DIV8);

  return init();
}

uint8_t Sd2Card::init(uint8_t mosiPin, uint8_t misoPin, uint8_t clockPin, uint8_t chipSelectPin) {
  mosiPin_ = mosiPin;
  misoPin_ = misoPin;
  clockPin_ = clockPin;
  chipSelectPin_ = chipSelectPin;

  pinMode(clockPin_, OUTPUT);
  pinMode(mosiPin_, OUTPUT);
  pinMode(misoPin_, INPUT);
  pinMode(chipSelectPin_, OUTPUT);

  SPImode_ = 0;		// Set software SPI mode

  return init();
}

uint8_t Sd2Card::init() {
//  chipSelectPin_ = SS;
//  SPI_INTERFACE.begin();

  errorCode_ = inBlock_ = partialBlockRead_ = type_ = 0;
#ifdef SPI_DMA
    //init DMA
    dma_init(DMA1);
    //enable SPI over DMA
    spi_rx_dma_enable(SPI1);
    spi_tx_dma_enable(SPI1);
    //DMA activity control
    dmaActive = false;
    //Acknowledgment array
    for(int i=0; i<SPI_BUFF_SIZE; i++)
        ack[i] = 0xFF;
#endif

  //chipSelectPin_ = chipSelectPin;
  // 16-bit init start time allows over a minute

  uint16_t t0 = (uint16_t)millis();
  uint32_t arg;


//  SPIn = s;
  // set pin modes
/*  pinMode(chipSelectPin_, OUTPUT);

  chipSelectHigh();
  pinMode(SPI_MISO_PIN, INPUT);
  pinMode(SPI_MOSI_PIN, OUTPUT);
  pinMode(SPI_SCK_PIN, OUTPUT);
*/

  // SS must be in output mode even it is not chip select
//  pinMode(SS_PIN, OUTPUT);
  // Enable SPI, Master, clock rate f_osc/128
//  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
  // clear double speed
//  SPSR &= ~(1 << SPI2X);

  // must supply min of 74 clock cycles with CS high.

  chipSelectHigh();
    for (uint8_t i = 0; i < 10; i++) spiSend(0XFF);
  chipSelectLow();

  // command to go idle in SPI mode
  while ((status_ = cardCommand(CMD0, 0)) != R1_IDLE_STATE) {
    if (((uint16_t)millis() - t0) > SD_INIT_TIMEOUT) {
		Serial.println("Error: CMD0");
		error(SD_CARD_ERROR_CMD0);
		goto fail;
    }
  }
  // check SD version
  if ((cardCommand(CMD8, 0x1AA) & R1_ILLEGAL_COMMAND)) {
    type(SD_CARD_TYPE_SD1);
  } else {
    // only need last byte of r7 response
    for (uint8_t i = 0; i < 4; i++) status_ = spiRec();
    if (status_ != 0XAA) {
      error(SD_CARD_ERROR_CMD8);
	  Serial.println("Error: CMD8");
	  goto fail;
    }
    type(SD_CARD_TYPE_SD2);
  }
  // initialize card and send host supports SDHC if SD2
  arg = (type() == SD_CARD_TYPE_SD2) ? 0X40000000 : 0;

  while ((status_ = cardAcmd(ACMD41, arg)) != R1_READY_STATE) {
    // check for timeout
    if (((uint16_t)millis() - t0) > SD_INIT_TIMEOUT) {
		Serial.println("Error: ACMD41");
		error(SD_CARD_ERROR_ACMD41);
		goto fail;
    }
  }
  // if SD2 read OCR register to check for SDHC card
  if (type() == SD_CARD_TYPE_SD2) {
    if (cardCommand(CMD58, 0)) {
		Serial.println("Error: CMD58");
		error(SD_CARD_ERROR_CMD58);
		goto fail;
    }
    if ((spiRec() & 0XC0) == 0XC0) type(SD_CARD_TYPE_SDHC);
    // discard rest of ocr - contains allowed voltage range
    for (uint8_t i = 0; i < 3; i++) spiRec();
  }
  chipSelectHigh();

//  return setSckRate(sckRateID);
  return true;

 fail:
  chipSelectHigh();
  Serial.println("Error: Sd2Card::init()");
  return false;
}
//------------------------------------------------------------------------------
/**
 * Enable or disable partial block reads.
 *
 * Enabling partial block reads improves performance by allowing a block
 * to be read over the SPI bus as several sub-blocks.  Errors may occur
 * if the time between reads is too long since the SD card may timeout.
 * The SPI SS line will be held low until the entire block is read or
 * readEnd() is called.
 *
 * Use this for applications like the Adafruit Wave Shield.
 *
 * \param[in] value The value TRUE (non-zero) or FALSE (zero).)
 */
void Sd2Card::partialBlockRead(uint8_t value) {
  readEnd();
  partialBlockRead_ = value;
}
//------------------------------------------------------------------------------
/**
 * Read a 512 byte block from an SD card device.
 *
 * \param[in] block Logical block to be read.
 * \param[out] dst Pointer to the location that will receive the data.

 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::readBlock(uint32_t block, uint8_t* dst) {
  return readData(block, 0, 512, dst);
}
//------------------------------------------------------------------------------
/**
 * Read part of a 512 byte block from an SD card.
 *
 * \param[in] block Logical block to be read.
 * \param[in] offset Number of bytes to skip at start of block
 * \param[out] dst Pointer to the location that will receive the data.
 * \param[in] count Number of bytes to read
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::readData(uint32_t block,
        uint16_t offset, uint16_t count, uint8_t* dst) {
  //uint16_t n;
  if (count == 0) return true;
  if ((count + offset) > 512) {
    goto fail;
  }
  if (!inBlock_ || block != block_ || offset < offset_) {
    block_ = block;
    // use address if not SDHC card
    if (type()!= SD_CARD_TYPE_SDHC) block <<= 9;
    if (cardCommand(CMD17, block)) {
      error(SD_CARD_ERROR_CMD17);
	  Serial.println("Error: CMD17");
      goto fail;
    }
    if (!waitStartBlock()) {
      goto fail;
    }
    offset_ = 0;
    inBlock_ = 1;
  }

#ifdef SPI_DMA
    // skip data before offset
    if(offset_ < offset){
        dma_setup_transfer(DMA1,
				DMA_CH3,
				&SPI1->regs->DR,
				DMA_SIZE_8BITS,
				ack,
				DMA_SIZE_8BITS,
                           (/*DMA_MINC_MODE | DMA_CIRC_MODE  |*/ DMA_FROM_MEM | DMA_TRNS_CMPLT | DMA_TRNS_ERR));
        dma_attach_interrupt(DMA1, DMA_CH3, DMAEvent);
        dma_set_priority(DMA1, DMA_CH3, DMA_PRIORITY_VERY_HIGH);
        dma_set_num_transfers(DMA1, DMA_CH3, offset - offset_);

        dmaActive = true;
        dma_enable(DMA1, DMA_CH3);

        while(dmaActive) delayMicroseconds(1);
        dma_disable(DMA1, DMA_CH3);
    }
    offset_ = offset;

    // transfer data
    dma_setup_transfer(DMA1, DMA_CH2, &SPI1->regs->DR, DMA_SIZE_8BITS, dst, DMA_SIZE_8BITS,
                       (DMA_MINC_MODE | DMA_TRNS_CMPLT | DMA_TRNS_ERR));
    dma_attach_interrupt(DMA1, DMA_CH2, DMAEvent);
    dma_setup_transfer(DMA1, DMA_CH3, &SPI1->regs->DR, DMA_SIZE_8BITS, ack, DMA_SIZE_8BITS,
                       (/*DMA_MINC_MODE | DMA_CIRC_MODE |*/ DMA_FROM_MEM));
    dma_set_priority(DMA1, DMA_CH2, DMA_PRIORITY_VERY_HIGH);
    dma_set_priority(DMA1, DMA_CH3, DMA_PRIORITY_VERY_HIGH);
    dma_set_num_transfers(DMA1, DMA_CH2, count);
    dma_set_num_transfers(DMA1, DMA_CH3, count);

    dmaActive = true;
    dma_enable(DMA1, DMA_CH3);
    dma_enable(DMA1, DMA_CH2);

    while(dmaActive) delayMicroseconds(1);
    dma_disable(DMA1, DMA_CH3);
    dma_disable(DMA1, DMA_CH2);

    offset_ += count;
    if (!partialBlockRead_ || offset_ >= SPI_BUFF_SIZE) {
        readEnd();
    }

#else
  // skip data before offset
  for (;offset_ < offset; offset_++) {
    spiRec();
  }
  // transfer data
  for (uint16_t i = 0; i < count; i++) {
    dst[i] = spiRec();
  }
  offset_ += count;
  if (!partialBlockRead_ || offset_ >= 512) {
    // read rest of data, checksum and set chip select high
    readEnd();
  }
#endif

  return true;

 fail:
  chipSelectHigh();
  Serial.println("Error: Sd2Card::readData()");
  return false;
}
//------------------------------------------------------------------------------
/** Skip remaining data in a block when in partial block read mode. */
void Sd2Card::readEnd(void) {
  if (inBlock_) {
      // skip data and crc
#ifdef SPI_DMA
        dma_setup_transfer(DMA1, DMA_CH3, &SPI1->regs->DR, DMA_SIZE_8BITS, ack, DMA_SIZE_8BITS,
                           (/*DMA_MINC_MODE | DMA_CIRC_MODE |*/ DMA_FROM_MEM | DMA_TRNS_CMPLT | DMA_TRNS_ERR));
        dma_attach_interrupt(DMA1, DMA_CH3, DMAEvent);
        dma_set_priority(DMA1, DMA_CH3, DMA_PRIORITY_VERY_HIGH);
        dma_set_num_transfers(DMA1, DMA_CH3, SPI_BUFF_SIZE + 1 - offset_);

        dmaActive = true;
        dma_enable(DMA1, DMA_CH3);

        while(dmaActive)delayMicroseconds(1);
        dma_disable(DMA1, DMA_CH3);
#else  // SPI_DMA
    while (offset_++ < 514) spiRec();
#endif  // SPI_DMA
    chipSelectHigh();
    inBlock_ = 0;
  }
}
//------------------------------------------------------------------------------
/** read CID or CSR register */
uint8_t Sd2Card::readRegister(uint8_t cmd, void* buf) {
  uint8_t* dst = reinterpret_cast<uint8_t*>(buf);
  if (cardCommand(cmd, 0)) {
	  error(SD_CARD_ERROR_READ_REG);
	  Serial.println("Error: Read reg");
	  goto fail;
  }
  if (!waitStartBlock()) goto fail;
  // transfer data
  for (uint16_t i = 0; i < 16; i++) dst[i] = spiRec();
  spiRec();  // get first crc byte
  spiRec();  // get second crc byte
  chipSelectHigh();
  return true;

 fail:
  Serial.println("Error: Sd2Card::readRegister()");
  chipSelectHigh();
  return false;
}
//------------------------------------------------------------------------------
/**
 * Set the SPI clock rate.
 *
 * \param[in] sckRateID A value in the range [0, 6].
 *
 * The SPI clock will be set to F_CPU/pow(2, 1 + sckRateID). The maximum
 * SPI rate is F_CPU/2 for \a sckRateID = 0 and the minimum rate is F_CPU/128
 * for \a scsRateID = 6.
 *
 * \return The value one, true, is returned for success and the value zero,
 * false, is returned for an invalid value of \a sckRateID.
 */
uint8_t Sd2Card::setSckRate(uint8_t sckRateID) {
/*  if (sckRateID > 6) {
    error(SD_CARD_ERROR_SCK_RATE);
    return false;
  }
  // see avr processor datasheet for SPI register bit definitions
  if ((sckRateID & 1) || sckRateID == 6) {
    SPSR &= ~(1 << SPI2X);
  } else {
    SPSR |= (1 << SPI2X);
  }
  SPCR &= ~((1 <<SPR1) | (1 << SPR0));
  SPCR |= (sckRateID & 4 ? (1 << SPR1) : 0)
    | (sckRateID & 2 ? (1 << SPR0) : 0);
*/
	return true;
}
//------------------------------------------------------------------------------
// wait for card to go not busy
uint8_t Sd2Card::waitNotBusy(uint16_t timeoutMillis) {
  uint16_t t0 = millis();
  do {
    if (spiRec() == 0XFF) return true;
  }
  while (((uint16_t)millis() - t0) < timeoutMillis);
  return false;
}
//------------------------------------------------------------------------------
/** Wait for start block token */
uint8_t Sd2Card::waitStartBlock(void) {
  uint16_t t0 = millis();
  while ((status_ = spiRec()) == 0XFF) {
    if (((uint16_t)millis() - t0) > SD_READ_TIMEOUT) {
      error(SD_CARD_ERROR_READ_TIMEOUT);
	  Serial.println("Error: Read timeout");
      goto fail;
    }
  }
  if (status_ != DATA_START_BLOCK) {
    error(SD_CARD_ERROR_READ);
	Serial.println("Error: Read");
    goto fail;
  }
  return true;

 fail:
  chipSelectHigh();
  Serial.println("Error: Sd2Card::waitStartBlock()");
  return false;
}
//------------------------------------------------------------------------------
/**
 * Writes a 512 byte block to an SD card.
 *
 * \param[in] blockNumber Logical block to be written.
 * \param[in] src Pointer to the location of the data to be written.
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::writeBlock(uint32_t blockNumber, const uint8_t* src) {
#if SD_PROTECT_BLOCK_ZERO
  // don't allow write to first block
  if (blockNumber == 0) {
    error(SD_CARD_ERROR_WRITE_BLOCK_ZERO);
    Serial.println("Error: Write block zero");
    goto fail;
  }
#endif  // SD_PROTECT_BLOCK_ZERO

  // use address if not SDHC card
  if (type() != SD_CARD_TYPE_SDHC) blockNumber <<= 9;
  if (cardCommand(CMD24, blockNumber)) {
	Serial.println("Error: CMD24");
	error(SD_CARD_ERROR_CMD24);
	goto fail;
  }
  if (!writeData(DATA_START_BLOCK, src)) goto fail;

  // wait for flash programming to complete
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
    error(SD_CARD_ERROR_WRITE_TIMEOUT);
    Serial.println("Error: Write timeout");
    goto fail;
  }
  // response is r2 so get and check two bytes for nonzero
  if (cardCommand(CMD13, 0) || spiRec()) {
    error(SD_CARD_ERROR_WRITE_PROGRAMMING);
    Serial.println("Error: Write programming");
    goto fail;
  }
  chipSelectHigh();
  return true;

 fail:
  chipSelectHigh();
  Serial.println("Error: Sd2Card::writeBlock");
  return false;
}
//------------------------------------------------------------------------------
/** Write one data block in a multiple block write sequence */
uint8_t Sd2Card::writeData(const uint8_t* src) {
  // wait for previous write to finish
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
    error(SD_CARD_ERROR_WRITE_MULTIPLE);
	Serial.println("Error: writeData");
    chipSelectHigh();
    return false;
  }
  return writeData(WRITE_MULTIPLE_TOKEN, src);
}
//------------------------------------------------------------------------------
// send one block of data for write block or write multiple blocks
uint8_t Sd2Card::writeData(uint8_t token, const uint8_t* src) {
#ifdef SPI_DMA
        dma_setup_transfer(DMA1, DMA_CH3, &SPI1->regs->DR, DMA_SIZE_8BITS, (uint8_t *)src, DMA_SIZE_8BITS, (DMA_MINC_MODE |  DMA_FROM_MEM | DMA_TRNS_CMPLT | DMA_TRNS_ERR));
        dma_attach_interrupt(DMA1, DMA_CH3, DMAEvent);
        dma_set_priority(DMA1, DMA_CH3, DMA_PRIORITY_VERY_HIGH);
        dma_set_num_transfers(DMA1, DMA_CH3, 512);

        dmaActive = true;
        dma_enable(DMA1, DMA_CH3);

        while(dmaActive) delayMicroseconds(1);
        dma_disable(DMA1, DMA_CH3);

#else  // SPI_DMA
  spiSend(token);
  for (uint16_t i = 0; i < 512; i++) {
    spiSend(src[i]);
  }
#endif  // OPTIMIZE_HARDWARE_SPI
  spiSend(0xff);  // dummy crc
  spiSend(0xff);  // dummy crc

  status_ = spiRec();
  if ((status_ & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
    error(SD_CARD_ERROR_WRITE);
    chipSelectHigh();
	Serial.println("Error: Write");
    Serial.println("Error: Sd2Card::writeData()");
    return false;
  }
  return true;
}
//------------------------------------------------------------------------------
/** Start a write multiple blocks sequence.
 *
 * \param[in] blockNumber Address of first block in sequence.
 * \param[in] eraseCount The number of blocks to be pre-erased.
 *
 * \note This function is used with writeData() and writeStop()
 * for optimized multiple block writes.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::writeStart(uint32_t blockNumber, uint32_t eraseCount) {
#if SD_PROTECT_BLOCK_ZERO
  // don't allow write to first block
  if (blockNumber == 0) {
    error(SD_CARD_ERROR_WRITE_BLOCK_ZERO);
	Serial.println("Error: Write block zero");
    goto fail;
  }
#endif  // SD_PROTECT_BLOCK_ZERO
  // send pre-erase count
  if (cardAcmd(ACMD23, eraseCount)) {
	Serial.println("Error: ACMD23");
    error(SD_CARD_ERROR_ACMD23);
    goto fail;
  }
  // use address if not SDHC card
  if (type() != SD_CARD_TYPE_SDHC) blockNumber <<= 9;
  if (cardCommand(CMD25, blockNumber)) {
    error(SD_CARD_ERROR_CMD25);
	Serial.println("Error: CMD25");
    goto fail;
  }
  return true;

 fail:
  chipSelectHigh();
    Serial.println("Error: Sd2Card::writeStart()");
  return false;
}
//------------------------------------------------------------------------------
/** End a write multiple blocks sequence.
 *
* \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2Card::writeStop(void) {
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) goto fail;
  spiSend(STOP_TRAN_TOKEN);
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) goto fail;
  chipSelectHigh();
  return true;

 fail:
  error(SD_CARD_ERROR_STOP_TRAN);
  chipSelectHigh();
    Serial.println("Error: Sd2Card::writeStop()");
  return false;
}

inline __attribute__((always_inline))
uint8_t Sd2Card::sparkSPISend(uint8_t data) {
	uint8_t b=0;

	if (SPImode_) {				// SPI Mode is Hardware so use Spark SPI function
		b = SPI_INTERFACE.transfer(data);
	}
	else {						// SPI Mode is Software so use bit bang method
		for (uint8_t bit = 0; bit < 8; bit++)  {
			if (data & (1 << (7-bit)))		// walks down mask from bit 7 to bit 0
				//PIN_MAP[mosiPin_].gpio_peripheral->BSRR = PIN_MAP[mosiPin_].gpio_pin; // Data High
				pinSetFast(mosiPin_);
			else
				//PIN_MAP[mosiPin_].gpio_peripheral->BRR = PIN_MAP[mosiPin_].gpio_pin; // Data Low
				pinResetFast(mosiPin_);

			//PIN_MAP[clockPin_].gpio_peripheral->BSRR = PIN_MAP[clockPin_].gpio_pin; // Clock High
			pinSetFast(clockPin_);

			b <<= 1;
			//if (PIN_MAP[misoPin_].gpio_peripheral->IDR & PIN_MAP[misoPin_].gpio_pin)
			if (pinReadFast(misoPin_))
				b |= 1;

			//PIN_MAP[clockPin_].gpio_peripheral->BRR = PIN_MAP[clockPin_].gpio_pin; // Clock Low
			pinResetFast(clockPin_);
		}
	}
	return b;
}
