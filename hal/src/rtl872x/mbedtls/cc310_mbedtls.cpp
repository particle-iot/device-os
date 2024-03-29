/*
 *  Copyright (c) 2018, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#define DISABLE_CC310

#ifndef DISABLE_CC310

#include "cc310_mbedtls.h"

#include "crys_rnd.h"
#include "sns_silib.h"

#include "logging.h"

CRYS_RND_State_t     m_rndState;
CRYS_RND_WorkBuff_t  m_rndWorkBuff;

CRYS_RND_State_t    * pRndState    = &m_rndState;
CRYS_RND_WorkBuff_t * pRndWorkBuff = &m_rndWorkBuff;

void cc310_init(void)
{
    SA_SilibRetCode_t sa_result;
    CRYSError_t       crys_result;

    CC310_OPERATION(SaSi_LibInit(), sa_result);
    if (sa_result != SA_SILIB_RET_OK)
    {
        LOG(PANIC, "Failed SaSi_LibInit - ret = 0x%x", sa_result);
    }

    CC310_OPERATION(CRYS_RndInit(pRndState, pRndWorkBuff), crys_result);
    if (crys_result != CRYS_OK)
    {
        LOG(PANIC, "Failed CRYS_RndInit - ret = 0x%x", crys_result);
    }
}

void cc310_deinit(void)
{
    CC310_OPERATION_NO_RESULT(SaSi_LibFini());
    CC310_OPERATION_NO_RESULT(CRYS_RND_UnInstantiation(pRndState));
}

namespace {

/* FIXME: This should probably be done somewhere in core_hal */
struct CC310Initializer {
    CC310Initializer() {
        cc310_init();
    }
};

CC310Initializer s_cc310Initializer;

} /* anonymous */

#endif // DISABLE_CC310
