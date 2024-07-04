#pragma once

/*************************************************************************************************************
  * @brief    STUSB_GEN1S ALERT_STATUS register Structure definition
  * @Address:   0Bh
  * @Access:  RC
  * @Note:  This register indicates an Alert has occurred.
  *             (When a bit value change occurs on one of the mentioned transition register, it automatically
  *             sets the corresponding alert bit in ALERT_STATUS register. )
  ************************************************************************************************************/
#define ALERT_STATUS_1 0x0B // Interrupt register
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t PHY_STATUS_AL          : 1;
    uint8_t PRT_STATUS_AL          : 1;
    uint8_t _Reserved_2            : 1;
    uint8_t PD_TYPEC_STATUS_AL     : 1;
    uint8_t HW_FAULT_STATUS_AL     : 1;
    uint8_t MONITORING_STATUS_AL   : 1;
    uint8_t CC_DETECTION_STATUS_AL : 1;
    uint8_t HARD_RESET_AL          : 1;
  } b;
} STUSB_GEN1S_ALERT_STATUS_RegTypeDef;

#define ALERT_STATUS_MASK 0x0C // interrupt MASK same mask as status should be used
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t PHY_STATUS_AL_MASK          : 1;
    uint8_t PRT_STATUS_AL_MASK          : 1;
    uint8_t _Reserved_2                 : 1;
    uint8_t PD_TYPEC_STATUS_AL_MASK     : 1;
    uint8_t HW_FAULT_STATUS_AL_MASK     : 1;
    uint8_t MONITORING_STATUS_AL_MASK   : 1;
    uint8_t CC_DETECTION_STATUS_AL_MASK : 1;
    uint8_t HARD_RESET_AL_MASK          : 1;
  } b;
} STUSB_GEN1S_ALERT_STATUS_MASK_RegTypeDef;

/*************************************************************************************************************
  * @brief:   STUSB_GEN1S CC_DETECTION_STATUS_TRANS register Structure definition
  * @Address:   0Dh
  * @Access:  RC
  * @Note:  This register indicates a bit value change has occurred in CC_DETECTION_STATUS register.
  ************************************************************************************************************/
#define PORT_STATUS_TRANS 0x0D
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t ATTACH_STATE_TRANS : 1;
    uint8_t _Reserved_1_7      : 7;
  } b;
} STUSB_GEN1S_CC_DETECTION_STATUS_TRANS_RegTypeDef;

#define STUSBMASK_ATTACH_STATUS_TRANS 0x01 //"0b: Cleared, 1b: Transition occurred on ATTACH_STATUS bit"

/*************************************************************************************************************
  * @brief:   STUSB_GEN1S CC_DETECTION_STATUS register Structure definition
  * @Address:   0Eh
  * @Access:  RO
  * @Note:  This register provides current status of the connection detection and corresponding operation modes.
  ************************************************************************************************************/
#define REG_PORT_STATUS 0x0E
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t CC_ATTACH_STATE       : 1;
    uint8_t CC_VCONN_SUPPLY_STATE : 1;
    uint8_t CC_DATA_ROLE          : 1;
    uint8_t CC_POWER_ROLE         : 1;
    uint8_t START_UP_POWER_MODE   : 1;
    uint8_t CC_ATTACH_MODE        : 3;
  } b;
} STUSB_GEN1S_CC_DETECTION_STATUS_RegTypeDef;

#define STUSBMASK_ATTACHED_STATUS 0x01
#define VALUE_NOT_ATTACHED        0x00
#define VALUE_ATTACHED            0x01

/*************************************************************************************************************
  * @brief:   STUSB_GEN1S MONITORING_STATUS_TRANS register Structure definition
  * @Address:   0Fh
  * @Access:  RC
  * @Note:  This register allows to:
  *              - Alert about any change that occurs in MONITORING_STATUS register.
  *              - Manage specific USB PD Acknowledge commands
  *              - to manage Type-C state machine Acknowledge to USB PD Requests commands
  ************************************************************************************************************/
