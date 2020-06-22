/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_idle_state.c
\brief      Fast Pair Idle State Event handling
*/

#include "fast_pair_idle_state.h"
#include "fast_pair_bloom_filter.h"
#include "fast_pair_advertising.h"
#include "fast_pair_session_data.h"
#include "fast_pair_events.h"

#include <panic.h>

static bool fastpair_ProcessACLConnect(fast_pair_state_event_connect_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair = fastPair_GetTaskData();

    DEBUG_LOG("fastpair_ProcessACLConnect");

    if(args->connect_ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        /* New BLE ACL was openned. Read RPA used to make connection */
        ConnectionSmBleReadRandomAddressTaskReq(&theFastPair->task, ble_read_random_address_local, &(args->connect_ind->tpaddr));
        status = TRUE;
    }

    return status;
}

static bool fastpair_ProcessACLDisconnect(fast_pair_state_event_disconnect_args_t* args)
{
    bool status = FALSE;
    uint8 index;
    fastPairTaskData *theFastPair;

    DEBUG_LOG("fastpair_ProcessACLDisconnect");

    theFastPair = fastPair_GetTaskData();

    if(args->disconnect_ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        memset(&theFastPair->rpa_bd_addr, 0x0, sizeof(bdaddr));

        for(index = 0; index < MAX_BLE_CONNECTIONS; index++)
        {
            if(BdaddrIsSame(&theFastPair->peer_bd_addr[index], &args->disconnect_ind->tpaddr.taddr.addr))
            {
                DEBUG_LOG("fastpair_ProcessACLDisconnect. Reseting peer BD address and own RPA of index %x", index);
                memset(&theFastPair->peer_bd_addr[index], 0x0, sizeof(bdaddr));
                memset(&theFastPair->own_random_address[index], 0x0, sizeof(bdaddr));
            }
        }
        status = TRUE;
    }

    return status;
}

static bool fastpair_StateIdleHandleAuthCfm(fast_pair_state_event_auth_args_t* args)
{
    le_adv_data_set_t data_set;
    if(args->auth_cfm->status == auth_status_success)
    {
        DEBUG_LOG("fastpair_StateIdleHandleAuthCfm. CL_SM_AUTHENTICATE_CFM status %d", args->auth_cfm->status);
        data_set = le_adv_data_set_handset_unidentifiable;
        fastPair_SetIdentifiable(data_set);
        return TRUE;
    }
    return FALSE;
}

static bool fastpair_ReadRandomAddressEventHandler(fast_pair_state_event_rpa_addr_args_t* args)
{
    bool status = FALSE;
    uint8 index;
    fastPairTaskData *theFastPair;

    DEBUG_LOG("fastpair_ReadRandomAddressEventHandler");

    theFastPair = fastPair_GetTaskData();

    if((args->rpa_cfm->status == success) && (args->rpa_cfm->flags == ble_read_random_address_local))
    {
        memcpy(&theFastPair->rpa_bd_addr, &(args->rpa_cfm->random_tpaddr.taddr.addr), sizeof(bdaddr));

        for(index = 0; index < MAX_BLE_CONNECTIONS; index++)
        {
            /*! Duplicate Peer BD address case, update newly generated own random address */
            if(BdaddrIsSame(&theFastPair->peer_bd_addr[index], &args->rpa_cfm->peer_tpaddr.taddr.addr))
            {
                DEBUG_LOG("fastpair_ReadRandomAddressEventHandler. Duplicate Peer BD address case");
                memcpy(&theFastPair->own_random_address[index], &(args->rpa_cfm->random_tpaddr.taddr.addr), sizeof(bdaddr));

                DEBUG_LOG("Peer BD Addr %04x%02x%06lx", theFastPair->peer_bd_addr[index].nap, theFastPair->peer_bd_addr[index].uap,
                                        theFastPair->peer_bd_addr[index].lap);
                DEBUG_LOG("Own RPA %04x%02x%06lx", theFastPair->own_random_address[index].nap, theFastPair->own_random_address[index].uap,
                                        theFastPair->own_random_address[index].lap);
                break;
            }
            /*! If peer BD address is all zero, update the table with new peer connetion data */
            else if(BdaddrIsZero(&theFastPair->peer_bd_addr[index]))
            {
                DEBUG_LOG("fastpair_ReadRandomAddressEventHandler. Updating table with new peer connection data");
                memcpy(&theFastPair->peer_bd_addr[index], &(args->rpa_cfm->peer_tpaddr.taddr.addr), sizeof(bdaddr));
                memcpy(&theFastPair->own_random_address[index], &(args->rpa_cfm->random_tpaddr.taddr.addr), sizeof(bdaddr));
                
                DEBUG_LOG("Peer BD Addr %04x%02x%06lx", theFastPair->peer_bd_addr[index].nap, theFastPair->peer_bd_addr[index].uap,
                                        theFastPair->peer_bd_addr[index].lap);
                DEBUG_LOG("Own RPA %04x%02x%06lx", theFastPair->own_random_address[index].nap, theFastPair->own_random_address[index].uap,
                                        theFastPair->own_random_address[index].lap);
                break;
            }
        }
        status = TRUE;
    }
    return status;
}


static bool fastPair_HandleAdvBloomFilterCalc(fast_pair_state_event_crypto_hash_args_t* args)
{
    DEBUG_LOG("fastPair_HandleAdvBloomFilterCalc");

    if(args->crypto_hash_cfm->status == success)
    {
        fastPair_AdvHandleHashCfm(args->crypto_hash_cfm);
        return TRUE;
    }
    return FALSE;
}

