/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_null_state.c
\brief      Fast Pair Null State Event handling
*/

#include "fast_pair_null_state.h"
#include "fast_pair_advertising.h"

static bool fastpair_StateNullHandleAuthCfm(fast_pair_state_event_auth_args_t* args)
{
    le_adv_data_set_t data_set;
    if(args->auth_cfm->status == auth_status_success)
    {
        DEBUG_LOG("fastpair_StateNullHandleAuthCfm. CL_SM_AUTHENTICATE_CFM status %d", args->auth_cfm->status);
        data_set = le_adv_data_set_handset_unidentifiable;
        fastPair_SetIdentifiable(data_set);
        return TRUE;
    }
    return FALSE;
}

bool fastPair_StateNullHandleEvent(fast_pair_state_event_t event)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

    switch (event.id)
    {

        case fast_pair_state_event_timer_expire:
        case fast_pair_state_event_power_off:
        {
            fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
        }
        break;

        case fast_pair_state_event_auth:
        {
            if(event.args == NULL)
            {
                return FALSE;
            }
            status = fastpair_StateNullHandleAuthCfm((fast_pair_state_event_auth_args_t *) event.args);
        }
        break;

        default:
        {
            DEBUG_LOG("Unhandled event [%d]\n", event.id);
        }
        break;
    }
    return status;

}
