/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#ifndef AM18X5_DEFINES_H
#define AM18X5_DEFINES_H

#include "hal_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC

// Time and Date Bits Mask
#define SECONDS_MASK                0x7F
#define MINUTES_MASK                0x7F
#define HOURS_12_MASK               0x1F
#define HOURS_AM_PM_MASK            0x20
#define HOURS_24_MASK               0x3F
#define DATE_MASK                   0x3F
#define MONTHS_MASK                 0x1F
#define WEEKDAY_MASK                0x07

// Alarm Bits Mask
#define SECONDS_ALARM_MASK          0x7F
#define MINUTES_ALARM_MASK          0x7F
#define HOURS_12_ALARM_MASK         0x1F
#define HOURS_AM_PM_ALARM_MASK      0x20
#define HOURS_24_ALARM_MASK         0x3F
#define DATE_ALARM_MASK             0x3F
#define MONTHS_ALARM_MASK           0x1F
#define WEEKDAY_ALARM_MASK          0x07

// Status Bit Mask
#define STATUS_CB_MASK              0x80
#define STATUS_BAT_MASK             0x40
#define STATUS_WDT_MASK             0x20
#define STATUS_BL_MASK              0x10
#define STATUS_TIM_MASK             0x08
#define STATUS_ALM_MASK             0x04
#define STATUS_EX2_MASK             0x02
#define STATUS_EX1_MASK             0x01

// Control 1 Bits Mask
#define CONTROL1_STOP_MASK          0x80
#define CONTROL1_1224_MASK          0x40
#define CONTROL1_1224_SHIFT         (6)
#define CONTROL1_OUTB_MASK          0x20
#define CONTROL1_OUT_MASK           0x10
#define CONTROL1_RSP_MASK           0x08
#define CONTROL1_ARST_MASK          0x04
#define CONTROL1_ARST_SHIFT         (2)
#define CONTROL1_PWR2_MASK          0x02
#define CONTROL1_WRTC_MASK          0x01

// Control 2 Bits Mask
#define CONTROL2_RS1E_MASK          0x20
#define CONTROL2_OUT2S_MASK         0x1C
#define CONTROL2_OUT2S_SHIFT        (2)
#define CONTROL2_OUT1S_MASK         0x03

// Interrupt Bits Mask
#define INTERRUPT_CEB_MASK          0x80
#define INTERRUPT_IM_MASK           0x60
#define INTERRUPT_BLIE_MASK         0x10
#define INTERRUPT_TIE_MASK          0x08
#define INTERRUPT_TIE_SHIFT         (3)
#define INTERRUPT_AIE_MASK          0x04
#define INTERRUPT_AIE_SHIFT         (2)
#define INTERRUPT_EX2E_MASK         0x02
#define INTERRUPT_EX1E_MASK         0x01

// SQW Bit Mask
#define SQW_SQWE_MASK               0x80
#define SQW_SQFS_MASK               0x1F

// Calibration Bits Mask
#define CAL_XT_CMDX_MASK            0x80
#define CAL_XT_OFFSETX_MASK         0x7F
#define CAL_RC_HI_CMDR_MASK         0xC0
#define CAL_RC_HI_OFFSETRU_MASK     0x3F

// Sleep Control Bit Mask
#define SLEEP_CONTROL_SLP_MASK      0x80
#define SLEEP_CONTROL_SLP_SHIFT     (7)
#define SLEEP_CONTROL_SLRES_MASK    0x40 // When 1, assert nRST low when the Power Control SM is in the SLEEP state.
#define SLEEP_CONTROL_EX2P_MASK     0x20 // When 1, the external interrupt XT2 will trigger on a rising edge of the WDI pin.
#define SLEEP_CONTROL_EX1P_MASK     0x10 // When 1, the external interrupt XT1 will trigger on a rising edge of the EXTI pin.
#define SLEEP_CONTROL_SLST_MASK     0x08 // Set when the AM18X5 enters Sleep Mode
#define SLEEP_CONTROL_SLTO_MASK     0x07 // The number of 7.8 ms periods after SLP is set until the Power Control SM goes into the SLEEP state.

// Countdown Timer Control Bits Mask
#define TIMER_CONTROL_TE_MASK       0x80
#define TIMER_CONTROL_TE_SHIFT      (7)
#define TIMER_CONTROL_TM_MASK       0x40
#define TIMER_CONTROL_TRPT_MASK     0x20
#define TIMER_CONTROL_RPT_MASK      0x1C
#define TIMER_CONTROL_RPT_SHIFT     (2)
#define TIMER_CONTROL_TFS_MASK      0x03

// WDT Bits Mask
#define WDT_REGISTER_WDS_MASK       0x80
#define WDT_REGISTER_BMB_MASK       0x7C
#define WDT_REGISTER_BMB_SHIFT      (2)
#define WDT_REGISTER_WRB_MASK       0x03

//outcontrol sleep mode value
// #define CCTRL_SLEEP_MODE_MASK 0xC0 //清除 WDDS,EXDS,RSEN,O4EN,O3EN,O1EN

// OSC Control Bits Mask
#define OSC_CONTROL_OSEL_MASK       0x80
#define OSC_CONTROL_OSEL_SHIFT      (7)
#define OSC_CONTROL_ACAL_MASK       0x60
#define OSC_CONTROL_AOS_MASK        0x10
#define OSC_CONTROL_AOS_SHIFT       (4)
#define OSC_CONTROL_FOS_MASK        0x08
#define OSC_CONTROL_PWGT_MASK       0x04
#define OSC_CONTROL_OFIE_MASK       0x02
#define OSC_CONTROL_ACIE_MASK       0x01

// OSC Status Bits Mask
#define OSC_STATUS_XTCAL_MASK       0xC0
#define OSC_STATUS_XTCAL_SHIFT      (6)
#define OSC_STATUS_LKO2_MASK        0x20
#define OSC_STATUS_OMODE_MASK       0x10
#define OSC_STATUS_OF_MASK          0x02
#define OSC_STATUS_ACF_MASK         0x01

// Trickle Bits Mask
#define TRICKLE_TCS_MASK            0xF0
#define TRICKLE_DIODE_MASK          0x0C
#define TRICKLE_ROUT_MASK           0x03

// BREF Control Bits Mask
#define BREF_MASK                   0xF0

// Batmode IO Bit Mask
#define BATMODE_IO_MASK             0x80

// Analog Status Bits Mask
#define ANALOG_STATUS_BBOD_MASK     0x80
#define ANALOG_STATUS_BMIN_MASK     0x40
#define ANALOG_STATUS_VINIT_MASK    0x02

// Out Control Bit Mask
#define OUTPUT_CTRL_WDBM_MASK       0x80
#define OUTPUT_CTRL_EXBM_MASK       0x40
#define OUTPUT_CTRL_WDDS_MASK       0x20
#define OUTPUT_CTRL_EXDS_MASK       0x10
#define OUTPUT_CTRL_RSEN_MASK       0x08
#define OUTPUT_CTRL_O4EN_MASK       0x04
#define OUTPUT_CTRL_O3EN_MASK       0x02
#define OUTPUT_CTRL_O1EN_MASK       0x01

#endif // HAL_PLATFORM_EXTERNAL_RTC

#endif // AM18X5_DEFINES_H