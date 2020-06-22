/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_wait_passkey_state.c
\brief      Fast Pair Wait for Passkey State Event handling
*/


#include "fast_pair_wait_passkey_state.h"
#include "fast_pair_gfps.h"
#include "fast_pair_pairing_if.h"
#include "fast_pair_session_data.h"
#include "fast_pair_account_key_sync.h"
#include "fast_pair_events.h"


static bool pairing_successful = FALSE;

static bool fastPair_ValidatePasskey(fast_pair_state_event_crypto_decrypt_args_t* args)
{
    bool status = FALSE;
    uint32 seeker_passkey = 0;
    fastPairTaskData *theFastPair;
    uint8* decrypted_data_be = (uint8 *)args->crypto_decrypt_cfm->decrypted_data;

    theFastPair = fastPair_GetTaskData();
    
    if (decrypted_data_be[0] != fast_pair_passkey_seeker)
    {
        /* We failed to decrypt passkey */
        DEBUG_LOG("Failed to decrypt passkey!\n");

        /* AES Key Not Valid!. Free it and set fast Pair state to state to Idle */
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
    }
    else
    {
        seeker_passkey = ((uint32)decrypted_data_be[1] << 16 ) | 
                        ((uint32)decrypted_data_be[2] << 8 ) |
                            (uint32)decrypted_data_be[3];

        status = TRUE;
    }

    fastPair_PairingPasskeyReceived(seeker_passkey);
    
    return status;

}

static bool fastPair_SendPasskeyResponse(fast_pair_state_event_crypto_encrypt_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

    if(args->crypto_encrypt_cfm->status == success)
    {
        fastPair_SendFPNotification(FAST_PAIR_PASSKEY, (uint8 *)args->crypto_encrypt_cfm->encrypted_data);
        
        fastPair_PairingReset();
        status = TRUE;
    }
    
    if(pairing_successful)
    {
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_WAIT_ACCOUNT_KEY);
    }
    else
    {
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
    }
    return status;
}

static bool fastpair_PasskeyWriteEventHandler(fast_pair_state_event_passkey_write_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();
    
    if(args->enc_data != NULL)
    {
        /* Decrypt passkey Block */
        ConnectionDecryptBlockAes(&theFastPair->task, (uint16 *)args->enc_data, theFastPair->session_data.aes_key);
        status = TRUE;
    }
    return status;
}

static uint8* fastPair_GeneratePasskeyResponse(uint32 provider_passkey)
{
    uint8 *response = PanicUnlessMalloc(FAST_PAIR_ENCRYPTED_PASSKEY_BLOCK_LEN);
    uint16 i;


    response[0] = fast_pair_passkey_provider;
    response[1] = (provider_passkey >> 16) & 0xFF;
    response[2] = (provider_passkey >> 8) & 0xFF;
    response[3] = provider_passkey & 0xFF;
    for (i = 4; i < 16; i++)
    {
        response[i] = UtilRandom() & 0xFF;
    }

    return response;
}

static bool fastpair_ProviderPasskeyEventHandler(fast_pair_state_event_provider_passkey_write_args_t* args)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

    uint8* passkey_response =  fastPair_GeneratePasskeyResponse(args->provider_passkey);

     if(args->isPasskeySame)
     {
         /* Pairing was successful. If Pairing was done using account key pairing, update the account key table */
         if(theFastPair->session_data.public_key == NULL)
         {
             /* The account key will be present in account key list already, De-duplication logic 
                will take care of updating list to track most recently used account keys 
              */
             fastPair_StoreAccountKey((uint8 *)theFastPair->session_data.aes_key);
             /*! Account Key Sharing*/
             fastPair_AccountKeySync_Sync();
         }  
         pairing_successful = TRUE;
         status = TRUE;
     }
     else
     {
         /* We failed to match passkey */
         DEBUG_LOG("Failed to match passkey!\n");
         
         pairing_successful = FALSE;
     }
     
    /* Encrypt Raw Passkey Response with AES Key */
    ConnectionEncryptBlockAes(&theFastPair->task, (uint16 *)passkey_response, theFastPair->session_data.aes_key);  

    free(passkey_response);

    return status;
}

static bool fastpair_StateWaitPasskeyProcessACLDisconnect(fast_pair_state_event_disconnect_args_t* args)
{
    bool status = FALSE;
    uint8 index;
    fastPairTaskData *theFastPair;

    DEBUG_LOG("fastpair_StateWaitPasskeyProcessACLDisconnect");

    theFastPair = fastPair_GetTaskData();

    if(args->disconnect_ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        memset(&theFastPair->rpa_bd_addr, 0x0, sizeof(bdaddr));

        for(index = 0; index < MAX_BLE_CONNECTIONS; index++)
        {
            if(BdaddrIsSame(&theFastPair->peer_bd_addr[index], &args->disconnect_ind->tpaddr.taddr.addr))
            {
                DEBUG_LOG("fastpair_StateWaitPasskeyProcessACLDisconnect. Reseting peer BD address and own RPA of index %x", index);
                memset(&theFastPair->peer_bd_addr[index], 0x0, sizeof(bdaddr));
                memset(&theFastPair->own_random_address[index], 0x0, sizeof(bdaddr));
            }
        }
        status = TRUE;
    }
    return status;
}

bool fastPair_StateWaitPasskeyHandleEvent(fast_pair_state_event_t event)
{
    bool status = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();
    DEBUG_LOG("fastPair_StateWaitPasskeyHandleEvent event [%d]", event.id);
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
            status = TRUE;
        }
        break;
        
        case fast_pair_state_event_crypto_decrypt:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastPair_ValidatePasskey((fast_pair_state_event_crypto_decrypt_args_t *)event.args);
        }
        break;
        
        case fast_pair_state_event_crypto_encrypt:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastPair_SendPasskeyResponse((fast_pair_state_event_crypto_encrypt_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_passkey_write:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastpair_PasskeyWriteEventHandler((fast_pair_state_event_passkey_write_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_provider_passkey:
        {
            if (event.args == NULL)
            {
                return FALSE;
            }
            status = fastpair_ProviderPasskeyEventHandler((fast_pair_state_event_provider_passkey_write_args_t *)event.args);
        }
        break;

        case fast_pair_state_event_disconnect:
        {
            if(event.args == NULL)
            {
                return FALSE;
            }
            status = fastpair_StateWaitPasskeyProcessACLDisconnect((fast_pair_state_event_disconnect_args_t*)event.args);
        }
        break;

        case fast_pair_state_event_power_off:
        {
            fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
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
