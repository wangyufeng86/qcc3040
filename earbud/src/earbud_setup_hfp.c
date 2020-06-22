/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Earbud specific incoming call handler.

*/

#include "earbud_setup_hfp.h"

#include "earbud_sm.h"
#include "state_proxy.h"
#include "hfp_profile.h"

static bool earbud_HfpAcceptCallCallback(void)
{
    /* check if this earbud is in ear */
    bool local_in_ear = appSmIsInEar();
    /* check if the peer earbud is in ear (peer sync must be completed) */
    bool peer_in_ear = StateProxy_IsPeerInEar();

    /* accept or not with the rule described above */
    return local_in_ear || peer_in_ear;
}

void Earbud_SetupHfp(void)
{
    appHfpRegisterAcceptCallCallback(earbud_HfpAcceptCallCallback);
}
