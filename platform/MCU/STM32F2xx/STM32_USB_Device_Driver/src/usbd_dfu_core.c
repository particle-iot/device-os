/**
  ******************************************************************************
  * @file    usbd_dfu_core.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file provides the high layer firmware functions to manage the
  *          following functionalities of the USB DFU Class:
  *           - Initialization and Configuration of high and low layer
  *           - Enumeration as DFU Device (and enumeration for each implemented memory interface)
  *           - Transfers to/from memory interfaces
  *           - Easy-to-customize "plug-in-like" modules for adding/removing memory interfaces.
  *           - Error management
  *
  *  @verbatim
  *
  *          ===================================================================
  *                                DFU Class Driver Description
  *          ===================================================================
  *           This driver manages the DFU class V1.1 following the "Device Class Specification for
  *           Device Firmware Upgrade Version 1.1 Aug 5, 2004".
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Enumeration as DFU device (in DFU mode only)
  *             - Requests management (supporting ST DFU sub-protocol)
  *             - Memory operations management (Download/Upload/Erase/Detach/GetState/GetStatus)
  *             - DFU state machine implementation.
  *
  *           @note
  *            ST DFU sub-protocol is compliant with DFU protocol and use sub-requests to manage
  *            memory addressing, commands processing, specific memories operations (ie. Erase) ...
  *            As required by the DFU specification, only endpoint 0 is used in this application.
  *            Other endpoints and functions may be added to the application (ie. DFU ...)
  *
  *           These aspects may be enriched or modified for a specific user application.
  *
  *           This driver doesn't implement the following aspects of the specification
  *           (but it is possible to manage these features with some modifications on this driver):
  *             - Manifestation Tolerant mode
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_dfu_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "usb_bsp.h"
#include "usbd_dfu_mal.h"
#include "usbd_wcid.h"
#include <string.h>

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup usbd_dfu
  * @brief usbd core module
  * @{
  */

/** @defgroup usbd_dfu_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup usbd_dfu_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup usbd_dfu_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup usbd_dfu_Private_FunctionPrototypes
  * @{
  */

/*********************************************
   DFU Device library callbacks
 *********************************************/
static uint8_t  usbd_dfu_Init     (void  *pdev,
                                  uint8_t cfgidx);

static uint8_t  usbd_dfu_DeInit   (void  *pdev,
                                  uint8_t cfgidx);

static uint8_t  usbd_dfu_Setup    (void  *pdev,
                                  USB_SETUP_REQ *req);

static uint8_t  EP0_TxSent        (void  *pdev);

static uint8_t  EP0_RxReady       (void  *pdev);


static uint8_t  *USBD_DFU_GetCfgDesc (uint8_t speed,
                                      uint16_t *length);


#ifdef USB_OTG_HS_CORE
static uint8_t  *USBD_DFU_GetOtherCfgDesc (uint8_t speed,
                                      uint16_t *length);
#endif

static uint8_t* USBD_DFU_GetUsrStringDesc (uint8_t speed,
                                           uint8_t index ,
                                           uint16_t *length);

/*********************************************
   DFU Requests management functions
 *********************************************/
static void DFU_Req_DETACH    (void *pdev,
                               USB_SETUP_REQ *req);

static void DFU_Req_DNLOAD    (void *pdev,
                               USB_SETUP_REQ *req);

static void DFU_Req_UPLOAD    (void *pdev,
                               USB_SETUP_REQ *req);

static void DFU_Req_GETSTATUS (void *pdev);

static void DFU_Req_CLRSTATUS (void *pdev);

static void DFU_Req_GETSTATE  (void *pdev);

static void DFU_Req_ABORT     (void *pdev);

static void DFU_LeaveDFUMode  (void *pdev);

/**
  * @}
  */

/** @defgroup usbd_dfu_Private_Variables
  * @{
  */
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t usbd_dfu_CfgDesc[USB_DFU_CONFIG_DESC_SIZ] __ALIGN_END ;


#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t usbd_dfu_OtherCfgDesc[USB_DFU_CONFIG_DESC_SIZ] __ALIGN_END ;

/* The list of Interface String descriptor pointers is defined in usbd_dfu_mal.c
  file. This list can be updated whenever a memory has to be added or removed */
extern const uint8_t* usbd_dfu_StringDesc[];

