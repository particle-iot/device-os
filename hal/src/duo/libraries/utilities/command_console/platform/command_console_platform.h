/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#ifdef BCM43909
#define PLATFORM_COMMANDS_BCM43909_SPECIFIC \
    { "hibernation",         hibernation_console_command,         1, NULL, NULL, "<sleep_ms>", "Force chip to hibernate for specified amount of milliseconds"}, \
    { "mcu_powersave_clock", mcu_powersave_clock_console_command, 2, NULL, NULL, "<0|1> <0|1|2|3|4>", "<Clock request or release: 0 - release, 1 - request> <Which clock: 0 - ALP available, 1 - HT available, 2 - have at least ILP on backplane, 3 - at least ALP, 4 - at least HT>"}, \
    { "mcu_powersave_tick",  mcu_powersave_tick_console_command,  1, NULL, NULL, "<0|1|2>", "<RTOS tick mode: 0 - always tickless, 1 - never tickless, 2 - tickless if MCU power-save enabled>"}, \
    { "mcu_powersave_mode",  mcu_powersave_mode_console_command,  1, NULL, NULL, "<0|1>", "<MCU powersave mode: 0 - deep-sleep, 1 - normal sleep>"}, \
    { "mcu_powersave_freq",  mcu_powersave_freq_console_command,  1, NULL, NULL, "<freq_mode>", "<CPU/backplane frequency mode>"}, \
    { "mcu_powersave_sleep", mcu_powersave_sleep_console_command, 2, NULL, NULL, "<0|1> <sleep_ms>", "0 - RTOS sleeping with CPU can wake-up earlier if requested by other threads though current one remain in sleep state, 1 - forced sleeping where platform forced to ignore all interrupts except timer and sleep specified amount of time"}, \
    { "mcu_powersave_info",  mcu_powersave_info_console_command,  0, NULL, NULL, NULL, "Print powersave information"}, \
    { "mcu_wlan_powersave_stats",          mcu_wlan_powersave_stats_console_command,  0, NULL, NULL, NULL, "Print WLAN powersave statistics"}, \
    { "mcu_powersave_gpio_wakeup_enable",  mcu_powersave_gpio_wakeup_enable_console_command,  2, NULL, NULL, "<input_pin_pull_mode> <trigger>", "Enable wakening up from deep-sleep via GPIO"}, \
    { "mcu_powersave_gpio_wakeup_disable", mcu_powersave_gpio_wakeup_disable_console_command, 0, NULL, NULL, NULL, "Disable wakening up from deep-sleep via GPIO"}, \
    { "mcu_powersave_gpio_wakeup_ack",     mcu_powersave_gpio_wakeup_ack_console_command,     0, NULL, NULL, NULL, "If GPIO generated wake up event it remain triggered till acked"}, \
    { "mcu_powersave_gci_gpio_wakeup_enable",  mcu_powersave_gci_gpio_wakeup_enable_console_command,  3, NULL, NULL, "<pin> <input_pin_pull_mode> <trigger>", "Enable wakening up from deep-sleep via GPIO"}, \
    { "mcu_powersave_gci_gpio_wakeup_disable", mcu_powersave_gci_gpio_wakeup_disable_console_command, 1, NULL, NULL, "<pin>", "Disable wakening up from deep-sleep via GPIO"}, \
    { "mcu_powersave_gci_gpio_wakeup_ack",     mcu_powersave_gci_gpio_wakeup_ack_console_command,     1, NULL, NULL, "<pin>", "If GPIO generated wake up event it remain triggered till acked"},
#else
#define PLATFORM_COMMANDS_BCM43909_SPECIFIC
#endif

#define PLATFORM_COMMANDS \
    { "reboot",             reboot_console_command,        0, NULL, NULL, NULL,    "Reboot the device"}, \
    { "get_time",           get_time_console_command,      0, NULL, NULL, NULL,    "Print current time"}, \
    { "sleep",              sleep_console_command,         1, NULL, NULL, NULL,    "Sleep number of milliseconds"}, \
    { "prng",               prng_console_command,          1, NULL, NULL, "<num>", "Bytes number"}, \
    { "mcu_powersave",      mcu_powersave_console_command, 1, NULL, NULL, "<0|1>", "Enable/disable MCU powersave"}, \
    { "wiced_init",         wiced_init_console_command,    1, NULL, NULL, "<0|1>", "Call wiced_deinit/wiced_init"}, \
    PLATFORM_COMMANDS_BCM43909_SPECIFIC

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

int reboot_console_command( int argc, char* argv[] );
int get_time_console_command( int argc, char* argv[] );
int sleep_console_command( int argc, char* argv[] );
int prng_console_command( int argc, char* argv[] );
int mcu_powersave_console_command( int argc, char *argv[] );
int wiced_init_console_command( int argc, char *argv[] );

int hibernation_console_command( int argc, char *argv[] );
int mcu_powersave_clock_console_command( int argc, char *argv[] );
int mcu_powersave_tick_console_command( int argc, char *argv[] );
int mcu_powersave_mode_console_command( int argc, char *argv[] );
int mcu_powersave_freq_console_command( int argc, char *argv[] );
int mcu_powersave_sleep_console_command( int argc, char *argv[] );
int mcu_powersave_info_console_command( int argc, char *argv[] );
int mcu_powersave_gpio_wakeup_enable_console_command( int argc, char *argv[] );
int mcu_powersave_gpio_wakeup_disable_console_command( int argc, char *argv[] );
int mcu_powersave_gpio_wakeup_ack_console_command( int argc, char *argv[] );
int mcu_powersave_gci_gpio_wakeup_enable_console_command( int argc, char *argv[] );
int mcu_powersave_gci_gpio_wakeup_disable_console_command( int argc, char *argv[] );
int mcu_powersave_gci_gpio_wakeup_ack_console_command( int argc, char *argv[] );
int mcu_wlan_powersave_stats_console_command( int argc, char *argv[] );

#ifdef __cplusplus
} /*extern "C" */
#endif
