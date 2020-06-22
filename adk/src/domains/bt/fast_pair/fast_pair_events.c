/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_events.c
\brief      Fast Pair Events
*/

#include "fast_pair_events.h"
#include "fast_pair_advertising.h"


/*! \brief Handle messages from Connection Library */
bool fastPair_AuthenticateCfm(CL_SM_AUTHENTICATE_CFM_T *cfm)
{
    fast_pair_state_event_auth_args_t args;
    fast_pair_state_event_t event;

    /* Assign args to SM Auth event */
    args.auth_cfm = cfm;

    event.id = fast_pair_state_event_auth;
    event.args = &args;

    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle messages from Connection Manager */
bool fastPair_ConManagerConnectInd(CON_MANAGER_TP_CONNECT_IND_T *cfm)
{
    fast_pair_state_event_connect_args_t args;
    fast_pair_state_event_t event;
    
    /* Assign args to the BLE connect/disconnect event */
    args.connect_ind = cfm;
    
    event.id = fast_pair_state_event_connect;
    event.args = &args;
    
    return fastPair_StateMachineHandleEvent(event);
}


/*! \brief Handle messages from Connection Manager */
bool fastPair_ConManagerDisconnectInd(CON_MANAGER_TP_DISCONNECT_IND_T *cfm)
{
    fast_pair_state_event_disconnect_args_t args;
    fast_pair_state_event_t event;
    
    /* Assign args to the BLE connect/disconnect event */
    args.disconnect_ind = cfm;
    
    event.id = fast_pair_state_event_disconnect;
    event.args = &args;
    
    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle messages from Connection Manager */
bool fastPair_ConManagerHandsetConnectAllowInd(void)
{
    fast_pair_state_event_t event = {fast_pair_state_event_handset_connect_allow, NULL};

    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle messages from Conenction Manager */
bool fastPair_ConManagerHandsetConnectDisallowInd(void)
{
    fast_pair_state_event_t event = {fast_pair_state_event_handset_connect_disallow, NULL};

    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle messages from Conenction Message Dispatcher */
bool fastPair_CacheRandomAddressCfm(const CL_SM_BLE_READ_RANDOM_ADDRESS_CFM_T *cfm)
{
    fast_pair_state_event_rpa_addr_args_t args;
    fast_pair_state_event_t event;
    
    /* Assign args to the RPA Address event */
    args.rpa_cfm = cfm;
    
    event.id = fast_pair_state_event_rpa_addr;
    event.args = &args;
    
    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle Crypto ECDH Shared Secret confirm */
bool fastPair_SharedSecretCfm(CL_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM_T *cfm)
{
    fast_pair_state_event_crypto_shared_secret_args_t args;
    fast_pair_state_event_t event;
    
    /* Assign args to the ECDH Shared Secret event */
    args.crypto_shared_secret_cfm = cfm;
    
    event.id = fast_pair_state_event_crypto_shared_secret;
    event.args = &args;
    
    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle Crypto Hash confirm */
bool fastPair_HashCfm(CL_CRYPTO_HASH_CFM_T *cfm)
{
    fast_pair_state_event_crypto_hash_args_t args;
    fast_pair_state_event_t event;
    
    /* Assign args to the Hash256 event */
    args.crypto_hash_cfm = cfm;
    
    event.id = fast_pair_state_event_crypto_hash;
    event.args = &args;
    
    return fastPair_StateMachineHandleEvent(event);
    
}

/*! \brief Handle Crypto AES Encrypt confirm */
bool fastPair_EncryptCfm(CL_CRYPTO_ENCRYPT_CFM_T *cfm)
{
    fast_pair_state_event_crypto_encrypt_args_t args;
    fast_pair_state_event_t event;
    
    /* Assign args to the AES Encrypt event */
    args.crypto_encrypt_cfm = cfm;
    
    event.id = fast_pair_state_event_crypto_encrypt;
    event.args = &args;
    
    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle Crypto AES Encrypt confirm */
bool fastPair_DecryptCfm(CL_CRYPTO_DECRYPT_CFM_T *cfm)
{
    fast_pair_state_event_crypto_decrypt_args_t args;
    fast_pair_state_event_t event;
    
    /* Assign args to the AES Encrypt event */
    args.crypto_decrypt_cfm = cfm;
    
    event.id = fast_pair_state_event_crypto_decrypt;
    event.args = &args;
    
    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle Key Based Pairing Characterstic Write by Fast Pair Seeker */
void fastPair_KeyBasedPairingWrite(const uint16 cid, const uint8 *enc_data, uint32 enc_data_len)
{
    fast_pair_state_event_kbp_write_args_t args;
    fast_pair_state_event_t event;
    
    /* Assign args to the KbP write event */
    args.cid = cid;
    args.enc_data = enc_data;
    args.size = enc_data_len;
    
    event.id = fast_pair_state_event_kbp_write;
    event.args = &args;
    
    fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle Passkey Characterstic Write by Fast Pair Seeker */
void fastPair_PasskeyWrite(const uint16 cid, const uint8 *enc_data, uint32 enc_data_len)
{
    fast_pair_state_event_passkey_write_args_t args;
    fast_pair_state_event_t event;
    
    /* Assign args to the passkey write event */
    args.cid = cid;
    args.enc_data = enc_data;
    args.size = enc_data_len;
    
    event.id = fast_pair_state_event_passkey_write;
    event.args = &args;
    
    fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle Account Key Characterstic Write by Fast Pair Seeker */
void fastPair_AccountkeyWrite(const uint16 cid, const uint8 *enc_data, uint32 enc_data_len)
{
    fast_pair_state_event_account_key_write_args_t args;
    fast_pair_state_event_t event;

    /* Assign args to the account key write event */
    args.cid = cid;
    args.enc_data = enc_data;
    args.size = enc_data_len;

    event.id = fast_pair_state_event_account_key_write;
    event.args = &args;
    
    fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle New Pairing Request */
void fastPair_ReceivedPairingRequest(bool accept)
{
    fast_pair_state_event_t event;

    event.id = fast_pair_state_event_pairing_request;
    event.args = &accept;
    
    fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle Provider Passkey Generated Event*/
void fastPair_ProviderPasskey(bool isPasskeySame, uint32 provider_passkey)
{
    fast_pair_state_event_provider_passkey_write_args_t args;
    fast_pair_state_event_t event;

    args.isPasskeySame = isPasskeySame;
    args.provider_passkey = provider_passkey;
    event.id = fast_pair_state_event_provider_passkey;
    event.args = &args;
    
    fastPair_StateMachineHandleEvent(event);
}


/*! \brief Handle FP Timeout Event */
bool fastPair_TimerExpired(void)
{
    fast_pair_state_event_t event = {fast_pair_state_event_timer_expire, NULL};
    
    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle Power Off Event */
bool fastPair_PowerOff(void)
{
    fast_pair_state_event_t event = {fast_pair_state_event_power_off, NULL};
    
    return fastPair_StateMachineHandleEvent(event);
}

/*! \brief Handle the handset connection allow/disallow event from connection manager */
bool fastPair_HandsetConnectStatusChange(fast_pair_state_event_id event_id)
{
    bool status = FALSE;

    switch(event_id)
    {
        case fast_pair_state_event_handset_connect_allow:
        {
            status = fastPair_AdvNotifyChangeInConnectableState(CON_MANAGER_HANDSET_CONNECT_ALLOW_IND);
        }
        break;

        case fast_pair_state_event_handset_connect_disallow:
        {
            status = fastPair_AdvNotifyChangeInConnectableState(CON_MANAGER_HANDSET_CONNECT_DISALLOW_IND);
        }
        break;

        default:
            DEBUG_LOG("FP Events:  Invalid event id in fastPair_HandsetConnectStatusChange ");
        break;
    }

    return status;
}

/*! \brief Handle the Pairing activity messages from pairing module */
void fastPair_PairingActivity(PAIRING_ACTIVITY_T *message)
{
    switch(message->status)
    {
        case pairingInProgress:
        {
            fastPair_SetIdentifiable(le_adv_data_set_handset_identifiable);
        }
        break;

        case pairingSuccess:
        case pairingTimeout:
        case pairingStopped:
        case pairingFailed:    
        case pairingNotInProgress:
        {
            fastPair_SetIdentifiable(le_adv_data_set_handset_unidentifiable);
        }
        break;

        default:
            DEBUG_LOG("FP Events:  Invalid message id in fastPair_PairingActivity ");
        break;
    }

}

