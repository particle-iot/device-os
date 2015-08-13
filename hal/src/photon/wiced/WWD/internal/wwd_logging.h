/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this
 * software may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as
 * incorporated in your product or device that incorporates Broadcom wireless connectivity
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef INCLUDED_WWD_LOGGING_H_
#define INCLUDED_WWD_LOGGING_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
#define WWD_LOGGING_UART_ENABLE
*/
/*
#define WWD_LOGGING_BUFFER_ENABLE
*/

#if defined( WWD_LOGGING_UART_ENABLE )

#include <stdio.h>

#define WWD_LOG( x ) {printf x; }

#elif defined( WWD_LOGGING_BUFFER_ENABLE )


extern int wwd_logging_printf(const char *format, ...);

#define WWD_LOG( x ) {wwd_logging_printf x; }


#else /* if defined( WWD_LOGGING_BUFFER_ENABLE ) */

#define WWD_LOG(x)

#endif /* if defined( WWD_LOGGING_BUFFER_ENABLE ) */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_LOGGING_H_ */
