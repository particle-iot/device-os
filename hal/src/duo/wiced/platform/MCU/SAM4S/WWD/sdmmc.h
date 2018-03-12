/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */


#ifndef SDMMC_CMD_H
#define SDMMC_CMD_H
#include "RTOS/wwd_rtos_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------
 *         Constants
 *----------------------------------------------------------------------------*/
#define CARD_TYPE_bmHC           (1 << 0)   /**< Bit for High-Capacity(Density) */
#define CARD_TYPE_bmSDMMC        (0x3 << 1) /**< Bits mask for SD/MMC */
#define CARD_TYPE_bmUNKNOWN      (0x0 << 1) /**< Bits for Unknown card */
#define CARD_TYPE_bmSD           (0x1 << 1) /**< Bits for SD */
#define CARD_TYPE_bmMMC          (0x2 << 1) /**< Bits for MMC */
#define CARD_TYPE_bmSDIO         (1 << 3)   /**< Bit for SDIO */
/** Card can not be identified */
#define CARD_UNKNOWN    (0)
/** SD Card (0x2) */
#define CARD_SD         (CARD_TYPE_bmSD)
/** SD High Capacity Card (0x3) */
#define CARD_SDHC       (CARD_TYPE_bmSD|CARD_TYPE_bmHC)
/** MMC Card (0x4) */
#define CARD_MMC        (CARD_TYPE_bmMMC)
/** MMC High-Density Card (0x5) */
#define CARD_MMCHD      (CARD_TYPE_bmMMC|CARD_TYPE_bmHC)
/** SDIO only card (0x8) */
#define CARD_SDIO       (CARD_TYPE_bmSDIO)
/** SDIO Combo, with SD embedded (0xA) */
#define CARD_SDCOMBO    (CARD_TYPE_bmSDIO|CARD_SD)
/** SDIO Combo, with SDHC embedded (0xB) */
#define CARD_SDHCCOMBO  (CARD_TYPE_bmSDIO|CARD_SDHC)

/** No error */
#define SDMMC_OK                    0
/** The driver is locked. */
#define SDMMC_ERROR_LOCKED          1
/** There was an error with the SD driver. */
#define SDMMC_ERROR                 2
/** The SD card did not answer the command. */
#define SDMMC_ERROR_NORESPONSE      3
/** The SD card is not initialized. */
#define SDMMC_ERROR_NOT_INITIALIZED 4
/** The SD card is busy. */
#define SDMMC_ERROR_BUSY            5
/** The input parameter error */
#define SDMMC_ERROR_PARAM           6
/** The operation is not supported. */
#define SDMMC_ERROR_NOT_SUPPORT     0xFE

/** Support 1-bit bus mode */
#define SDMMC_BUS_1_BIT             0x0UL
/** Support 4-bit bus mode */
#define SDMMC_BUS_4_BIT             0x1UL
/** Support 8-bit bus mode */
#define SDMMC_BUS_8_BIT             0x2UL

/** SD/MMC card block size in bytes. */
#define SDMMC_BLOCK_SIZE            512
/** SD/MMC card block size binary shift value. */
#define SDMMC_BLOCK_SIZE_SHIFT      9

/** SD/MMC command status: ready */
#define SDMMC_CMD_READY             0
/** SD/MMC command status: waiting command end */
#define SDMMC_CMD_PENDING           1
/** SD/MMC command status: error */
#define SDMMC_CMD_ERROR             2

/** SD/MMC Low Level Property: Bus mode */
#define SDMMC_PROP_BUS_MODE       0
/** SD/MMC Low Level Property: High-speed mode */
#define SDMMC_PROP_HS_MODE        1
/** SD/MMC Low Level Property: Boot mode */
#define SDMMC_PROP_BOOT_MODE      2

/*----------------------------------------------------------------------------
 *         Types
 *----------------------------------------------------------------------------*/

/** SD/MMC end-of-transfer callback function. */
typedef void (*SdmmcCallback)(uint8_t status, void *pArg);

/**
 * SD/MMC enumeration data buffers.
 */
typedef struct _SdEnumData {
    uint32_t cid[4]; /**< Card IDentification register */
    uint32_t csd[4]; /**< Card-Specific Data register */
    /** SD SCR (64 bit) + status (512 bit) or
        MMC EXT_CSD(512 bytes) register */
    uint32_t extData[512 / 4];
} SdEnumData;

/**
 * SD SPI mode extended functions.
 */
typedef struct _SdSpiFunctions {
    void *fCmd58;       /**< SD SPI Read OCR */
    void *fCmd59;       /**< SD SPI CRC on/off control */
    void *fStopToken;   /**< SD SPI Stop token */
    void *fDecodeResp;  /**< SD SPI response decoding */
} SdSpiFunctions;

/**
 * Sdmmc command operation settings.
 */