/* State Machine variables */
uint8_t DeviceState;
uint8_t DeviceStatus[6] = {0};
uint32_t Manifest_State = Manifest_complete;
/* Data Management variables */
static uint32_t wBlockNum = 0, wlength = 0;
static uint32_t Pointer = APP_DEFAULT_ADD;  /* Base Address to Erase, Program or Read */
static __IO uint32_t  usbd_dfu_AltSet = 0;

extern uint8_t MAL_Buffer[];

/* DFU interface class callbacks structure */
USBD_Class_cb_TypeDef  DFU_cb =
{
  usbd_dfu_Init,
  usbd_dfu_DeInit,
  usbd_dfu_Setup,
  EP0_TxSent,
  EP0_RxReady,
  NULL, /* DataIn, */
  NULL, /* DataOut, */
  NULL, /*SOF */
  NULL,
  NULL,
  USBD_DFU_GetCfgDesc,
#ifdef USB_OTG_HS_CORE
  USBD_DFU_GetOtherCfgDesc, /* use same cobfig as per FS */
#endif
  USBD_DFU_GetUsrStringDesc,
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* USB DFU device Configuration Descriptor */
__ALIGN_BEGIN uint8_t usbd_dfu_CfgDesc[USB_DFU_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09, /* bLength: Configuation Descriptor size */
  USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
  USB_DFU_CONFIG_DESC_SIZ,
  /* wTotalLength: Bytes returned */
  0x00,
  0x01,         /*bNumInterfaces: 1 interface*/
  0x01,         /*bConfigurationValue: Configuration value*/
  0x02,         /*iConfiguration: Index of string descriptor describing the configuration*/
  0xC0,         /*bmAttributes: bus powered and Supprts Remote Wakeup */
  0x32,         /*MaxPower 100 mA: this current is used for detecting Vbus*/
  /* 09 */

  /**********  Descriptor of DFU interface 0 Alternate setting 0 **************/
  USBD_DFU_IF_DESC(0), /* This interface is mandatory for all devices */

#if (USBD_DFU_INT_NUM > 1)
  /**********  Descriptor of DFU interface 0 Alternate setting 1 **************/
  USBD_DFU_IF_DESC(1),
#endif /* (USBD_DFU_INT_NUM > 1) */

#if (USBD_DFU_INT_NUM > 2)
  /**********  Descriptor of DFU interface 0 Alternate setting 2 **************/
  USBD_DFU_IF_DESC(2),
#endif /* (USBD_DFU_INT_NUM > 2) */

#if (USBD_DFU_INT_NUM > 3)
  /**********  Descriptor of DFU interface 0 Alternate setting 3 **************/
  USBD_DFU_IF_DESC(3),
#endif /* (USBD_DFU_INT_NUM > 3) */

#if (USBD_DFU_INT_NUM > 4)
  /**********  Descriptor of DFU interface 0 Alternate setting 4 **************/
  USBD_DFU_IF_DESC(4),
#endif /* (USBD_DFU_INT_NUM > 4) */

#if (USBD_DFU_INT_NUM > 5)
  /**********  Descriptor of DFU interface 0 Alternate setting 5 **************/
  USBD_DFU_IF_DESC(5),
#endif /* (USBD_DFU_INT_NUM > 5) */

#if (USBD_DFU_INT_NUM > 6)
#error "ERROR: usbd_dfu_core.c: Modify the file to support more descriptors!"
#endif /* (USBD_DFU_INT_NUM > 6) */

  /******************** DFU Functional Descriptor********************/
  0x09,   /*blength = 9 Bytes*/
  DFU_DESCRIPTOR_TYPE,   /* DFU Functional Descriptor*/
  0x0B,   /*bmAttribute
                bitCanDnload             = 1      (bit 0)
                bitCanUpload             = 1      (bit 1)
                bitManifestationTolerant = 0      (bit 2)
                bitWillDetach            = 1      (bit 3)
                Reserved                          (bit4-6)
                bitAcceleratedST         = 0      (bit 7)*/
  0xFF,   /*DetachTimeOut= 255 ms*/
  0x00,
  /*WARNING: In DMA mode the multiple MPS packets feature is still not supported
   ==> In this case, when using DMA XFERSIZE should be set to 64 in usbd_conf.h */
  TRANSFER_SIZE_BYTES(XFERSIZE),       /* TransferSize = 1024 Byte*/
  0x1A,                                /* bcdDFUVersion*/
  0x01
  /***********************************************************/
  /* 9*/
} ;

#ifdef USE_USB_OTG_HS
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

__ALIGN_BEGIN uint8_t usbd_dfu_OtherCfgDesc[USB_DFU_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09, /* bLength: Configuation Descriptor size */
  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION, /* bDescriptorType: Configuration */
  USB_DFU_CONFIG_DESC_SIZ,
  /* wTotalLength: Bytes returned */
  0x00,
  0x01,         /*bNumInterfaces: 1 interface*/
  0x01,         /*bConfigurationValue: Configuration value*/
  0x02,         /*iConfiguration: Index of string descriptor describing the configuration*/
  0xC0,         /*bmAttributes: bus powered and Supprts Remote Wakeup */
  0x32,         /*MaxPower 100 mA: this current is used for detecting Vbus*/
  /* 09 */

  /**********  Descriptor of DFU interface 0 Alternate setting 0 **************/
  USBD_DFU_IF_DESC(0), /* This interface is mandatory for all devices */

#if (USBD_DFU_INT_NUM > 1)
  /**********  Descriptor of DFU interface 0 Alternate setting 1 **************/
  USBD_DFU_IF_DESC(1),
#endif /* (USBD_DFU_INT_NUM > 1) */

#if (USBD_DFU_INT_NUM > 2)
  /**********  Descriptor of DFU interface 0 Alternate setting 2 **************/
  USBD_DFU_IF_DESC(2),
#endif /* (USBD_DFU_INT_NUM > 2) */

#if (USBD_DFU_INT_NUM > 3)
  /**********  Descriptor of DFU interface 0 Alternate setting 3 **************/
  USBD_DFU_IF_DESC(3),
#endif /* (USBD_DFU_INT_NUM > 3) */

#if (USBD_DFU_INT_NUM > 4)
  /**********  Descriptor of DFU interface 0 Alternate setting 4 **************/
  USBD_DFU_IF_DESC(4),
#endif /* (USBD_DFU_INT_NUM > 4) */

#if (USBD_DFU_INT_NUM > 5)
  /**********  Descriptor of DFU interface 0 Alternate setting 5 **************/
  USBD_DFU_IF_DESC(5),
#endif /* (USBD_DFU_INT_NUM > 5) */

#if (USBD_DFU_INT_NUM > 6)
#error "ERROR: usbd_dfu_core.c: Modify the file to support more descriptors!"
#endif /* (USBD_DFU_INT_NUM > 6) */

  /******************** DFU Functional Descriptor********************/
  0x09,   /*blength = 9 Bytes*/
  DFU_DESCRIPTOR_TYPE,   /* DFU Functional Descriptor*/
  0x0B,   /*bmAttribute
                bitCanDnload             = 1      (bit 0)
                bitCanUpload             = 1      (bit 1)
                bitManifestationTolerant = 0      (bit 2)
                bitWillDetach            = 1      (bit 3)
                Reserved                          (bit4-6)
                bitAcceleratedST         = 0      (bit 7)*/
  0xFF,   /*DetachTimeOut= 255 ms*/
  0x00,
  /*WARNING: In DMA mode the multiple MPS packets feature is still not supported
   ==> In this case, when using DMA XFERSIZE should be set to 64 in usbd_conf.h */
  TRANSFER_SIZE_BYTES(XFERSIZE),       /* TransferSize = 1024 Byte*/
  0x1A,                                /* bcdDFUVersion*/
  0x01
  /***********************************************************/
  /* 9*/
};
#endif /* USE_USB_OTG_HS */

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif

__ALIGN_BEGIN static uint8_t usbd_dfu_Desc[USB_DFU_DESC_SIZ] __ALIGN_END =
{
  0x09,   /*blength = 9 Bytes*/
  DFU_DESCRIPTOR_TYPE,   /* DFU Functional Descriptor*/
  0x0B,   /*bmAttribute
                bitCanDnload             = 1      (bit 0)
                bitCanUpload             = 1      (bit 1)
                bitManifestationTolerant = 0      (bit 2)
                bitWillDetach            = 1      (bit 3)
                Reserved                          (bit4-6)
                bitAcceleratedST         = 0      (bit 7)*/
  0xFF,   /*DetachTimeOut= 255 ms*/
  0x00,
  /*WARNING: In DMA mode the multiple MPS packets feature is still not supported
   ==> In this case, when using DMA XFERSIZE should be set to 64 in usbd_conf.h */
  TRANSFER_SIZE_BYTES(XFERSIZE),  /* TransferSize = 1024 Byte*/
  0x1A,                     /* bcdDFUVersion*/
  0x01
};
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

/* Extended Compat ID OS Descriptor */
static const uint8_t USBD_DFU_MsftExtCompatIdOsDescr[] = {
    USB_WCID_EXT_COMPAT_ID_OS_DESCRIPTOR(
        0x00,
        USB_WCID_DATA('W', 'I', 'N', 'U', 'S', 'B', '\0', '\0'),
        USB_WCID_DATA(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
    )
};

/* Extended Properties OS Descriptor */
static const uint8_t USBD_DFU_MsftExtPropOsDescr[] = {
    USB_WCID_EXT_PROP_OS_DESCRIPTOR(
        USB_WCID_DATA(
            /* bPropertyData "{37fb5f90-1a34-4929-933b-8a27e1850033}" */
            '{', 0x00, '3', 0x00, '7', 0x00, 'f', 0x00, 'b', 0x00,
            '5', 0x00, 'f', 0x00, '9', 0x00, '0', 0x00, '-', 0x00,
            '1', 0x00, 'a', 0x00, '3', 0x00, '4', 0x00, '-', 0x00,
            '4', 0x00, '9', 0x00, '2', 0x00, '9', 0x00, '-', 0x00,
            '9', 0x00, '3', 0x00, '3', 0x00, 'b', 0x00, '-', 0x00,
            '8', 0x00, 'a', 0x00, '2', 0x00, '7', 0x00, 'e', 0x00,
            '1', 0x00, '8', 0x00, '5', 0x00, '0', 0x00, '0', 0x00,
            '3', 0x00, '3', 0x00, '}'
        )
    )
};


/**
  * @}
  */

/** @defgroup usbd_dfu_Private_Functions
  * @{
  */

/**
  * @brief  usbd_dfu_Init
  *         Initializes the DFU interface.
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_dfu_Init (void  *pdev,
                               uint8_t cfgidx)
{
  /* Initilialize the MAL(Media Access Layer) */
  MAL_Init();

  /* Initialize the state of the DFU interface */
  DeviceState = STATE_dfuIDLE;
  DeviceStatus[4] = DeviceState;

  return USBD_OK;
}

/**
  * @brief  usbd_dfu_Init
  *         De-initializes the DFU layer.
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_dfu_DeInit (void  *pdev,
                                 uint8_t cfgidx)
{
  /* Restore default state */
  DeviceState = STATE_dfuIDLE;
  DeviceStatus[4] = DeviceState;
  wBlockNum = 0;
  wlength = 0;

  /* DeInitilialize the MAL(Media Access Layer) */
  MAL_DeInit();

  return USBD_OK;
}

