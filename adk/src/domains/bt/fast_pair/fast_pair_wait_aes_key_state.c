/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_wait_aes_key_state.c
\brief      Fast Pair Wait for AES Key State Event handling
*/
#include "fast_pair_wait_aes_key_state.h"
#include "fast_pair_pairing_if.h"
#include "bt_device.h"
#include "fast_pair_events.h"
#include "fast_pair_advertising.h"
#include "fast_pair_gfps.h"

#define FAST_PAIR_REQ_DISCOVERABILITY   (0x01)
#define FAST_PAIR_REQ_START_PAIRING     (0x02)

static bool fastPair_EcdhSharedSecretEventHandler(fast_pair_state_event_crypto_shared_secret_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    DEBUG_LOG("fastPair_EcdhSharedSecretEventHandler");

    theFastPair = fastPair_GetTaskData();

    if(args->crypto_shared_secret_cfm->status == success)
    {
        ConnectionEncryptBlockSha256(&theFastPair->task, args->crypto_shared_secret_cfm->shared_secret_key, (CL_CRYPTO_SHA_DATA_LEN*2));

        status = TRUE;
    }
    else
    {
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
    }
    return status;
}

static bool fastPair_CheckAESKey(fast_pair_state_event_crypto_hash_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;
    
    DEBUG_LOG("fastPair_CheckAESKey");

    theFastPair = fastPair_GetTaskData();

    if(args->crypto_hash_cfm->status == success)
    {
        memcpy(theFastPair->session_data.aes_key, args->crypto_hash_cfm->hash, FAST_PAIR_AES_KEY_LEN);
        ConnectionDecryptBlockAes(&theFastPair->task, (uint16 *)theFastPair->session_data.encrypted_data, theFastPair->session_data.aes_key);

        status = TRUE;
    }
    else
    {
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
    }
    return status;
}


static void fastPair_ConvertBigEndianBDAddress(bdaddr *device_addr, uint16 *decrypted_data, uint16 data_offset)
{
    uint8 *decrypted_packet_be = (uint8 *)decrypted_data;

    DEBUG_LOG("fastPair_ConvertBigEndianBDAddress");

    if(device_addr != NULL)
    {
         device_addr->nap = (uint16)(decrypted_packet_be[data_offset]<<8)| decrypted_packet_be[data_offset+1];
         device_addr->uap = (uint8)(decrypted_packet_be[data_offset+2]);
         device_addr->lap = (uint32)(decrypted_packet_be[data_offset+3] & 0xFF) << 16 | (uint32)(decrypted_packet_be[data_offset+4]) << 8 | (uint32)decrypted_packet_be[data_offset+5];
    }
}

static bool fastPair_MatchProviderAddress(uint16 *decrypted_data)
{
    bool status = FALSE;
    uint8 index;
    bdaddr provider_addr;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

    fastPair_ConvertBigEndianBDAddress(&provider_addr, decrypted_data, FAST_PAIR_PROVIDER_ADDRESS_OFFSET);

    for(index = 0; index < MAX_BLE_CONNECTIONS; index++)
    {
        /*! Check if the provider adress is matching with any random address entry in the table */
        if(BdaddrIsSame(&theFastPair->own_random_address[index], &provider_addr))
        {
            /*! Copy the random address */
            memcpy(&theFastPair->rpa_bd_addr, &theFastPair->own_random_address[index], sizeof(bdaddr));
            status = TRUE;
        }
    }

    DEBUG_LOG("Provider addr provided by FP Seeker %04x%02x%06lx\n", provider_addr.nap, provider_addr.uap, provider_addr.lap);
    DEBUG_LOG("Local BLE Address %04x%02x%06lx\n", theFastPair->rpa_bd_addr.nap, theFastPair->rpa_bd_addr.uap, theFastPair->rpa_bd_addr.lap);

    if(status == FALSE)
    {
        DEBUG_LOG("Fast Pair provider addr mismatch!");
    }
    return status;
}