#define TYPEC_MONITORING_STATUS_0 0x0F
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t VCONN_VALID_TRANS    : 1;
    uint8_t VBUS_VALID_SNK_TRANS : 1;
    uint8_t VBUS_VSAFE0V_TRANS   : 1;
    uint8_t VBUS_READY_TRANS     : 1;
    uint8_t VBUS_LOW_STATUS      : 1;
    uint8_t VBUS_HIGH_STATUS     : 1;
    uint8_t Reserved6_7          : 2;
  } b;
} STUSB_GEN1S_MONITORING_STATUS_TRANS_RegTypeDef;

/*************************************************************************************************************
  * @brief:   STUSB_GEN1S MONITORING_STATUS register Structure definition
  * @Address:   10h
  * @Access:  RO
  * @Note:  This register provides information on current status of the VBUS and VCONN voltages
  *   monitoring done respectively on VBUS_SENSE input pin and VCONN input pin.
  ************************************************************************************************************/
#define TYPEC_MONITORING_STATUS_1 0x10
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t VCONN_VALID    : 1;
    uint8_t VBUS_VALID_SNK : 1;
    uint8_t VBUS_VSAFE0V   : 1;
    uint8_t VBUS_READY     : 1;
    uint8_t _Reserved_4_7  : 4;
  } b;
} STUSB_GEN1S_MONITORING_STATUS_RegTypeDef;

#define CC_STATUS 0x11
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t CC1_STATE              : 2;
    uint8_t CC2_STATE              : 2;
    uint8_t CONNECT_RESULT         : 1;
    uint8_t LOOKING_FOR_CONNECTION : 1;
    uint8_t _Reserved_4_7          : 2;
  } b;
} STUSB_GEN1S_CC_STATUS_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB_GEN1S HW_FAULT_STATUS_TRANS register Structure definition
  * @Address:   12h
  * @Access:  RC
  * @Note:  This register indicates a bit value change has occurred in HW_FAULT_STATUS register.
  *   It alerts also when the over temperature condition is met.
  ************************************************************************************************************/
#define CC_HW_FAULT_STATUS_0 0x12
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t VCONN_SW_OVP_FAULT_TRANS    : 1;
    uint8_t VCONN_SW_OCP_FAULT_TRANS    : 1;
    uint8_t VCONN_SW_RVP_FAULT_TRANS    : 1;
    uint8_t VBUS_VSRC_DISCH_FAULT_TRANS : 1;
    uint8_t VPU_VALID_TRANS             : 1;
    uint8_t VPU_OVP_FAULT_TRANS         : 1;
    uint8_t _Reserved_6                 : 1;
    uint8_t THERMAL_FAULT               : 1;
  } b;
} STUSB_GEN1S_HW_FAULT_STATUS_TRANS_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB_GEN1S HW_FAULT_STATUS register Structure definition
  * @Address:   13h
  * @Access:  RO
  * @Note:  This register provides information on hardware fault conditions related to the
  *   internal pull-up voltage in Source power role and to the VCONN power switches
  ************************************************************************************************************/
#define CC_HW_FAULT_STATUS_1 0x13
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t VCONN_SW_OVP_FAULT : 1;
    uint8_t VCONN_SW_OCP_FAULT : 1;
    uint8_t VCONN_SW_RVP_FAULT : 1;
    uint8_t VSRC_DISCH_FAULT   : 1;
    uint8_t Reserved           : 1;
    uint8_t VBUS_DISCH_FAULT   : 1;
    uint8_t VPU_PRESENCE       : 1;
    uint8_t VPU_OVP_FAULT      : 1;
  } b;
} STUSB_GEN1S_HW_FAULT_STATUS_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB_GEN1S PD_TYPEC_STATUS register Structure definition
  * @Address:   14h
  * @Access:  RO
  * @Note:  This register provides handcheck from typeC
  ************************************************************************************************************/
#define PD_TYPEC_STATUS 0x14
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t PD_TYPEC_HAND_CHECK : 4;
    uint8_t Reserved_4_7        : 4;
  } b;
} STUSB_GEN1S_PD_TYPEC_STATUS_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB_GEN1S TYPE_C_STATUS register Structure definition
  * @Address:   15h
  * @Access:  RO
  * @Note:  This register provides information on typeC connection status
  ************************************************************************************************************/