uint8_t USBD_DFU_Handle_Msft_Request(void* pdev, USB_SETUP_REQ* req) {
  if (req->wIndex == 0x0004) {
    USBD_CtlSendData(pdev, USBD_DFU_MsftExtCompatIdOsDescr, req->wLength);
  } else if (req->wIndex == 0x0005) {
    if ((req->wValue & 0xff) == 0x00) {
      USBD_CtlSendData(pdev, USBD_DFU_MsftExtPropOsDescr, req->wLength);
    } else {
      // Send dummy
      uint8_t dummy[10] = {0};
      USBD_CtlSendData(pdev, dummy, req->wLength);
    }
  } else {
    return USBD_FAIL;
  }

  return USBD_OK;
}


/**
  * @brief  usbd_dfu_Setup
  *         Handles the DFU request parsing.
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  usbd_dfu_Setup (void  *pdev,
                                USB_SETUP_REQ *req)
{
  uint16_t len = 0;
  uint8_t  *pbuf = NULL;

  if ((req->bRequest == 0xee && req->bmRequest == 0b11000001 && req->wIndex == 0x0005) ||
      (req->bRequest == 0xee && req->bmRequest == 0xc0 && req->wIndex == 0x0004)) {
    return USBD_DFU_Handle_Msft_Request(pdev, req);
  }

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    /* DFU Class Requests -------------------------------*/
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {
    case DFU_DNLOAD:
      DFU_Req_DNLOAD(pdev, req);
      break;

    case DFU_UPLOAD:
      DFU_Req_UPLOAD(pdev, req);
      break;

    case DFU_GETSTATUS:
      DFU_Req_GETSTATUS(pdev);
      break;

    case DFU_CLRSTATUS:
      DFU_Req_CLRSTATUS(pdev);
      break;

    case DFU_GETSTATE:
      DFU_Req_GETSTATE(pdev);
      break;

    case DFU_ABORT:
      DFU_Req_ABORT(pdev);
      break;

    case DFU_DETACH:
      DFU_Req_DETACH(pdev, req);
      break;

    default:
      USBD_CtlError (pdev, req);
      return USBD_FAIL;
    }
    break;

    /* Standard Requests -------------------------------*/
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR:
      if( (req->wValue >> 8) == DFU_DESCRIPTOR_TYPE)
      {
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
        pbuf = usbd_dfu_Desc;
#else
        pbuf = usbd_dfu_CfgDesc + 9 + (9 * USBD_DFU_INT_NUM);
#endif
        len = MIN(USB_DFU_DESC_SIZ , req->wLength);
      }

      USBD_CtlSendData (pdev,
                        pbuf,
                        len);
      break;

    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        (uint8_t *)&usbd_dfu_AltSet,
                        1);
      break;

    case USB_REQ_SET_INTERFACE :
      if ((uint8_t)(req->wValue) < USBD_DFU_INT_NUM)
      {
        usbd_dfu_AltSet = (uint8_t)(req->wValue);
      }
      else
      {
        /* Call the error management function (command will be nacked */
        USBD_CtlError (pdev, req);
      }
      break;
    }
  }
  return USBD_OK;
}