typedef union _SdmmcCmdOperation {
    uint8_t bVal;
    struct {
        unsigned int powerON:1, /**< Do power on initialize */
                sendCmd:1, /**< Send SD/MMC command */
                xfrData:2, /**< Send/Stop data transfer */
                respType:3,/**< Response type */
                crcON:1;   /**< CRC is used (SPI only) */
    } bmBits;
} SdmmcCmdOp;
#define SDMMC_CMD_bmPOWERON     (0x1     )
#define SDMMC_CMD_bmCOMMAND     (0x1 << 1)
#define SDMMC_CMD_bmDATAMASK    (0x3 << 2)
#define SDMMC_CMD_bmNODATA      (0x0 << 2)
#define SDMMC_CMD_RX             0x1
#define SDMMC_CMD_bmDATARX      (0x1 << 2)
#define SDMMC_CMD_TX             0x2
#define SDMMC_CMD_bmDATATX      (0x2 << 2)
#define SDMMC_CMD_bmSTOPXFR     (0x2 << 2)
#define SDMMC_CMD_bmRESPMASK    (0x7 << 4)
#define SDMMC_CMD_bmCRC         (0x1 << 7)
/* Do power on initialize */
#define SDMMC_CMD_POWERONINIT   (SDMMC_CMD_bmPOWERON)
/* Data only, read */
#define SDMMC_CMD_DATARX        (SDMMC_CMD_bmDATARX)
/* Data only, write */
#define SDMMC_CMD_DATATX        (SDMMC_CMD_bmDATATX)
/* Command without data */
#define SDMMC_CMD_CNODATA(R)    ( SDMMC_CMD_bmCOMMAND \
                                |(((R)&0x7)<<4))
/* Command with data, read */
#define SDMMC_CMD_CDATARX(R)    ( SDMMC_CMD_bmCOMMAND \
                                | SDMMC_CMD_bmDATARX \
                                | (((R)&0x7)<<4))
/* Command with data, write */
#define SDMMC_CMD_CDATATX(R)    ( SDMMC_CMD_bmCOMMAND \
                                | SDMMC_CMD_bmDATATX \
                                | (((R)&0x7)<<4))
/* Send Stop token for SPI */
#define SDMMC_CMD_STOPTOKEN     (SDMMC_CMD_bmSTOPXFR)

/**
 * Sdmmc command.
 */
typedef struct _SdmmcCommand {
    /** Optional user-provided callback function. */
    SdmmcCallback callback;
    /** Optional argument to the callback function. */
    void *pArg;
    /** Data buffer, with MCI_DMA_ENABLE defined 1, the buffer can be
     * 1, 2 or 4 bytes aligned. It has to be 4 byte aligned if no DMA.
     */
    uint8_t *pData;
    /** Size of data block in bytes. */
    uint16_t blockSize;
    /** Number of blocks to be transfered */
    uint16_t nbBlock;
    /** Response buffer. */
    uint32_t  *pResp;
    /** Command argument. */
    uint32_t   arg;
    /**< Command index */
    uint8_t    cmd;
    /**< Command operation settings */
    SdmmcCmdOp cmdOp;
    /**< Command return status */
    uint8_t    status;
    /**< Command state */
    volatile uint8_t state;
} SdmmcCommand;

/**
 * \typedef SdCard
 * Sdcard driver structure. It holds the current command being processed and
 * the SD card address.
 */
typedef struct _SdCard
{
    /** Pointer to the underlying HW driver. */
    void *pSdDriver;
    /** Pointer to the extension data for SPI mode */
    void *pSpiExt;

    /** Card IDentification (CID register) */
    uint32_t cid[4];
    /** Card-specific data (CSD register) */
    uint32_t csd[4];
    /** SD SCR(64 bit), Status(512 bit) or
        MMC EXT_CSD(512 bytes) register */
    uint32_t extData[512 / 4];

    /** Card TRANS_SPEED: Max supported transfer speed */
    uint32_t transSpeed;

    /** Card total size */
    uint32_t totalSize;
    /** Card total number of blocks */
    uint32_t blockNr;

    /** Card option command support list */
    uint32_t optCmdBitMap;
    /** Previous access block number for memory. */
    uint32_t preBlock;
    /** Previous access block number for SDIO. */
    uint32_t preSdioBlock;

    /** SD card current access speed. */
    uint32_t accSpeed;
    /** SD card current access address. */
    uint16_t cardAddress;
    /** Card type */
    uint8_t cardType;
    /** Card access bus mode */
    uint8_t busMode;
    /** Card access slot */
    uint8_t cardSlot;
    /** Card State */
    uint8_t state;
} SdCard;


/** \addtogroup sdmmc_struct_cmdarg SD/MMC command arguments
 *  Here lists the command arguments for SD/MMC.
 *  - CMD6 Argument
 *    - \ref _MmcCmd6Arg "MMC CMD6"
 *    - \ref _SdCmd6Arg  "SD CMD6"
 *  - \ref _SdioCmd52Arg CMD52
 *  - \ref _SdioCmd53Arg CMD53
 *      @{*/
/**
 * \typedef MmcCmd6Arg
 * Argument for MMC CMD6
 */
typedef struct _MmcCmd6Arg
{
    uint8_t access;
    uint8_t index;
    uint8_t value;
    uint8_t cmdSet;
} MmcCmd6Arg, MmcSwitchArg;

/**
 * \typedef SdCmd6Arg
 * Argument for SD CMD6
 */
