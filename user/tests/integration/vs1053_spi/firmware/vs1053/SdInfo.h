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
#ifndef SdInfo_h
#define SdInfo_h

#include "application.h"
//#include <stdint.h>

// Based on the document:
//
// SD Specifications
// Part 1
// Physical Layer
// Simplified Specification
// Version 2.00
// September 25, 2006
//
// www.sdcard.org/developers/tech/sdcard/pls/Simplified_Physical_Layer_Spec.pdf
//------------------------------------------------------------------------------
// SD card commands
/** GO_IDLE_STATE - init card in spi mode if CS low */
uint8_t const CMD0 = 0X00;
/** SEND_IF_COND - verify SD Memory Card interface operating condition.*/
uint8_t const CMD8 = 0X08;
/** SEND_CSD - read the Card Specific Data (CSD register) */
uint8_t const CMD9 = 0X09;
/** SEND_CID - read the card identification information (CID register) */
uint8_t const CMD10 = 0X0A;
/** SEND_STATUS - read the card status register */
uint8_t const CMD13 = 0X0D;
/** READ_BLOCK - read a single data block from the card */
uint8_t const CMD17 = 0X11;
/** WRITE_BLOCK - write a single data block to the card */
uint8_t const CMD24 = 0X18;
/** WRITE_MULTIPLE_BLOCK - write blocks of data until a STOP_TRANSMISSION */
uint8_t const CMD25 = 0X19;
/** ERASE_WR_BLK_START - sets the address of the first block to be erased */
uint8_t const CMD32 = 0X20;
/** ERASE_WR_BLK_END - sets the address of the last block of the continuous
    range to be erased*/
uint8_t const CMD33 = 0X21;
/** ERASE - erase all previously selected blocks */
uint8_t const CMD38 = 0X26;
/** APP_CMD - escape for application specific command */
uint8_t const CMD55 = 0X37;
/** READ_OCR - read the OCR register of a card */
uint8_t const CMD58 = 0X3A;
/** SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be
     pre-erased before writing */
uint8_t const ACMD23 = 0X17;
/** SD_SEND_OP_COMD - Sends host capacity support information and
    activates the card's initialization process */
