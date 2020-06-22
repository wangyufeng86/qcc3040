/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_wait_pairing_request_state.c
\brief      Fast Pair Wait for Pairing Request State Event handling
*/


#include "fast_pair_wait_pairing_request_state.h"
#include "fast_pair_pairing_if.h"
#include "fast_pair_gfps.h"
#include "fast_pair_events.h"
#include "fast_pair_advertising.h"

static bool fastPair_SendKbPResponse(fast_pair_state_event_crypto_encrypt_args_t* args)
{
    bool status = FALSE;

    DEBUG_LOG("fastPair_SendKbPResponse");

    if(args->crypto_encrypt_cfm->status == success)
    {
        fastPair_SendFPNotification(FAST_PAIR_KEY_BASED_PAIRING, (uint8 *)args->crypto_encrypt_cfm->encrypted_data);
        
        fastPair_StartPairing();
        status = TRUE;
    }
    return status;
}

static bool fastPair_HandlePairingRequest(bool* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;
    
    DEBUG_LOG("fastPair_HandlePairingRequest");

    theFastPair = fastPair_GetTaskData();

    /* If TRUE, the Pairing request was receieved with I/O Capapbailities set to 
       DisplayKeyboard or DisplayYes/No. If False Remote I/O Capabalities was set
       to No Input No Output
     */
    if(*args == TRUE)
    {        
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_WAIT_PASSKEY);
        status = TRUE;
    }
    else
    {    
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
    }
    return status;
}

static bool fastpair_StateWaitPairingRequestHandleAuthCfm(fast_pair_state_event_auth_args_t* args)
{
    le_adv_data_set_t data_set;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

    if(args->auth_cfm->status == auth_status_success)
    {
        DEBUG_LOG("fastpair_StateWaitPairingRequestHandleAuthCfm. CL_SM_AUTHENTICATE_CFM status %d", args->auth_cfm->status);
        data_set = le_adv_data_set_handset_unidentifiable;
        fastPair_SetIdentifiable(data_set);

        /* After setting the identifiable parameter to unidentifiable, Set the FP state to idle */
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);

        return TRUE;
    }
    return FALSE;
}

static bool fastpair_StateWaitPairingRequestProcessACLDisconnect(fast_pair_state_event_disconnect_args_t* args)
{
    bool status = FALSE;
    uint8 index;
    fastPairTaskData *theFastPair;

    DEBUG_LOG("fastpair_StateWaitPairingRequestProcessACLDisconnect");

    theFastPair = fastPair_GetTaskData();

    if(args->disconnect_ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        memset(&theFastPair->rpa_bd_addr, 0x0, sizeof(bdaddr));

        for(index = 0; index < MAX_BLE_CONNECTIONS; index++)
        {
            if(BdaddrIsSame(&theFastPair->peer_bd_addr[index], &args->disconnect_ind->tpaddr.taddr.addr))
            {
                DEBUG_LOG("fastpair_StateWaitPairingRequestProcessACLDisconnect. Reseting peer BD address and own RPA of index %x", index);
                memset(&theFastPair->peer_bd_addr[index], 0x0, sizeof(bdaddr));
                memset(&theFastPair->own_random_address[index], 0x0, sizeof(bdaddr));
            }
        }
        status = TRUE;
    }
    return status;
}

bool fastPair_StateWaitPairingRequestHandleEvent(fast_pair_state_event_t event)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();
    DEBUG_LOG("fastPair_StateWaitPairingRequestHandleEvent event [%d]", event.id);
    /* Return if event is related to handset connection allowed/disallowed and is handled */
    if(fastPair_HandsetConnectStatusChange(event.id))
    {
        return TRUE;
    }

    switch (event.id)
    {
        case fast_pair_state_event_timer_expire:
        {
            fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
        }
        break;

        case fast_pair_state_event_crypto_encrypt:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastPair_SendKbPResponse((fast_pair_state_event_crypto_encrypt_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_pairing_request:
        {
            if(event.args == NULL)
            {
                return FALSE;
            }
            else
            {
                status = fastPair_HandlePairingRequest((bool *)event.args);
            }
        }
        break;

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
            status = fastpair_StateWaitPairingRequestHandleAuthCfm((fast_pair_state_event_auth_args_t *) event.args);
        }
        break;

        case fast_pair_state_event_disconnect:
        {
            if(event.args == NULL)
            {
                return FALSE;
            }
            status = fastpair_StateWaitPairingRequestProcessACLDisconnect((fast_pair_state_event_disconnect_args_t*)event.args);
        }
        break;

        default:
        {
            DEBUG_LOG("Unhandled event [%d]", event.id);
        }
        break;
    }
    return status;
}