/**
  * @brief  EP0_TxSent
  *         Handles the DFU control endpoint data IN stage.
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  EP0_TxSent (void  *pdev)
{
  uint32_t Addr;
  USB_SETUP_REQ req;

  if (DeviceState == STATE_dfuDNBUSY)
  {
    /* Decode the Special Command*/
    if (wBlockNum == 0)
    {
      if ((MAL_Buffer[0] ==  CMD_GETCOMMANDS) && (wlength == 1))
      {}
      else if  (( MAL_Buffer[0] ==  CMD_SETADDRESSPOINTER ) && (wlength == 5))
      {
        Pointer  = MAL_Buffer[1];
        Pointer += MAL_Buffer[2] << 8;
        Pointer += MAL_Buffer[3] << 16;
        Pointer += MAL_Buffer[4] << 24;
      }
      else if (( MAL_Buffer[0] ==  CMD_ERASE ) && (wlength == 5))
      {
        Pointer  = MAL_Buffer[1];
        Pointer += MAL_Buffer[2] << 8;
        Pointer += MAL_Buffer[3] << 16;
        Pointer += MAL_Buffer[4] << 24;
        uint16_t status = MAL_Erase(usbd_dfu_AltSet, Pointer);
        if (status != MAL_OK) {
          /* Call the error management function (command will be nacked) */
          req.bmRequest = 0;
          req.wLength = 1;
          USBD_CtlError (pdev, &req);
        }
      }
      else
      {
        /* Reset the global length and block number */
        wlength = 0;
        wBlockNum = 0;
        /* Call the error management function (command will be nacked) */
        req.bmRequest = 0;
        req.wLength = 1;
        USBD_CtlError (pdev, &req);
      }
    }
    /* Regular Download Command */
    else if (wBlockNum > 1)
    {
      /* Decode the required address */
      Addr = ((wBlockNum - 2) * XFERSIZE) + Pointer;

      /* Preform the write operation */
      uint16_t status = MAL_Write(usbd_dfu_AltSet, Addr, wlength);
      if (status != MAL_OK) {
        /* Call the error management function (command will be nacked) */
        req.bmRequest = 0;
        req.wLength = 1;
        USBD_CtlError (pdev, &req);
      }
    }
    /* Reset the global lenght and block number */
    wlength = 0;
    wBlockNum = 0;

    /* Update the state machine */
    DeviceState =  STATE_dfuDNLOAD_SYNC;
    DeviceStatus[4] = DeviceState;
    return USBD_OK;
  }
  else if (DeviceState == STATE_dfuMANIFEST)/* Manifestation in progress*/
  {
    /* Start leaving DFU mode */
    DFU_LeaveDFUMode(pdev);
  }

  return USBD_OK;
}