#define REG_TYPE_C_STATUS 0x15
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t TYPEC_FSM_STATE : 5;
    uint8_t PD_SNK_TX_RP    : 1;
    uint8_t PD_SRC_TX_RP    : 1;
    uint8_t REVERSE         : 1;
  } b;
} STUSB_GEN1S_TYPE_C_STATUS_RegTypeDef;

#define MASK_REVERSE 0x80 //0: CC1 is attached 1: CC2 is Attach.

/************************************************************************************************************
  * @brief:   STUSB_GEN1S PRT_STATUS register Structure definition
  * @Address:   16h
  * @Access:  RO
  * @Note:  This register provides information on PRT status
  *
  ************************************************************************************************************/
#define PRT_STATUS 0x16
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t HWRESET_RECEIVED : 1;
    uint8_t HWRESET_DONE     : 1;
    uint8_t MSG_RECEIVED     : 1;
    uint8_t MSG_SENT         : 1;
    uint8_t BIST_RECEIVED    : 1;
    uint8_t BIST_SENT        : 1;
    uint8_t Reserved_6       : 1;
    uint8_t TX_ERROR         : 1;
  } b;
} STUSB_GEN1S_PRT_STATUS_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB_GEN1S PHY_STATUS register Structure definition
  * @Address:   17h
  * @Access:  RO
  * @Note:  This register provides information on PHY status
  *
  ************************************************************************************************************/
#define PHY_STATUS 0x17
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t TX_MSG_FAIL : 1;
    uint8_t TX_MSG_DISC : 1;
    uint8_t TX_MSG_SUCC : 1;
    uint8_t IDLE        : 1;
    uint8_t Reserved2   : 1;
    uint8_t SOP_RX_Type : 3;

  } b;
} STUSB_GEN1S_PHY_STATUS_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB_GEN1S CC_CAPABILITY_CTRL register Structure definition
  * @Address:   18h
  * @Access:  R/W
  * @Note:  This register allows to change the advertising of the current capability and the VCONN
  *   supply capability when operating in Source power role.
  ************************************************************************************************************/
#define CC_CAPABILITY_CTRL 0x18
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t CC_VCONN_SUPPLY_EN    : 1;
    uint8_t VCONN_SWAP_EN         : 1;
    uint8_t PR_SWAP_EN            : 1;
    uint8_t DR_SWAP_EN            : 1;
    uint8_t CC_VCONN_DISCHARGE_EN : 1;
    uint8_t SNK_DISCONNECT_MODE   : 1;
    uint8_t CC_CURRENT_ADVERTISED : 2;
  } b;
} STUSB_GEN1S_CC_CAPABILITY_CTRL_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB_GEN1S PRT_TX_CTRL register Structure definition
  * @Address:   19h
  * @Access:  R/W
  * @Note:  This register allows PRL TX layer
  ************************************************************************************************************/
#define STUSB_GEN1S_PRT_TX_CTRL 0x19
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t PRT_TX_SOP_MSG    : 3;
    uint8_t reserved_3        : 1;
    uint8_t PRT_RETRY_MSG_CNT : 2;
    uint8_t reserved_6_7      : 2;
  } b;
} STUSB_GEN1S_PRT_TX_CTRL_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB_GEN1S PD_COMMAND_CTRL register Structure definition
  * @Address:   1Ah
  * @Access:  R/W
  * @Note:  This register allows to send command to PRL or PE internal state machines
  ************************************************************************************************************/
#define STUSB_GEN1S_CMD_CTRL 0x1A
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t PD_CMD : 8;
  } b;
} STUSB_GEN1S_PD_CMD_CTRL_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB_GEN1S_DEVICE_CTRL register Structure definition
  * @Address:   1Dh
  * @Access:  R/W
  * @Note:  This register allows to Reset PHYTX & change device Automation level
  ************************************************************************************************************/
#define STUSB_GEN1S_DEV_CTRL 0x1D
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t reserved_0   : 1;
    uint8_t PHY_TX_RESET : 1;
    uint8_t reserved_2_5 : 4;
    uint8_t PD_TOP_LAYER : 2;
  } b;
} STUSB_GEN1S_DEVICE_CTRL_RegTypeDef;