static void fastPair_AllocatePublicPrivateKeyMemory(void)
{
    fastPairTaskData *theFastPair;
    theFastPair = fastPair_GetTaskData();

    theFastPair->session_data.private_key = PanicUnlessMalloc(FAST_PAIR_PRIVATE_KEY_LEN);
    theFastPair->session_data.public_key = PanicUnlessMalloc(FAST_PAIR_PUBLIC_KEY_LEN);
}

static void fastPair_AllocateSessionDataMemory(void)
{
    fastPairTaskData *theFastPair;
    theFastPair = fastPair_GetTaskData();

    theFastPair->session_data.encrypted_data = PanicUnlessMalloc(FAST_PAIR_ENCRYPTED_REQUEST_LEN);
    theFastPair->session_data.aes_key = PanicUnlessMalloc(FAST_PAIR_AES_KEY_LEN);
}

static bool fastpair_KeyBasedPairingWriteEventHandler(fast_pair_state_event_kbp_write_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    DEBUG_LOG("fastpair_KeyBasedPairingWriteEventHandler");

    theFastPair = fastPair_GetTaskData();
	
    /* If Fast pair seeker has written public key along with encrypted request,
       use the public private key ECDH algorithm to arrive at the AES key
     */
    if(args->size == (FAST_PAIR_ENCRYPTED_REQUEST_LEN+FAST_PAIR_PUBLIC_KEY_LEN))
    {
        /* Proceed with Public/Private key based fast pair procedure only if we are in discoverable mode */
        if(fastPair_AdvIsBrEdrDiscoverable())
        {
            /* Update state so that no new requests are processed till current fast pair seesion is over */
            fastPair_SetState(theFastPair, FAST_PAIR_STATE_WAIT_AES_KEY);

            fastPair_AllocatePublicPrivateKeyMemory();
            fastPair_AllocateSessionDataMemory();

            fastPair_GetAntiSpoofingPrivateKey((uint8 *)theFastPair->session_data.private_key);

            memcpy(theFastPair->session_data.encrypted_data, args->enc_data, FAST_PAIR_ENCRYPTED_REQUEST_LEN);
            memcpy(theFastPair->session_data.public_key, &(args->enc_data[FAST_PAIR_ENCRYPTED_REQUEST_LEN]), FAST_PAIR_PUBLIC_KEY_LEN);

            ConnectionGenerateSharedSecretKey(&theFastPair->task, cl_crypto_ecc_p256, theFastPair->session_data.private_key, theFastPair->session_data.public_key);

            status = TRUE;
        }
    }
    else if(args->size == FAST_PAIR_ENCRYPTED_REQUEST_LEN)
    {
       /* If only encrypted request is written to KbP, then try to find an account key
          from account key list which can function as AES key to decode encrypted packets
        */
        fastPair_AllocateSessionDataMemory();

        theFastPair->session_data.account_key.num_keys = fastPair_GetAccountKeys(&theFastPair->session_data.account_key.keys);

        if(theFastPair->session_data.account_key.num_keys)
        {
            /* Update state so that no new requests are processed till current fast pair seesion is over */
            fastPair_SetState(theFastPair, FAST_PAIR_STATE_WAIT_AES_KEY);

            theFastPair->session_data.account_key.num_keys_processed = 0;
            memcpy(theFastPair->session_data.encrypted_data, args->enc_data, FAST_PAIR_ENCRYPTED_REQUEST_LEN);

            /* Use account keys to decrypt Kbp packet. num_keys_processed starts from ZERO indiacting first account key stored */
             memcpy(theFastPair->session_data.aes_key, &theFastPair->session_data.account_key.keys[theFastPair->session_data.account_key.num_keys_processed * FAST_PAIR_AES_KEY_LEN], FAST_PAIR_AES_KEY_LEN);
            ConnectionDecryptBlockAes(&theFastPair->task, (uint16 *)theFastPair->session_data.encrypted_data, theFastPair->session_data.aes_key);

            theFastPair->session_data.account_key.num_keys_processed++;

            status = TRUE;
        }
    }
    else
    {
         /* The error counter is incremented here to adhere to failure
             handling mechanism as per FastPair specification
          */
        theFastPair->failure_count++;
    }
    return status;
}


bool fastPair_StateIdleHandleEvent(fast_pair_state_event_t event)
{
    bool status = FALSE;

    DEBUG_LOG("fastPair_StateIdleHandleEvent: event [%d]", event.id);

    status =fastPair_HandsetConnectStatusChange(event.id);
    /* Return if event is related to handset connection allowed/disallowed and is handled */
    if(status)
    {
        return status;
    }

    if (event.args == NULL)
    {
        return FALSE;
    }

    switch (event.id)
    {

        case fast_pair_state_event_connect:
        {
            status = fastpair_ProcessACLConnect((fast_pair_state_event_connect_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_disconnect:
        {
            status = fastpair_ProcessACLDisconnect((fast_pair_state_event_disconnect_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_rpa_addr:
        {
            status = fastpair_ReadRandomAddressEventHandler((fast_pair_state_event_rpa_addr_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_crypto_hash:
        {
            status = fastPair_HandleAdvBloomFilterCalc((fast_pair_state_event_crypto_hash_args_t *)event.args);
        }
        break;
        
        case fast_pair_state_event_kbp_write:
        {
            status = fastpair_KeyBasedPairingWriteEventHandler((fast_pair_state_event_kbp_write_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_auth:
        {
            status = fastpair_StateIdleHandleAuthCfm((fast_pair_state_event_auth_args_t *) event.args);
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