/**
  * @brief  EP0_RxReady
  *         Handles the DFU control endpoint data OUT stage.
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  EP0_RxReady (void  *pdev)
{
  return USBD_OK;
}


/******************************************************************************
     DFU Class requests management
******************************************************************************/
/**
  * @brief  DFU_Req_DETACH
  *         Handles the DFU DETACH request.
  * @param  pdev: device instance
  * @param  req: pointer to the request structure.
  * @retval None.
  */
static void DFU_Req_DETACH(void *pdev, USB_SETUP_REQ *req)
{
  if (DeviceState == STATE_dfuIDLE || DeviceState == STATE_dfuDNLOAD_SYNC
      || DeviceState == STATE_dfuDNLOAD_IDLE || DeviceState == STATE_dfuMANIFEST_SYNC
        || DeviceState == STATE_dfuUPLOAD_IDLE )
  {
    /* Update the state machine */
    DeviceState = STATE_dfuIDLE;
    DeviceStatus[4] = DeviceState;
    wBlockNum = 0;
    wlength = 0;
  }

  /* Check the detach capability in the DFU functional descriptor */
  if ((usbd_dfu_CfgDesc[12 + (9 * USBD_DFU_INT_NUM)]) & DFU_DETACH_MASK)
  {
    /* Perform an Attach-Detach operation on USB bus */
    DCD_DevDisconnect (pdev);
    DCD_DevConnect (pdev);
  }
  else
  {
    /* Wait for the period of time specified in Detach request */
    USB_OTG_BSP_mDelay (req->wValue);
  }
}