#define SNK_UNATTACHED        0x00
#define SNK_ATTACHWAIT        0x01
#define SNK_ATTACHED          0x02
#define SNK_2_SRC_PR_SWAP     0x06
#define SNK_TRYWAIT           0x07
#define SRC_UNATTACHED        0x08
#define SRC_ATTACHWAIT        0x09
#define SRC_ATTACHED          0x0A
#define SRC_2_SNK_PR_SWAP     0x0B
#define SRC_TRY               0x0C
#define ACCESSORY_UNATTACHED  0x0D
#define ACCESSORY_ATTACHWAIT  0x0E
#define ACCESSORY_AUDIO       0x0F
#define ACCESSORY_DEBUG       0x10
#define ACCESSORY_POWERED     0x11
#define ACCESSORY_UNSUPPORTED 0x12
#define ERRORRECOVERY         0x13

#define STUSB_GEN1S_MONITORING_CTRL_0 0x20
#define STUSB_GEN1S_MONITORING_CTRL_1 0x21
#define STUSB_GEN1S_MONITORING_CTRL_2 0x22

typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t VSEL_PDO  : 8;
  } b;
} STUSB_GEN1S_MONITORING_CTRL_1_RegTypeDef;

/************************************************************************************************************
  * @brief:   STUSB RESET_CTRL register Structure definition
  * @Address:   23h
  * @Access:  R/W
  * @Note:  This register allows to reset the device by software.
  *   The SW_RESET_EN bit acts as the hardware RESET pin but it does not command the RESET pin.
  ************************************************************************************************************/
#define STUSB_GEN1S_RESET_CTRL_REG 0x23
typedef union
{
  uint8_t d8;
  struct
  {
    uint8_t SW_RESET_EN   : 1;
    uint8_t _Reserved_1_7 : 7;
  } b;
} STUSB_GEN1S_RESET_CTRL_RegTypeDef;

typedef enum
{
  No_SW_RST = 0, /*!< DEFAULT: Device reset is performed through the hardware RESET pin */
  SW_RST    = 1  /*!< Force the device reset as long as this bit value is set */
} SW_RESET_TypeDef;

#define PE_FSM 0x29
/* PE STATES */
#define PE_SOFT_RESET                        0x01          //     "00000001"
#define PE_HARD_RESET                        0x02           //     "00000010"
#define PE_SEND_SOFT_RESET                   0x03           //     "00000011"
#define PE_BIST_CARRIER_MODE                 0x04           //     "00000100"

#define PE_SNK_STARTUP                       0x12           //     "00010010"
#define PE_SNK_DISCOVERY                     0x13           //     "00010011"
#define PE_SNK_WAIT_FOR_CAPABILITIES         0x14           //     "00010100"
#define PE_SNK_EVALUATE_CAPABILITIES         0x15           //     "00010101"
#define PE_SNK_SELECT_CAPABILITIES           0x16           //     "00010110"
#define PE_SNK_TRANSITION_SINK               0x17           //     "00010111"
#define PE_SNK_READY                         0x18           //     "00011000"
#define PE_SNK_READY_SENDING                 0x19           //     "00011001"
#define PE_DB_CP_CHECK_FOR_VBUS              0x1A           //     "00011010"
// 0x19 -- 0x29 reserved
#define PE_ERRORRECOVERY                     0x30           //     "00110000"
#define PE_SRC_TRANSITION_SUPPLY_3           0x31           //     "00110001"
#define PE_SRC_TRANSITION_SUPPLY_2B          0x31           //     "00110001"
#define PE_SRC_GET_SINK_CAP                  0x31           //

#define RX_BYTE_CNT  0x30
#define RX_HEADER    0x31 // RX message header (16bit)
#define RX_DATA_OBJ  0x33
#define RX_DATA_OBJ1 0x33 // (32bit)
#define RX_DATA_OBJ2 0x37 // (32bit)
#define RX_DATA_OBJ3 0x3B // (32bit)
#define RX_DATA_OBJ4 0x3F // (32bit)
#define RX_DATA_OBJ5 0x33 // (32bit)
#define RX_DATA_OBJ6 0x37 // (32bit)
#define RX_DATA_OBJ7 0x4B // (32bit)

