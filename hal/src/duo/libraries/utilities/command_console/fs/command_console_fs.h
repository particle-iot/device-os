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
#define FS_COMMANDS \
    { (char*) "mount",          mount,                  0, NULL, NULL, (char*) "<device name>"}, \
    { (char*) "unmount",        unmount,                1, NULL, NULL, (char*) "<device id, 0:Ramdisk, 1:Usbdisk>"}, \
    { (char*) "pwd",            getcwd,                 0, NULL, NULL, (char*) ""}, \
    { (char*) "mkdir",          mk_dir,                 1, NULL, NULL, (char*) "<directory name>"}, \
    { (char*) "cd",             change_dir,             1, NULL, NULL, (char*) "<directory name>"}, \
    { (char*) "cat",            cat_file,               1, NULL, NULL, (char*) "<file name>"}, \
    { (char*) "ls",             ls_dir,                 0, NULL, NULL, (char*) ""}, \


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
/* Console commands */
int mount (int argc, char* argv[]);
int unmount (int argc, char* argv[]);
int getcwd (int argc, char* argv[]);
int mk_dir (int argc, char* argv[]);
int change_dir (int argc, char* argv[]);
int cat_file (int argc, char* argv[]);
int ls_dir (int argc, char* argv[]);

/* This function read a given file and create/write to another new
 * file, then do file compare.
 */
int file_rw_sha1sum_test (int argc, char* argv[]);

/* This function read and write to the given file with several different
 * burst lengths to do throughput test.
 */
int file_rw_tput_test (int argc, char* argv[]);

/* This function read all of the files under /read directory,
 * and write to /write directory. Also calculate read/write file sha1sum
 * and compare with /sha1_expect_list.txt for validation.
 */
int files_rw_sha1sum_test (int argc, char* argv[]);

int sw_calc_sha1 (int argc, char* argv[]);


#ifdef __cplusplus
} /*extern "C" */
#endif
