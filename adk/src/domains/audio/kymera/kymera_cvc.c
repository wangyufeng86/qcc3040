/*!
\copyright  Copyright (c) 2017-2020  Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       kymera_cvc.c
\brief      Kymera module to manage the CVC capability
*/

#include "kymera_private.h"
#include "kymera_cvc.h"
#include <vmal.h>
#include <panic.h>
#include <chain.h>

void kymera_EnableCvcPassthroughMode(void)
{
    kymeraTaskData *theKymera = KymeraGetTaskData();
    theKymera->enable_cvc_passthrough = 1;

    if( appKymeraGetState() == KYMERA_STATE_SCO_ACTIVE )
    {
        kymera_chain_handle_t sco_chain = appKymeraGetScoChain();
        if(sco_chain != NULL)
        {
            Operator cvc_op;
            const uint16 cvc_snd_msg[] = {OPMSG_COMMON_ID_SET_CONTROL, 1, OPMSG_CONTROL_MODE_ID, 0, 4};
            const uint16 cvc_rcv_msg[] = {OPMSG_COMMON_ID_SET_CONTROL, 1, OPMSG_CONTROL_MODE_ID, 0, 3};

            PanicFalse(GET_OP_FROM_CHAIN(cvc_op, sco_chain, OPR_CVC_SEND));
            PanicFalse(VmalOperatorMessage(cvc_op, cvc_snd_msg, ARRAY_DIM(cvc_snd_msg), NULL, 0));

            PanicFalse(GET_OP_FROM_CHAIN(cvc_op, sco_chain, OPR_CVC_RECEIVE));
            PanicFalse(VmalOperatorMessage(cvc_op, cvc_rcv_msg, ARRAY_DIM(cvc_rcv_msg), NULL, 0));
        }
    }
}