uint8_t const ACMD41 = 0X29;
//------------------------------------------------------------------------------
/** status for card in the ready state */
uint8_t const R1_READY_STATE = 0X00;
/** status for card in the idle state */
uint8_t const R1_IDLE_STATE = 0X01;
/** status bit for illegal command */
uint8_t const R1_ILLEGAL_COMMAND = 0X04;
/** start data token for read or write single block*/
uint8_t const DATA_START_BLOCK = 0XFE;
/** stop token for write multiple blocks*/
uint8_t const STOP_TRAN_TOKEN = 0XFD;
/** start data token for write multiple blocks*/
uint8_t const WRITE_MULTIPLE_TOKEN = 0XFC;
/** mask for data response tokens after a write block operation */
uint8_t const DATA_RES_MASK = 0X1F;
/** write data accepted token */
uint8_t const DATA_RES_ACCEPTED = 0X05;
//------------------------------------------------------------------------------
struct CID {
  // byte 0
  uint8_t mid;  // Manufacturer ID
  // byte 1-2
  char oid[2];  // OEM/Application ID
  // byte 3-7
  char pnm[5];  // Product name
  // byte 8
  uint16_t prv_m : 4;  // Product revision n.m
  uint16_t prv_n : 4;
  // byte 9-12
  uint32_t psn;  // Product serial number
  // byte 13
  uint16_t mdt_year_high : 4;  // Manufacturing date
  uint16_t reserved : 4;
  // byte 14
  uint16_t mdt_month : 4;
  uint16_t mdt_year_low :4;
  // byte 15
  uint16_t always1 : 1;
  uint16_t crc : 7;
}__attribute__ ((packed));
typedef struct CID cid_t; 
//------------------------------------------------------------------------------
// CSD for version 1.00 cards
struct CSDV1 {
  // byte 0
  uint16_t reserved1 : 6;
  uint16_t csd_ver : 2;
  // byte 1
  uint8_t taac;
  // byte 2
  uint8_t nsac;
  // byte 3
  uint8_t tran_speed;
  // byte 4
  uint8_t ccc_high;
  // byte 5
  uint16_t read_bl_len : 4;
  uint16_t ccc_low : 4;
  // byte 6
  uint16_t c_size_high : 2;
  uint16_t reserved2 : 2;
  uint16_t dsr_imp : 1;
  uint16_t read_blk_misalign :1;
  uint16_t write_blk_misalign : 1;
  uint16_t read_bl_partial : 1;
  // byte 7
  uint8_t c_size_mid;
  // byte 8
  uint16_t vdd_r_curr_max : 3;
  uint16_t vdd_r_curr_min : 3;
  uint16_t c_size_low :2;
  // byte 9
  uint16_t c_size_mult_high : 2;
  uint16_t vdd_w_cur_max : 3;
  uint16_t vdd_w_curr_min : 3;
  // byte 10
  uint16_t sector_size_high : 6;
  uint16_t erase_blk_en : 1;
  uint16_t c_size_mult_low : 1;
  // byte 11
  uint16_t wp_grp_size : 7;
  uint16_t sector_size_low : 1;
  // byte 12
  uint16_t write_bl_len_high : 2;
  uint16_t r2w_factor : 3;
  uint16_t reserved3 : 2;
  uint16_t wp_grp_enable : 1;
  // byte 13
  uint16_t reserved4 : 5;
  uint16_t write_partial : 1;
  uint16_t write_bl_len_low : 2;
  // byte 14
  uint16_t reserved5: 2;
  uint16_t file_format : 2;
  uint16_t tmp_write_protect : 1;
  uint16_t perm_write_protect : 1;
  uint16_t copy : 1;
  uint16_t file_format_grp : 1;
  // byte 15
  uint16_t always1 : 1;
  uint16_t crc : 7;
}__attribute__ ((packed));
typedef CSDV1 csd1_t;
//------------------------------------------------------------------------------
// CSD for version 2.00 cards
struct CSDV2 {
  // byte 0
  uint16_t reserved1 : 6;
  uint16_t csd_ver : 2;
  // byte 1
  uint8_t taac;
  // byte 2
  uint8_t nsac;
  // byte 3
  uint8_t tran_speed;
  // byte 4
  uint8_t ccc_high;
  // byte 5
  uint16_t read_bl_len : 4;
  uint16_t ccc_low : 4;
  // byte 6
  uint16_t reserved2 : 4;
  uint16_t dsr_imp : 1;
  uint16_t read_blk_misalign :1;
  uint16_t write_blk_misalign : 1;
  uint16_t read_bl_partial : 1;
  // byte 7
  uint16_t reserved3 : 2;
  uint16_t c_size_high : 6;
  // byte 8
  uint8_t c_size_mid;
  // byte 9
  uint8_t c_size_low;
  // byte 10
  uint16_t sector_size_high : 6;
  uint16_t erase_blk_en : 1;
  uint16_t reserved4 : 1;
  // byte 11
  uint16_t wp_grp_size : 7;
  uint16_t sector_size_low : 1;
  // byte 12
  uint16_t write_bl_len_high : 2;
  uint16_t r2w_factor : 3;
  uint16_t reserved5 : 2;
  uint16_t wp_grp_enable : 1;
  // byte 13
  uint16_t reserved6 : 5;
  uint16_t write_partial : 1;
  uint16_t write_bl_len_low : 2;
  // byte 14
  uint16_t reserved7: 2;
  uint16_t file_format : 2;
  uint16_t tmp_write_protect : 1;
  uint16_t perm_write_protect : 1;
  uint16_t copy : 1;
  uint16_t file_format_grp : 1;
  // byte 15
  uint16_t always1 : 1;
  uint16_t crc : 7;
}__attribute__ ((packed));
typedef CSDV2 csd2_t;
//------------------------------------------------------------------------------
// union of old and new style CSD register
union csd_t {
  csd1_t v1;
  csd2_t v2;
};
#endif  // SdInfo_h
