/**
 ******************************************************************************
 * @file    eeprom_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    18-Nov-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "eeprom_hal.h"
#include "hw_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define PAGE_SIZE               ((uint32_t)0x4000)  /* Page size = 16KByte */
/* EEPROM emulation start address in Flash (just after the DCT space) */
#define EEPROM_START_ADDRESS    ((uint32_t)0x0800C000)

/* Device voltage range supposed to be [2.7V to 3.6V] */
#define VOLTAGE_RANGE           (uint8_t)VoltageRange_3

/* Pages 0 and 1 base and end addresses */
#define PAGE0_BASE_ADDRESS      ((uint32_t)(EEPROM_START_ADDRESS + 0x0000))
#define PAGE0_END_ADDRESS       ((uint32_t)(EEPROM_START_ADDRESS + (PAGE_SIZE - 1)))
#define PAGE0_ID                FLASH_Sector_3

#define PAGE1_BASE_ADDRESS      ((uint32_t)(EEPROM_START_ADDRESS + PAGE_SIZE))
#define PAGE1_END_ADDRESS       ((uint32_t)(EEPROM_START_ADDRESS + (2 * PAGE_SIZE - 1)))
#define PAGE1_ID                FLASH_Sector_4

/* Used Flash pages for EEPROM emulation */
#define PAGE0                   ((uint16_t)0x0000)
#define PAGE1                   ((uint16_t)0x0001)

/* No valid page define */
#define NO_VALID_PAGE           ((uint16_t)0x00AB)

/* Page status definitions */
#define ERASED                  ((uint16_t)0xFFFF)     /* PAGE is empty */
#define RECEIVE_DATA            ((uint16_t)0xEEEE)     /* PAGE is marked to receive data */
#define VALID_PAGE              ((uint16_t)0x0000)     /* PAGE containing valid data */

/* Valid pages in read and write defines */
#define READ_FROM_VALID_PAGE    ((uint8_t)0x00)
#define WRITE_IN_VALID_PAGE     ((uint8_t)0x01)

/* Page full define */
#define PAGE_FULL               ((uint8_t)0x80)

/* Private macro -------------------------------------------------------------*/
#define EEPROM_SIZE             2048

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
#if !defined(SYSTEM_MINIMAL)
static FLASH_Status EEPROM_Format(void);
static uint16_t EEPROM_FindValidPage(uint8_t Operation);
static uint16_t EEPROM_VerifyPageFullWriteVariable(uint16_t EepromAddress, uint16_t EepromData);
static uint16_t EEPROM_PageTransfer(uint16_t EepromAddress, uint16_t EepromData);
static uint16_t EEPROM_Init(void);
static uint16_t EEPROM_ReadVariable(uint16_t EepromAddress, uint16_t *EepromData);
static uint16_t EEPROM_WriteVariable(uint16_t EepromAddress, uint16_t EepromData);
#endif

void HAL_EEPROM_Init(void)
{
#if !defined(SYSTEM_MINIMAL)
    EEPROM_Init();
#endif
}

size_t HAL_EEPROM_Length()
{
#if !defined(SYSTEM_MINIMAL)
    return EEPROM_SIZE;
#else
    return 0;
#endif
}

uint8_t HAL_EEPROM_Read(uint32_t address)
{
#if !defined(SYSTEM_MINIMAL)
    uint16_t data;

    if ((address < EEPROM_SIZE) && (EEPROM_ReadVariable(address, &data) == 0))
    {
        return data;
    }
#endif
    return 0xFF;
}

void HAL_EEPROM_Write(uint32_t address, uint8_t data)
{
#if !defined(SYSTEM_MINIMAL)
    if (address < EEPROM_SIZE)
    {
        EEPROM_WriteVariable(address, data);
    }
#endif
}

#if !defined(SYSTEM_MINIMAL)


/**
 * @brief  Erases PAGE0 and PAGE1 and writes VALID_PAGE header to PAGE0
 * @param  None
 * @retval Status of the last operation (Flash write or erase) done during
 *         EEPROM formating
 */