typedef struct _SdCmd6Arg
{
    unsigned int accessMode:4,  /**< [ 3: 0] function group 1, access mode */
             command:4,     /**< [ 7: 4] function group 2, command system */
             reserveFG3:4,  /**< [11: 8] function group 3, 0xF or 0x0 */
             reserveFG4:4,  /**< [15:12] function group 4, 0xF or 0x0 */
             reserveFG5:4,  /**< [19:16] function group 5, 0xF or 0x0 */
             reserveFG6:4,  /**< [23:20] function group 6, 0xF or 0x0 */
             reserved:7,    /**< [30:24] reserved 0 */
             mode:1;        /**< [31   ] Mode, 0: Check, 1: Switch */
} SdCmd6Arg, SdSwitchArg;

/**
 * \typedef SdioCmd52Arg
 * Argument for SDIO CMD52
 */
typedef struct _SdioCmd52Arg
{
    unsigned int data:8,        /**< [ 7: 0] data for writing */
             stuff0:1,      /**< [    8] reserved */
             regAddress:17, /**< [25: 9] register address */
             stuff1:1,      /**< [   26] reserved */
             rawFlag:1,     /**< [   27] Read after Write flag */
             functionNum:3, /**< [30:28] Number of the function */
             rwFlag:1;      /**< [   31] Direction, 1:write, 0:read. */
} SdioCmd52Arg, SdioRwDirectArg;
/**
 * \typedef SdioCmd53Arg
 * Argument for SDIO CMD53
 */
typedef struct _SdioCmd53Arg {
    unsigned int count:9,       /**< [ 8: 0] Byte mode: number of bytes to transfer,
                                                   0 cause 512 bytes transfer.
                                         Block mode: number of blocks to transfer,
                                                    0 set count to infinite. */
             regAddress:17, /**< [25: 9] Start Address I/O register */
             opCode:1,      /**< [   26] 1:Incrementing address, 0: fixed */
             blockMode:1,   /**< [   27] (Optional) 1:block mode */
             functionNum:3, /**< [30:28] Number of the function */
             rwFlag:1;      /**< [   31] Direction, 1:WR, 0:RD */
} SdioCmd53Arg, SdioRwExtArg;
/**     @}*/


/** \addtogroup sdmmc_resp_struct SD/MMC Responses Structs
 *  Here lists the command responses for SD/MMC.
 *  - \ref _SdmmcR1 "R1"
 *  - \ref _SdmmcR3 "R3"
 *  - \ref _MmcR4 "MMC R4", \ref _SdioR4 "SDIO R4"
 *  - \ref _SdmmcR5 "R5"
 *  - \ref _SdR6 "R6"
 *  - \ref _SdR7 "R7"
 *      @{*/
/**
 * Response R1 (normal response command)
 */
typedef struct _SdmmcR1
{
    uint32_t cardStatus;    /**< [32: 0] Response card status flags */
} SdmmcR1, SdmmcR1b;

/**
 * Response R3 (OCR)
 */
typedef struct _SdmmcR3
{
    uint32_t OCR;           /**< [32: 0] OCR register */
} SdmmcR3;

/**
 * Response R4 (MMC Fast I/O CMD39)
 */
typedef struct _MmcR4
{
    unsigned int regData:8,     /**< [ 7: 0] Read register contents */
             regAddr:7,     /**< [14: 8] Register address */
             status:1,      /**< [   15] Status */
             RCA:16;        /**< [31:16] RCA */
} MmcR4;

/**
 * Response R4 (SDIO), no CRC.
 */
typedef struct _SdioR4
{
    unsigned int OCR:24,            /**< [23: 0]       Operation Conditions Register */
             reserved:3,        /**< [26:24]       Reserved */
             memoryPresent:1,   /**< [   27] MP    Set to 1 if contains
                                                   SD memory */
             nbIoFunction:3,    /**< [30:28] NF    Total number of I/O functions
                                                   supported */
             C:1;               /**< [   31] IORDY Set to 1 if card is ready */
} SdioR4;

/**
 * Response R5 (MMC Interrupt request CMD40 / SDIO CMD52)
 */
typedef struct _SdmmcR5
{
    unsigned int data:8,        /**< [ 7: 0] Response data */
             response:8,    /**< [15: 8] Response status flags */
             RCA:16;        /**< [31:16] (MMC) Winning card RCA */
} SdmmcR5;

/**
 * Response R6 (SD RCA)
 */
typedef struct _SdR6
{
    unsigned int status:16,     /**< [15: 0] Response status */
             RCA:16;        /**< [31:16] New published RCA */
} SdR6;
/**
 * Response R7 (Card interface condition)
 */
typedef struct _SdR7 {
    unsigned int checkPatten:8, /**< [ 7: 0] Echo-back of check pattern */
             voltage:4,     /**< [11: 8] Voltage accepted */
             reserved:20;   /**< [31:12] reserved bits */
} SdR7;


uint8_t Sdmmc_SendCommand( Mcid *pMci, MciCmd *pCommand, host_semaphore_type_t* semaphore );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif //#ifndef SDMMC_CMD_H
