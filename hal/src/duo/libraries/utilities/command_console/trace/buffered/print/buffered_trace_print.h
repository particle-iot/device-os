/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef BUFFEREDTRACEPRINT_H_
#define BUFFEREDTRACEPRINT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef TRACE_ENABLE_BUFFERED_PRINT
#define TRACE_PROCESS_T_BUFFERED_PRINT              \
    {                                               \
        (char*) "print",                            \
        buffered_trace_print_process_trace,         \
        buffered_trace_print_flush_trace            \
    },
#else
#define TRACE_PROCESS_T_BUFFERED_PRINT
#endif /* TRACE_ENABLE_BUFFERED_PRINT */

/******************************************************
 *      Function definitions
 ******************************************************/
void buffered_trace_print_process_trace( void );
void buffered_trace_print_flush_trace( void );


/******************************************************
 *      Output graph formatting
 ******************************************************/

typedef enum
{
    Output_Style_CSV,
    Output_Style_Table
} trace_output_style_t;

typedef enum
{
    Output_TimeFormat_AbsoluteTicks,
    Output_TimeFormat_RelativeTicks,
    Output_TimeFormat_AbsoluteMilliseconds,
    Output_TimeFormat_RelativeMilliseconds
} trace_output_timeformat_t;

/** Flags */
#define FLAG_FILL_IN_BLANKS     0x00000001  /* Extrapolate data */
#define FLAG_SHOW_TASK_NAMES    0x00000002
#define FLAG_SHOW_LEGEND        0x00000004

#define do_Fill_In_Blanks(format)   ((format.flags & FLAG_FILL_IN_BLANKS )  != 0 )
#define do_Show_Task_Names(format)  ((format.flags & FLAG_SHOW_TASK_NAMES ) != 0 )
#define do_Show_Legend(format)      ((format.flags & FLAG_SHOW_LEGEND )     != 0 )

struct trace_output_format_t
{
    trace_output_style_t style;
    trace_output_timeformat_t time;
    int flags;
};
typedef struct trace_output_format_t trace_output_format_t;

/******************************************************************************/
/**     Table output                                                          */
/******************************************************************************/

/** String to act as the header of the time column */
#define TABLE_TIME_HEADER           "| Time    |"

/** Character to be repeated to separate table data from headers */
#define TABLE_SEPARATOR             '='

/** String to appear at the end of each line of data */
#define TABLE_ENDLINE               " |\r\n"

/** Character to be used to "blank out" a task when there is no action */
#define TABLE_TASK_BLANK            ' '

/** Format string for each line of data - %s refers to the data */
#define TABLE_LINE_FORMAT           "| %7lu |%s |\r\n"
#define TABLE_LINE_FORMAT_CLOCKTIME "| %7lu |%s | %lu us\r\n"

/**
 * Format string for each line of data in which the time has already been
 * printed on a previous line.
 */
#define TABLE_LINE_FORMAT_NOTIME    "|         |%s |\r\n"
#define TABLE_LINE_FORMAT_NOTIME_CLOCKTIME "|         |%s | %lu us\r\n"

/** Print a separator line. */
#define TABLE_PRINT_SEPERATOR( width )          \
    do                                          \
    {                                           \
        unsigned int i;                         \
        for ( i = 0; i < width; i++ )           \
        {                                       \
            printf( "%c", TABLE_SEPARATOR );    \
        }                                       \
        printf( "\r\n" );                       \
    }                                           \
    while ( 0 )

/** Reset (blank) the data output buffer. */
#define TABLE_RESET_LINE( array, width )        \
    do                                          \
    {                                           \
        unsigned int i;                         \
        for ( i = 0; i < width; i++ )           \
        {                                       \
            array[i] = TABLE_TASK_BLANK;        \
        }                                       \
    }                                           \
    while( 0 )

/** Transform each trace action */
#define TABLE_TRANSFORM_LINE( array, width )    \
    do                                          \
    {                                           \
        unsigned int i;                         \
        for ( i = 0; i < width ; i++ )          \
        {                                       \
            trace_action_t action = char_to_traceaction( array[i] ); \
            trace_action_t subsequent_action = subsequent_traceaction( action ); \
            array[i] = traceaction_to_char( subsequent_action ); \
        }                                       \
    }                                           \
    while( 0 )

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* BUFFEREDTRACEPRINT_H_ */