static FLASH_Status EEPROM_Format(void)
{
    FLASH_Status FlashStatus = FLASH_COMPLETE;

    /* Erase Page0 */
    FlashStatus = FLASH_EraseSector(PAGE0_ID, VOLTAGE_RANGE);

    /* If erase operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }

    /* Set Page0 as valid page: Write VALID_PAGE at Page0 base address */
    FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);

    /* If program operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }

    /* Erase Page1 */
    FlashStatus = FLASH_EraseSector(PAGE1_ID, VOLTAGE_RANGE);

    /* Return Page1 erase operation status */
    return FlashStatus;
}

/**
 * @brief  Find valid Page for write or read operation
 * @param  Operation: operation to achieve on the valid page.
 *   This parameter can be one of the following values:
 *     @arg READ_FROM_VALID_PAGE: read operation from valid page
 *     @arg WRITE_IN_VALID_PAGE: write operation from valid page
 * @retval Valid page number (PAGE0 or PAGE1) or NO_VALID_PAGE in case
 *   of no valid page was found
 */
static uint16_t EEPROM_FindValidPage(uint8_t Operation)
{
    uint16_t PageStatus0 = 6, PageStatus1 = 6;

    /* Get Page0 actual status */
    PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);

    /* Get Page1 actual status */
    PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);

    /* Write or read operation */
    switch (Operation)
    {
        case WRITE_IN_VALID_PAGE:   /* ---- Write operation ---- */
            if (PageStatus1 == VALID_PAGE)
            {
                /* Page0 receiving data */
                if (PageStatus0 == RECEIVE_DATA)
                {
                    return PAGE0;         /* Page0 valid */
                }
                else
                {
                    return PAGE1;         /* Page1 valid */
                }
            }
            else if (PageStatus0 == VALID_PAGE)
            {
                /* Page1 receiving data */
                if (PageStatus1 == RECEIVE_DATA)
                {
                    return PAGE1;         /* Page1 valid */
                }
                else
                {
                    return PAGE0;         /* Page0 valid */
                }
            }
            else
            {
                return NO_VALID_PAGE;   /* No valid Page */
            }

        case READ_FROM_VALID_PAGE:  /* ---- Read operation ---- */
            if (PageStatus0 == VALID_PAGE)
            {
                return PAGE0;           /* Page0 valid */
            }
            else if (PageStatus1 == VALID_PAGE)
            {
                return PAGE1;           /* Page1 valid */
            }
            else
            {
                return NO_VALID_PAGE ;  /* No valid Page */
            }

        default:
            return PAGE0;             /* Page0 valid */
    }
}

/**
 * @brief  Verify if active page is full and Writes variable in EEPROM.
 * @param  EepromAddress: 16 bit virtual address of the variable
 * @param  EepromData: 16 bit data to be written as variable value
 * @retval Success or error status:
 *           - FLASH_COMPLETE: on success
 *           - PAGE_FULL: if valid page is full
 *           - NO_VALID_PAGE: if no valid page was found
 *           - Flash error code: on write Flash error
 */
static uint16_t EEPROM_VerifyPageFullWriteVariable(uint16_t EepromAddress, uint16_t EepromData)
{
    FLASH_Status FlashStatus = FLASH_COMPLETE;
    uint16_t ValidPage = PAGE0;
    uint32_t Address = PAGE0_BASE_ADDRESS, PageEndAddress = PAGE0_END_ADDRESS;

    /* Get valid Page for write operation */
    ValidPage = EEPROM_FindValidPage(WRITE_IN_VALID_PAGE);

    /* Check if there is no valid page */
    if (ValidPage == NO_VALID_PAGE)
    {
        return  NO_VALID_PAGE;
    }

    /* Get the valid Page start Address */
    Address = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE));

    /* Get the valid Page end Address */
    PageEndAddress = (uint32_t)((EEPROM_START_ADDRESS - 2) + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

    /* Check each active page address starting from begining */
    while (Address < PageEndAddress)
    {
        /* Verify if Address and Address+2 contents are 0xFFFFFFFF */
        if ((*(__IO uint32_t*)Address) == 0xFFFFFFFF)
        {
            /* Set variable data */
            FlashStatus = FLASH_ProgramHalfWord(Address, EepromData);
            /* If program operation was failed, a Flash error code is returned */
            if (FlashStatus != FLASH_COMPLETE)
            {
                return FlashStatus;
            }
            /* Set variable virtual address */
            FlashStatus = FLASH_ProgramHalfWord(Address + 2, EepromAddress);
            /* Return program operation status */
            return FlashStatus;
        }
        else
        {
            /* Next address location */
            Address = Address + 4;
        }
    }

    /* Return PAGE_FULL in case the valid page is full */
    return PAGE_FULL;
}