static void fastPair_ClearProcessedAccountKeys(void)
{
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

    /* Free fetched account keys during subsequent pairing */
    free(theFastPair->session_data.account_key.keys);
    theFastPair->session_data.account_key.keys = NULL;
    theFastPair->session_data.account_key.num_keys = 0;
    theFastPair->session_data.account_key.num_keys_processed = 0;
}

static uint8* fastPair_GenerateKbPResponse(void)
{
    uint8 *response = PanicUnlessMalloc(FAST_PAIR_ENCRYPTED_REQUEST_LEN);
    uint16 i;
    bdaddr local_addr;
    
    DEBUG_LOG("fastPair_GenerateKbPResponse");

    /* Check local addrss */
    appDeviceGetMyBdAddr(&local_addr);

    DEBUG_LOG("Local BD Address %04x%02x%06lx\n", local_addr.nap, local_addr.uap, local_addr.lap);

    response[0] = 0x01;
    response[1] = (local_addr.nap >> 8) & 0xFF;
    response[2] = local_addr.nap & 0xFF;
    response[3] = local_addr.uap;
    response[4] = (local_addr.lap >> 16) & 0xFF;
    response[5] = (local_addr.lap >> 8) & 0xFF;
    response[6] = local_addr.lap & 0xFF;
    for (i = 7; i < 16; i++)
    {
        response[i] = UtilRandom() & 0xFF;
    }

    return response;

}

static bool fastpair_StateWaitAESKeyHandleAuthCfm(fast_pair_state_event_auth_args_t* args)
{
    le_adv_data_set_t data_set;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

    if(args->auth_cfm->status == auth_status_success)
    {
        DEBUG_LOG("fastpair_StateWaitAESKeyHandleAuthCfm. CL_SM_AUTHENTICATE_CFM status %d", args->auth_cfm->status);
        data_set = le_adv_data_set_handset_unidentifiable;
        fastPair_SetIdentifiable(data_set);

        /* After setting the identifiable parameter to unidentifiable, Set the FP state to idle */
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);

        return TRUE;
    }
    return FALSE;
}

static bool fastPair_ValidateAESKey(fast_pair_state_event_crypto_decrypt_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;
    
    DEBUG_LOG("fastPair_ValidateAESKey");

    theFastPair = fastPair_GetTaskData();

    if(args->crypto_decrypt_cfm->status == success)
    {
        if(fastPair_MatchProviderAddress(args->crypto_decrypt_cfm->decrypted_data))
        {
            uint8* decrypted_data_be = (uint8 *)args->crypto_decrypt_cfm->decrypted_data;
            
            uint8* raw_response = fastPair_GenerateKbPResponse();

            /* Reset Failure count on KbP Decode Success */
            theFastPair->failure_count = 0;

            fastPair_SetState(theFastPair, FAST_PAIR_STATE_WAIT_PAIRING_REQUEST);
           
            if(theFastPair->session_data.account_key.num_keys)
            {
                fastPair_ClearProcessedAccountKeys();
            }

            /* If seeker request provider to be discoverable, become dicoverable for 10 seconds
               Discoverablity timeout handled with fast pair state key based pairing response timer
             */
            if(decrypted_data_be[1] & FAST_PAIR_REQ_DISCOVERABILITY)
            {
                /* FP seeker reqesting discoverability */
                theFastPair->session_data.discoverability_flag = fastPair_EnterDiscoverable(TRUE);
            }

            /* If seeker request provider to be initiate pairing, send pairing request to seeker 
               using BD Addr provided by seeker in encrypted KbP packet over BR/EDR Transport
             */
            if(decrypted_data_be[1] & FAST_PAIR_REQ_START_PAIRING)
            {
                bdaddr seeker_addr;

                fastPair_ConvertBigEndianBDAddress(&seeker_addr, args->crypto_decrypt_cfm->decrypted_data, FAST_PAIR_SEEKER_ADDRESS_OFFSET);

                fastPair_InitiatePairing(&seeker_addr);
            }
            
            /* Encrypt Raw KbP Response with AES Key */
            ConnectionEncryptBlockAes(&theFastPair->task, (uint16 *)raw_response, theFastPair->session_data.aes_key);
            
            free(raw_response);    

            
        }
        else
        {
            if(theFastPair->session_data.account_key.num_keys)
            {
                if(theFastPair->session_data.account_key.num_keys_processed < theFastPair->session_data.account_key.num_keys)
                {
                    /* Use next account key to decrypt KbP packet */
                     memcpy(theFastPair->session_data.aes_key, &theFastPair->session_data.account_key.keys[theFastPair->session_data.account_key.num_keys_processed * FAST_PAIR_AES_KEY_LEN], FAST_PAIR_AES_KEY_LEN);
                    ConnectionDecryptBlockAes(&theFastPair->task, (uint16 *)theFastPair->session_data.encrypted_data, theFastPair->session_data.aes_key);
                    theFastPair->session_data.account_key.num_keys_processed++;
                }
                else
                {
                    fastPair_ClearProcessedAccountKeys();

                    /* The counter is incremented here to adhere to failure
                    handling mechanism as per FastPair specification*/
                    theFastPair->failure_count++;

                    /* No Valid AES Key!. Free it and set fast Pair state to state to Idle */
                    fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
                }
            }
            else
            {
                /* The counter is incremented here to adhere to failure
                handling mechanism as per FastPair specification*/
                theFastPair->failure_count++;

                /* No Valid AES Key!. Free it and set fast Pair state to state to Idle */
                fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
            }
        }

        status = TRUE;
    }
    else
    {
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
    }
    return status;
}