/**
  * @brief  DFU_Req_DNLOAD
  *         Handles the DFU DNLOAD request.
  * @param  pdev: device instance
  * @param  req: pointer to the request structure
  * @retval None
  */
static void DFU_Req_DNLOAD(void *pdev, USB_SETUP_REQ *req)
{
  /* Data setup request */
  if (req->wLength > 0)
  {
    if ((DeviceState == STATE_dfuIDLE) || (DeviceState == STATE_dfuDNLOAD_IDLE))
    {
      /* Update the global length and block number */
      wBlockNum = req->wValue;
      wlength = req->wLength;

      /* Update the state machine */
      DeviceState = STATE_dfuDNLOAD_SYNC;
      DeviceStatus[4] = DeviceState;

      /* Prepare the reception of the buffer over EP0 */
      USBD_CtlPrepareRx (pdev,
                         (uint8_t*)MAL_Buffer,
                         wlength);
    }
    /* Unsupported state */
    else
    {
      /* Call the error management function (command will be nacked */
      USBD_CtlError (pdev, req);
    }
  }
  /* 0 Data DNLOAD request */
  else
  {
    /* End of DNLOAD operation*/
    if (DeviceState == STATE_dfuDNLOAD_IDLE || DeviceState == STATE_dfuIDLE )
    {
      Manifest_State = Manifest_In_Progress;
      DeviceState = STATE_dfuMANIFEST_SYNC;
      DeviceStatus[4] = DeviceState;
    }
    else
    {
      /* Call the error management function (command will be nacked */
      USBD_CtlError (pdev, req);
    }
  }
}

/**
  * @brief  DFU_Req_UPLOAD
  *         Handles the DFU UPLOAD request.
  * @param  pdev: instance
  * @param  req: pointer to the request structure
  * @retval status
  */