/**
 * @brief  Transfers last updated variables data from the full Page to
 *   an empty one.
 * @param  EepromAddress: 16 bit virtual address of the variable
 * @param  EepromData: 16 bit data to be written as variable value
 * @retval Success or error status:
 *           - FLASH_COMPLETE: on success
 *           - PAGE_FULL: if valid page is full
 *           - NO_VALID_PAGE: if no valid page was found
 *           - Flash error code: on write Flash error
 */
static uint16_t EEPROM_PageTransfer(uint16_t EepromAddress, uint16_t EepromData)
{
    FLASH_Status FlashStatus = FLASH_COMPLETE;
    uint32_t NewPageAddress = PAGE0_BASE_ADDRESS;
    uint16_t OldPageId=0;
    uint16_t ValidPage = PAGE0, virtualAddress = 0;
    uint16_t EepromStatus = 0, ReadStatus = 0;
    uint16_t EepromDataVar = 0;

    /* Get active Page for read operation */
    ValidPage = EEPROM_FindValidPage(READ_FROM_VALID_PAGE);

    if (ValidPage == PAGE1)       /* Page1 valid */
    {
        /* New page address where variable will be moved to */
        NewPageAddress = PAGE0_BASE_ADDRESS;

        /* Old page ID where variable will be taken from */
        OldPageId = PAGE1_ID;
    }
    else if (ValidPage == PAGE0)  /* Page0 valid */
    {
        /* New page address where variable will be moved to */
        NewPageAddress = PAGE1_BASE_ADDRESS;

        /* Old page ID where variable will be taken from */
        OldPageId = PAGE0_ID;
    }
    else
    {
        return NO_VALID_PAGE;       /* No valid Page */
    }

    /* Set the new Page status to RECEIVE_DATA status */
    FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, RECEIVE_DATA);
    /* If program operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }

    /* Write the variable passed as parameter in the new active page */
    EepromStatus = EEPROM_VerifyPageFullWriteVariable(EepromAddress, EepromData);
    /* If program operation was failed, a Flash error code is returned */
    if (EepromStatus != FLASH_COMPLETE)
    {
        return EepromStatus;
    }

    /* Transfer process: transfer variables from old to the new active page */
    for (virtualAddress = 0; virtualAddress < EEPROM_SIZE; virtualAddress++)
    {
        if (virtualAddress != EepromAddress)  /* Check each variable except the one passed as parameter */
        {
            /* Read the other last variable updates */
            ReadStatus = EEPROM_ReadVariable(virtualAddress, &EepromDataVar);
            /* In case variable corresponding to the virtual address was found */
            if (ReadStatus != 0x1)
            {
                /* Transfer the variable to the new active page */
                EepromStatus = EEPROM_VerifyPageFullWriteVariable(virtualAddress, EepromDataVar);
                /* If program operation was failed, a Flash error code is returned */
                if (EepromStatus != FLASH_COMPLETE)
                {
                    return EepromStatus;
                }
            }
        }
    }

    /* Erase the old Page: Set old Page status to ERASED status */
    FlashStatus = FLASH_EraseSector(OldPageId, VOLTAGE_RANGE);
    /* If erase operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }

    /* Set new Page status to VALID_PAGE status */
    FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, VALID_PAGE);
    /* If program operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }

    /* Return last operation flash status */
    return FlashStatus;
}

/**
 * @brief  Restore the pages to a known good state in case of page's status
 *   corruption after a power loss.
 * @param  None.
 * @retval - Flash error code: on write Flash error
 *         - FLASH_COMPLETE: on success
 */