#define TX_BYTE_CNT    0x50
#define TX_HEADER      0x51 // (16bit)
#define TX_HEADER_LOW  0x51
#define TX_HEADER_HIGH 0x52
#define TX_DATA_OBJ1   0x53
#define TX_DATA_OBJ2   0x57
#define TX_DATA_OBJ3   0x5B
#define TX_DATA_OBJ4   0x5F
#define TX_DATA_OBJ5   0x63
#define TX_DATA_OBJ6   0x67
#define TX_DATA_OBJ7   0x6B

#define DPM_PDO_NUMB 0x70

#define DPM_SNK_PDO1 0x85
typedef union
{
  uint32_t d32;
  struct
  {
    uint32_t Operational_Current        : 10;
    uint32_t Voltage                    : 10;
    uint8_t  Reserved_22_20             :  3;
    uint8_t  Fast_Role_Req_cur          :  2;  /* must be set to 0 in 2.0 */
    uint8_t  Dual_Role_Data             :  1;
    uint8_t  USB_Communications_Capable :  1;
    uint8_t  Unconstrained_Power        :  1;
    uint8_t  Higher_Capability          :  1;
    uint8_t  Dual_Role_Power            :  1;
    uint8_t  Fixed_Supply               :  2;
  } fix;
  struct
  {
    uint32_t Operating_Current : 10;
    uint32_t Min_Voltage       : 10;
    uint32_t Max_Voltage       : 10;
    uint8_t  VariableSupply    :  2;
  } var;
  struct
  {
    uint32_t Operating_Power : 10;
    uint32_t Min_Voltage     : 10;
    uint32_t Max_Voltage     : 10;
    uint8_t  Battery         :  2;
  } bat;

} USB_PD_SNK_PDO_TypeDef;

#define PDO_SNK_FIX_FIXED    0
#define PDO_SNK_FIX_VARIABLE 1
#define PDO_SNK_FIX_BATTERY  2

typedef union
{
  uint32_t d32;
  struct
  {
    //Table 6-9 Fixed Supply PDO - Source
    uint32_t Max_Operating_Current : 10; // bits 9..0
    uint32_t Voltage               : 10; // bits 19..10
    uint8_t  PeakCurrent           :  2; // bits 21..20
    uint8_t  Reserved              :  3; //
    uint8_t  DataRoleSwap          :  1; // bits 25
    uint8_t  Communication         :  1; // bits 26
    uint8_t  ExternalyPowered      :  1; // bits 27
    uint8_t  SuspendSuported       :  1; // bits 28
    uint8_t  DualRolePower         :  1; // bits 29
    uint8_t  FixedSupply           :  2; // bits 31..30
  } fix;
  struct
  {
    //Table 6-11 Variable Supply (non-Battery) PDO - Source
    uint32_t Operating_Current : 10;
    uint32_t Min_Voltage       : 10;
    uint32_t Max_Voltage       : 10;
    uint8_t  VariableSupply    :  2;
  } var;
  struct
  {
    //Table 6-12 Battery Supply PDO - Source
    uint32_t Operating_Power : 10;
    uint32_t Min_Voltage     : 10;
    uint32_t Max_Voltage     : 10;
    uint8_t  Battery         :  2;
  } bat;

} USB_PD_SRC_PDO_TypeDef;

#define DPM_REQ_RDO    0x91 // (32bits)
#define RDO_REG_STATUS 0x91
typedef union
{
  uint32_t d32;
  struct
  {
    uint32_t MaxCurrent        : 10; // Bits 9..0
    uint32_t OperatingCurrent  : 10;
    uint8_t  reserved_22_20    :  3;
    uint8_t  UnchunkedMess_sup :  1;
    uint8_t  UsbSuspend        :  1;
    uint8_t  UsbComCap         :  1;
    uint8_t  CapaMismatch      :  1;
    uint8_t  GiveBack          :  1;
    uint8_t  Object_Pos        :  3; // Bits 30..28 (3-bit)
    uint8_t  reserved_31       :  1; // Bits 31

  } b;
} STUSB_GEN1S_RDO_REG_STATUS_RegTypeDef;