static void DFU_Req_UPLOAD(void *pdev, USB_SETUP_REQ *req)
{
  const uint8_t *Phy_Addr = NULL;
  uint32_t Addr = 0;

  /* Data setup request */
  if (req->wLength > 0)
  {
    if ((DeviceState == STATE_dfuIDLE) || (DeviceState == STATE_dfuUPLOAD_IDLE))
    {
      /* Update the global langth and block number */
      wBlockNum = req->wValue;
      wlength = req->wLength;

      /* DFU Get Command */
      if (wBlockNum == 0)
      {
        /* Update the state machine */
        DeviceState = (wlength > 3)? STATE_dfuIDLE:STATE_dfuUPLOAD_IDLE;
        DeviceStatus[4] = DeviceState;

        /* Store the values of all supported commands */
        MAL_Buffer[0] = CMD_GETCOMMANDS;
        MAL_Buffer[1] = CMD_SETADDRESSPOINTER;
        MAL_Buffer[2] = CMD_ERASE;

        /* Send the status data over EP0 */
        USBD_CtlSendData (pdev,
                          (uint8_t *)(&(MAL_Buffer[0])),
                          3);
      }
      else if (wBlockNum > 1)
      {
        DeviceState = STATE_dfuUPLOAD_IDLE ;
        DeviceStatus[4] = DeviceState;
        Addr = ((wBlockNum - 2) * XFERSIZE) + Pointer;  /* Change is Accelerated*/

        /* Return the physical address where data are stored */
        Phy_Addr = MAL_Read(usbd_dfu_AltSet, Addr, wlength);

        /* Send the status data over EP0 */
        USBD_CtlSendData (pdev,
                          Phy_Addr,
                          wlength);
      }
      else  /* unsupported wBlockNum */
      {
        DeviceState = STATUS_ERRSTALLEDPKT;
        DeviceStatus[4] = DeviceState;

        /* Call the error management function (command will be nacked */
        USBD_CtlError (pdev, req);
      }
    }
    /* Unsupported state */
    else
    {
      wlength = 0;
      wBlockNum = 0;
      /* Call the error management function (command will be nacked */
      USBD_CtlError (pdev, req);
    }
  }
  /* No Data setup request */
  else
  {
    DeviceState = STATE_dfuIDLE;
    DeviceStatus[4] = DeviceState;
  }
}

/**
  * @brief  DFU_Req_GETSTATUS
  *         Handles the DFU GETSTATUS request.
  * @param  pdev: instance
  * @retval status
  */
static void DFU_Req_GETSTATUS(void *pdev)
{
  switch (DeviceState)
  {
  case   STATE_dfuDNLOAD_SYNC:
    if (wlength != 0)
    {
      DeviceState = STATE_dfuDNBUSY;
      DeviceStatus[4] = DeviceState;
      if ((wBlockNum == 0) && (MAL_Buffer[0] == CMD_ERASE))
      {
        MAL_GetStatus(usbd_dfu_AltSet, Pointer, 0, DeviceStatus);
      }
      else
      {
        MAL_GetStatus(usbd_dfu_AltSet, Pointer, 1, DeviceStatus);
      }
    }
    else  /* (wlength==0)*/
    {
      DeviceState = STATE_dfuDNLOAD_IDLE;
      DeviceStatus[4] = DeviceState;
    }
    break;

  case   STATE_dfuMANIFEST_SYNC :
    if (Manifest_State == Manifest_In_Progress)
    {
        // continue to disconnect USB and eventually reset asynchornously to ensure
        // the response to this message is returned correctly.
        DeviceState = STATE_dfuMANIFEST;
    // get a nice output from dfu-util - original code entered STATE_dfuMANIFEST
    // but this leaves a message Transitioning to dfuMANIFEST state as the last line, which just lingers with no clear indication of what to do next.

      DeviceStatus[4] = STATE_dfuDNLOAD_IDLE;
      //break;
    }
    else if ((Manifest_State == Manifest_complete) && \
      ((usbd_dfu_CfgDesc[(11 + (9 * USBD_DFU_INT_NUM))]) & 0x04))
    {
      DeviceState = STATE_dfuIDLE;
      DeviceStatus[4] = DeviceState;
      //break;
    }
    break;
      case STATE_dfuMANIFEST:
      {
        DeviceState = STATE_dfuIDLE;
        DeviceStatus[4] = DeviceState;
      }

  default :
    break;
  }

  /* Send the status data over EP0 */
  USBD_CtlSendData (pdev,
                    (uint8_t *)(&(DeviceStatus[0])),
                    6);
}

/**
  * @brief  DFU_Req_CLRSTATUS
  *         Handles the DFU CLRSTATUS request.
  * @param  pdev: device instance
  * @retval status
  */
