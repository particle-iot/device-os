/*****************************************************************************
*
*  os.c  - CC3000 Host Driver Implementation.
*  Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/


//*****************************************************************************
//
//! \addtogroup os_api
//! @{
//
//*****************************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "os.h"


/**
 * \brief allocate memory
 *
 * Function allocates size bytes of uninitialized memory, and returns a pointer to the allocated memory.\n
 * The allocated space is suitably aligned (after possible pointer coercion) for storage of any type of object.
 * 
 * \param[in] size    number of memory bytes to allocate
 *
 * \return   On success return a pointer to the allocated space, on failure return null
 * \sa OS_free
 * \note
 * 
 * \warning
 */

void * OS_malloc( unsigned long size )
{
    return malloc( size );
}

/**
 * \brief free allocated memory
 *
 * Function causes the allocated memory referenced by ptr to
 *  be made available for future allocations.\n If ptr is
 * NULL, no action occurs.
 * 
 *  \param[in] ptr   pointer to previously allocated memory
 *
 * \return   None 
 *  
 * \sa OS_malloc
 * \note
 * 
 * \warning
 */

void   OS_free( void * ptr )
{
    free( ptr );
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