//Table 6-5 Control Message Types
#define USBPD_CTRLMSG_Reserved1                 0x00
#define USBPD_CTRLMSG_GoodCRC                   0x01
#define USBPD_CTRLMSG_GotoMin                   0x02
#define USBPD_CTRLMSG_Accept                    0x03
#define USBPD_CTRLMSG_Reject                    0x04
#define USBPD_CTRLMSG_Ping                      0x05
#define USBPD_CTRLMSG_PS_RDY                    0x06
#define USBPD_CTRLMSG_Get_Source_Cap            0x07
#define USBPD_CTRLMSG_Get_Sink_Cap              0x08
#define USBPD_CTRLMSG_DR_Swap                   0x09
#define USBPD_CTRLMSG_PR_Swap                   0x0A
#define USBPD_CTRLMSG_VCONN_Swap                0x0B
#define USBPD_CTRLMSG_Wait                      0x0C
#define USBPD_CTRLMSG_Soft_Reset                0x0D
#define USBPD_CTRLMSG_Reserved2                 0x0E
#define USBPD_CTRLMSG_Reserved3                 0x0F
#define USBPD_CTRLMSG_Not_Supported             0x10
#define USBPD_CTRLMSG_Get_Source_Cap_Extended   0x11
#define USBPD_CTRLMSG_Get_Status                0x12
#define USBPD_CTRLMSG_FR_Swap                   0x13
#define USBPD_CTRLMSG_Get_PPS_Status            0x14
#define USBPD_CTRLMSG_Get_Country_Codes         0x15
#define USBPD_CTRLMSG_Reserved4                 0x16
#define USBPD_CTRLMSG_Reserved5                 0x1F

//Table 6-6 Data Message Types
#define USBPD_DATAMSG_Reserved1             0x00
#define USBPD_DATAMSG_Source_Capabilities   0x01
#define USBPD_DATAMSG_Request               0x02
#define USBPD_DATAMSG_BIST                  0x03
#define USBPD_DATAMSG_Sink_Capabilities     0x04
#define USBPD_DATAMSG_Battery_Status        0x05
#define USBPD_DATAMSG_Alert                 0x06
#define USBPD_DATAMSG_Get_Country_Info      0x07
#define USBPD_DATAMSG_Reserved2             0x08
#define USBPD_DATAMSG_Reserved3             0x0E
#define USBPD_DATAMSG_Vendor_Defined        0x0F
#define USBPD_DATAMSG_Reserved4             0x10
#define USBPD_DATAMSG_Reserved5             0x1F

/* Identification of STUSB */
#define REG_DEVICE_ID (uint8_t)0x2F
#define EXTENTION_10  (uint8_t)0x80
#define ID_Reg        (uint8_t)0x1C
#define CUT           (uint8_t)3 << 2
#define CUT_A         (uint8_t)4 << 2
#define GEN1S         (uint8_t)8 << 2
#define DEV_CUT       (uint8_t)0x03

#define DEFAULT                0xFF

#define FTP_CUST_PASSWORD_REG  0x95
#define FTP_CUST_PASSWORD      0x47

#define FTP_CTRL_0             0x96
#define FTP_CUST_PWR           0x80
#define FTP_CUST_RST_N         0x40
#define FTP_CUST_REQ           0x10
#define FTP_CUST_SECT          0x07
#define FTP_CTRL_1             0x97
#define FTP_CUST_SER           0xF8
#define FTP_CUST_OPCODE        0x07
#define RW_BUFFER              0x53
#define TX_HEADER_LOW          0x51
#define PD_COMMAND_CTRL        0x1A
#define DPM_PDO_NUMB           0x70

#define READ                   0x00
#define WRITE_PL               0x01
#define WRITE_SER              0x02
#define ERASE_SECTOR           0x05
#define PROG_SECTOR            0x06
#define SOFT_PROG_SECTOR       0x07

#define SECTOR_0               0x01
#define SECTOR_1               0x02
#define SECTOR_2               0x04
#define SECTOR_3               0x08
#define SECTOR_4               0x10