static void DFU_Req_CLRSTATUS(void *pdev)
{
  if (DeviceState == STATE_dfuERROR)
  {
    DeviceState = STATE_dfuIDLE;
    DeviceStatus[0] = STATUS_OK;/*bStatus*/
    DeviceStatus[4] = DeviceState;/*bState*/
  }
  else
  {   /*State Error*/
    DeviceState = STATE_dfuERROR;
    DeviceStatus[0] = STATUS_ERRUNKNOWN;/*bStatus*/
    DeviceStatus[4] = DeviceState;/*bState*/
  }
}

/**
  * @brief  DFU_Req_GETSTATE
  *         Handles the DFU GETSTATE request.
  * @param  pdev: device instance
  * @retval None
  */
static void DFU_Req_GETSTATE(void *pdev)
{
  /* Return the current state of the DFU interface */
  USBD_CtlSendData (pdev,
                    &DeviceState,
                    1);
}

int DFU_Reset_Count = 0;

void DFU_Check_Reset()
{
    if (DFU_Reset_Count && !--DFU_Reset_Count) {

        /* DeInitilialize the MAL(Media Access Layer) */
        MAL_DeInit();

        /* Set system flags and generate system reset to allow jumping to the user code */
        Finish_Update();
    }
}

/**
  * @brief  DFU_Req_ABORT
  *         Handles the DFU ABORT request.
  * @param  pdev: device instance
  * @retval None
  */
static void DFU_Req_ABORT(void *pdev)
{
  if (DeviceState == STATE_dfuIDLE || DeviceState == STATE_dfuDNLOAD_SYNC
      || DeviceState == STATE_dfuDNLOAD_IDLE || DeviceState == STATE_dfuMANIFEST_SYNC
        || DeviceState == STATE_dfuUPLOAD_IDLE )
  {
    DeviceState = STATE_dfuIDLE;
    DeviceStatus[0] = STATUS_OK;
    DeviceStatus[4] = DeviceState;
    wBlockNum = 0;
    wlength = 0;
  }
}

/**
  * @brief  DFU_LeaveDFUMode
  *         Handles the sub-protocol DFU leave DFU mode request (leaves DFU mode
  *         and resets device to jump to user loaded code).
  * @param  pdev: device instance
  * @retval None
  */
void DFU_LeaveDFUMode(void *pdev)
{
 Manifest_State = Manifest_complete;

  if ((usbd_dfu_CfgDesc[(11 + (9 * USBD_DFU_INT_NUM))]) & 0x04)
  {
    DeviceState = STATE_dfuMANIFEST_SYNC;
    DeviceStatus[4] = DeviceState;
    return;
  }
  else
  {
    DeviceState = STATE_dfuMANIFEST_WAIT_RESET;
    DeviceStatus[4] = DeviceState;

    DFU_Reset_Count = 500;

    return;
  }
}


/**
  * @brief  USBD_DFU_GetCfgDesc
  *         Returns configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_DFU_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (usbd_dfu_CfgDesc);
  return usbd_dfu_CfgDesc;
}

#ifdef USB_OTG_HS_CORE
/**
  * @brief  USBD_DFU_GetOtherCfgDesc
  *         Returns other speed configuration descriptor.
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_DFU_GetOtherCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (usbd_dfu_OtherCfgDesc);
  return usbd_dfu_OtherCfgDesc;
}
#endif

/**
  * @brief  USBD_DFU_GetUsrStringDesc
  *         Manages the transfer of memory interfaces string descriptors.
  * @param  speed : current device speed
  * @param  index: desciptor index
  * @param  length : pointer data length
  * @retval pointer to the descriptor table or NULL if the descriptor is not supported.
  */
static uint8_t* USBD_DFU_GetUsrStringDesc (uint8_t speed, uint8_t index , uint16_t *length)
{
  /* Check if the requested string interface is supported */
  if (index <= (USBD_IDX_INTERFACE_STR + USBD_ITF_MAX_NUM))
  {


    USBD_GetString ((uint8_t *)usbd_dfu_StringDesc[index - USBD_IDX_INTERFACE_STR - 1], USBD_StrDesc, length);
    return USBD_StrDesc;
  }
  /* Not supported Interface Descriptor index */
  else
  {
    return NULL;
  }
}
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