static bool fastpair_StateWaitAESKeyProcessACLDisconnect(fast_pair_state_event_disconnect_args_t* args)
{
    bool status = FALSE;
    uint8 index;
    fastPairTaskData *theFastPair;

    DEBUG_LOG("fastpair_StateWaitAESKeyProcessACLDisconnect");

    theFastPair = fastPair_GetTaskData();

    if(args->disconnect_ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        memset(&theFastPair->rpa_bd_addr, 0x0, sizeof(bdaddr));

        for(index = 0; index < MAX_BLE_CONNECTIONS; index++)
        {
            if(BdaddrIsSame(&theFastPair->peer_bd_addr[index], &args->disconnect_ind->tpaddr.taddr.addr))
            {
                DEBUG_LOG("fastpair_StateWaitAESKeyProcessACLDisconnect. Reseting peer BD address and own RPA of index %x", index);
                memset(&theFastPair->peer_bd_addr[index], 0x0, sizeof(bdaddr));
                memset(&theFastPair->own_random_address[index], 0x0, sizeof(bdaddr));
            }
        }
        status = TRUE;
    }
    return status;
}

bool fastPair_StateWaitAESKeyHandleEvent(fast_pair_state_event_t event)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;
    
    theFastPair = fastPair_GetTaskData();

    DEBUG_LOG("fastPair_StateWaitAESKeyHandleEvent: EventID=%d", event.id);
    /* Return if event is related to handset connection allowed/disallowed and is handled */
    if(fastPair_HandsetConnectStatusChange(event.id))
    {
        return TRUE;
    }

    switch (event.id)
    {
        case fast_pair_state_event_crypto_shared_secret:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastPair_EcdhSharedSecretEventHandler((fast_pair_state_event_crypto_shared_secret_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_crypto_hash:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastPair_CheckAESKey((fast_pair_state_event_crypto_hash_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_crypto_decrypt:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastPair_ValidateAESKey((fast_pair_state_event_crypto_decrypt_args_t *)event.args);
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
            status = fastpair_StateWaitAESKeyHandleAuthCfm((fast_pair_state_event_auth_args_t *) event.args);
        }
        break;

        case fast_pair_state_event_disconnect:
        {
            if(event.args == NULL)
            {
                return FALSE;
            }
            status = fastpair_StateWaitAESKeyProcessACLDisconnect((fast_pair_state_event_disconnect_args_t*)event.args);
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