static uint16_t EEPROM_Init(void)
{
    uint16_t PageStatus0 = 6, PageStatus1 = 6;
    uint16_t virtualAddress = 0;
    uint16_t EepromStatus = 0, ReadStatus = 0;
    int16_t x = -1;
    uint16_t  FlashStatus;
    uint16_t EepromDataVar = 0;

    /* Unlock the Flash Program Erase controller */
    FLASH_Unlock();

    /* Calling this is here is critical on STM32F2 Devices Else Flash Operation Fails */
    FLASH_ClearFlags();

    /* Get Page0 status */
    PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);
    /* Get Page1 status */
    PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);

    /* Check for invalid header states and repair if necessary */
    switch (PageStatus0)
    {
        case ERASED:
            if (PageStatus1 == VALID_PAGE) /* Page0 erased, Page1 valid */
            {
                /* Erase Page0 */
                FlashStatus = FLASH_EraseSector(PAGE0_ID,VOLTAGE_RANGE);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            else if (PageStatus1 == RECEIVE_DATA) /* Page0 erased, Page1 receive */
            {
                /* Erase Page0 */
                FlashStatus = FLASH_EraseSector(PAGE0_ID, VOLTAGE_RANGE);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
                /* Mark Page1 as valid */
                FlashStatus = FLASH_ProgramHalfWord(PAGE1_BASE_ADDRESS, VALID_PAGE);
                /* If program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            else /* First EEPROM access (Page0&1 are erased) or invalid state -> format EEPROM */
            {
                /* Erase both Page0 and Page1 and set Page0 as valid page */
                FlashStatus = EEPROM_Format();
                /* If erase/program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            break;

        case RECEIVE_DATA:
            if (PageStatus1 == VALID_PAGE) /* Page0 receive, Page1 valid */
            {
                /* Transfer data from Page1 to Page0 */
                for (virtualAddress = 0; virtualAddress < EEPROM_SIZE; virtualAddress++)
                {
                    if (( *(__IO uint16_t*)(PAGE0_BASE_ADDRESS + 6)) == virtualAddress)
                    {
                        x = virtualAddress;
                    }
                    if (virtualAddress != x)
                    {
                        /* Read the last variables' updates */
                        ReadStatus = EEPROM_ReadVariable(virtualAddress, &EepromDataVar);
                        /* In case variable corresponding to the virtual address was found */
                        if (ReadStatus != 0x1)
                        {
                            /* Transfer the variable to the Page0 */
                            EepromStatus = EEPROM_VerifyPageFullWriteVariable(virtualAddress, EepromDataVar);
                            /* If program operation was failed, a Flash error code is returned */
                            if (EepromStatus != FLASH_COMPLETE)
                            {
                                return EepromStatus;
                            }
                        }
                    }
                }
                /* Mark Page0 as valid */
                FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);
                /* If program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
                /* Erase Page1 */
                FlashStatus = FLASH_EraseSector(PAGE1_ID, VOLTAGE_RANGE);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            else if (PageStatus1 == ERASED) /* Page0 receive, Page1 erased */
            {
                /* Erase Page1 */
                FlashStatus = FLASH_EraseSector(PAGE1_ID, VOLTAGE_RANGE);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
                /* Mark Page0 as valid */
                FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);
                /* If program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            else /* Invalid state -> format eeprom */
            {
                /* Erase both Page0 and Page1 and set Page0 as valid page */
                FlashStatus = EEPROM_Format();
                /* If erase/program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            break;

        case VALID_PAGE:
            if (PageStatus1 == VALID_PAGE) /* Invalid state -> format eeprom */
            {
                /* Erase both Page0 and Page1 and set Page0 as valid page */
                FlashStatus = EEPROM_Format();
                /* If erase/program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            else if (PageStatus1 == ERASED) /* Page0 valid, Page1 erased */
            {
                /* Erase Page1 */
                FlashStatus = FLASH_EraseSector(PAGE1_ID, VOLTAGE_RANGE);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            else /* Page0 valid, Page1 receive */
            {
                /* Transfer data from Page0 to Page1 */
                for (virtualAddress = 0; virtualAddress < EEPROM_SIZE; virtualAddress++)
                {
                    if ((*(__IO uint16_t*)(PAGE1_BASE_ADDRESS + 6)) == virtualAddress)
                    {
                        x = virtualAddress;
                    }
                    if (virtualAddress != x)
                    {
                        /* Read the last variables' updates */
                        ReadStatus = EEPROM_ReadVariable(virtualAddress, &EepromDataVar);
                        /* In case variable corresponding to the virtual address was found */
                        if (ReadStatus != 0x1)
                        {
                            /* Transfer the variable to the Page1 */
                            EepromStatus = EEPROM_VerifyPageFullWriteVariable(virtualAddress, EepromDataVar);
                            /* If program operation was failed, a Flash error code is returned */
                            if (EepromStatus != FLASH_COMPLETE)
                            {
                                return EepromStatus;
                            }
                        }
                    }
                }
                /* Mark Page1 as valid */
                FlashStatus = FLASH_ProgramHalfWord(PAGE1_BASE_ADDRESS, VALID_PAGE);
                /* If program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
                /* Erase Page0 */
                FlashStatus = FLASH_EraseSector(PAGE0_ID, VOLTAGE_RANGE);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            break;

        default:  /* Any other state -> format eeprom */
            /* Erase both Page0 and Page1 and set Page0 as valid page */
            FlashStatus = EEPROM_Format();
            /* If erase/program operation was failed, a Flash error code is returned */
            if (FlashStatus != FLASH_COMPLETE)
            {
                return FlashStatus;
            }
            break;
    }

    return FLASH_COMPLETE;
}

/**
 * @brief  Returns the last stored variable data, if found, which correspond to
 *   the passed virtual address
 * @param  EepromAddress: Variable virtual address
 * @param  EepromData: Global variable contains the read variable value
 * @retval Success or error status:
 *           - 0: if variable was found
 *           - 1: if the variable was not found
 *           - NO_VALID_PAGE: if no valid page was found.
 */
static uint16_t EEPROM_ReadVariable(uint16_t EepromAddress, uint16_t *EepromData)
{
    uint16_t ValidPage = PAGE0;
    uint16_t AddressValue = 0, ReadStatus = 1;
    uint32_t Address = PAGE0_BASE_ADDRESS, PageStartAddress = PAGE0_BASE_ADDRESS;

    /* Get active Page for read operation */
    ValidPage = EEPROM_FindValidPage(READ_FROM_VALID_PAGE);

    /* Check if there is no valid page */
    if (ValidPage == NO_VALID_PAGE)
    {
        return  NO_VALID_PAGE;
    }

    /* Get the valid Page start Address */
    PageStartAddress = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE));

    /* Get the valid Page end Address */
    Address = (uint32_t)((EEPROM_START_ADDRESS - 2) + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

    /* Check each active page address starting from end */
    while (Address > (PageStartAddress + 2))
    {
        /* Get the current location content to be compared with virtual address */
        AddressValue = (*(__IO uint16_t*)Address);

        /* Compare the read address with the virtual address */
        if (AddressValue == EepromAddress)
        {
            /* Get content of Address-2 which is variable value */
            *EepromData = (*(__IO uint16_t*)(Address - 2));

            /* In case variable value is read, reset ReadStatus flag */
            ReadStatus = 0;

            break;
        }
        else
        {
            /* Next address location */
            Address = Address - 4;
        }
    }

    /* Return ReadStatus value: (0: variable exist, 1: variable doesn't exist) */
    return ReadStatus;
}

/**
 * @brief  Writes/updates variable data in EEPROM.
 * @param  EepromAddress: Variable virtual address
 * @param  EepromData: 16 bit data to be written
 * @retval Success or error status:
 *           - FLASH_COMPLETE: on success
 *           - PAGE_FULL: if valid page is full
 *           - NO_VALID_PAGE: if no valid page was found
 *           - Flash error code: on write Flash error
 */
static uint16_t EEPROM_WriteVariable(uint16_t EepromAddress, uint16_t EepromData)
{
    uint16_t Status = 0;

    /* Unlock the Flash Program Erase controller */
    FLASH_Unlock();

    /* Calling this is here is critical on STM32F2 Devices Else Flash Operation Fails */
    FLASH_ClearFlags();

    /* Write the variable virtual address and value in the EEPROM */
    Status = EEPROM_VerifyPageFullWriteVariable(EepromAddress, EepromData);

    /* In case the EEPROM active page is full */
    if (Status == PAGE_FULL)
    {
        /* Perform Page transfer */
        Status = EEPROM_PageTransfer(EepromAddress, EepromData);
    }

    /* Return last operation status */
    return Status;
}

#endif