/*******************************************************************************
Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    fast_pair_events.h

DESCRIPTION
    Event Handled in Fast Pair Module.
*/

#ifndef FAST_PAIR_EVENTS_H_
#define FAST_PAIR_EVENTS_H_

#include "fast_pair.h"

/* Event arguments for the KbP write event */
typedef struct
{
    uint16 cid;             
    const uint8* enc_data;
    uint16 size;
} fast_pair_state_event_kbp_write_args_t;

/* Event arguments for the Passkey write event */
typedef struct
{
    uint16 cid;             
    const uint8* enc_data;
    uint16 size;
} fast_pair_state_event_passkey_write_args_t;


/* Event arguments for the account key write event */
typedef struct
{
    uint16 cid;             
    const uint8* enc_data;
    uint16 size;
} fast_pair_state_event_account_key_write_args_t;


/* Event arguments for the provider passkey event */
typedef struct
{
    bool isPasskeySame;             
    uint32 provider_passkey;
} fast_pair_state_event_provider_passkey_write_args_t;

/* Event arguments for ACL connect event */
typedef struct
{
    CON_MANAGER_TP_CONNECT_IND_T* connect_ind;
} fast_pair_state_event_connect_args_t;

/* Event arguments for ACL disconnect event */
typedef struct
{
    CON_MANAGER_TP_DISCONNECT_IND_T* disconnect_ind;
} fast_pair_state_event_disconnect_args_t;


/* Event arguments for Read RPA Address event */
typedef struct
{
    const CL_SM_BLE_READ_RANDOM_ADDRESS_CFM_T* rpa_cfm;
} fast_pair_state_event_rpa_addr_args_t;

/* Event arguments for Crypto shared secret event */
typedef struct
{
    CL_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM_T* crypto_shared_secret_cfm;
} fast_pair_state_event_crypto_shared_secret_args_t;

/* Event arguments for Crypto Hash event */
typedef struct
{
    CL_CRYPTO_HASH_CFM_T* crypto_hash_cfm;
} fast_pair_state_event_crypto_hash_args_t;

/* Event arguments for Crypto AES Encrypt event */
typedef struct
{
    CL_CRYPTO_ENCRYPT_CFM_T* crypto_encrypt_cfm;
} fast_pair_state_event_crypto_encrypt_args_t;

/* Event arguments for Crypto AES Decrypt event */
typedef struct
{
    CL_CRYPTO_DECRYPT_CFM_T* crypto_decrypt_cfm;
} fast_pair_state_event_crypto_decrypt_args_t;

/*! \brief Handle messages from connection library */
bool fastPair_AuthenticateCfm(CL_SM_AUTHENTICATE_CFM_T *cfm);

/*! \brief Handle messages from Conenction Manager */
bool fastPair_ConManagerConnectInd(CON_MANAGER_TP_CONNECT_IND_T *cfm);

/*! \brief Handle messages from Conenction Manager */
bool fastPair_ConManagerDisconnectInd(CON_MANAGER_TP_DISCONNECT_IND_T *cfm);

/*! \brief Handle messages from Conenction Message Dispatcher */
bool fastPair_CacheRandomAddressCfm(const CL_SM_BLE_READ_RANDOM_ADDRESS_CFM_T *cfm);

/*! \brief Handle Crypto ECDH Shared Secret confirm */
bool fastPair_SharedSecretCfm(CL_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM_T *cfm);

/*! \brief Handle Crypto Hash confirm */
bool fastPair_HashCfm(CL_CRYPTO_HASH_CFM_T *cfm);

/*! \brief Handle Crypto AES Encrypt confirm */
bool fastPair_EncryptCfm(CL_CRYPTO_ENCRYPT_CFM_T *cfm);

/*! \brief Handle Crypto AES Encrypt confirm */
bool fastPair_DecryptCfm(CL_CRYPTO_DECRYPT_CFM_T *cfm);


/*! \brief Handle Key Based Pairing Characterstic Write by Fast Pair Seeker

    Establish AES and decode encrypted data written to Key based Pairing  characterstic.
    The AES key can be derived by using Public-Private key or by using one of the account
    keys to successfully decode the encrypted packet.

    \param[in] cid  connection ID of the LE Connection.
    \param[in] enc_data Pointer to encrypted data written by FP seeker.
    \param[in] enc_data_len Length of encrypted data written by FP seeker.
 */
void fastPair_KeyBasedPairingWrite(const uint16 cid, const uint8 *enc_data, uint32 enc_data_len);

/*! \brief Handle Passkey Characterstic Write by Fast Pair Seeker

    Use AES Key to decode passkey written in encrypted format.

    \param[in] cid  connection ID of the LE Connection.
    \param[in] enc_data Pointer to encrypted data written by FP seeker.
    \param[in] enc_data_len Length of encrypted data written by FP seeker.
 */
void fastPair_PasskeyWrite(const uint16 cid, const uint8 *enc_data, uint32 enc_data_len);

/*! \brief Handle Account Key Characterstic Write by Fast Pair Seeker

    Use AES Key to decode Account key written in encrypted format.

    \param[in] cid  connection ID of the LE Connection.
    \param[in] enc_data Pointer to encrypted data written by FP seeker.
    \param[in] enc_data_len Length of encrypted data written by FP seeker.
 */
void fastPair_AccountkeyWrite(const uint16 cid, const uint8 *enc_data, uint32 enc_data_len);

/*! \brief Handle New Pairing Request */
void fastPair_ReceivedPairingRequest(bool accept);

/*! \brief Handle Provider Passkey Generated Event*/
void fastPair_ProviderPasskey(bool isPasskeySame, uint32 provider_passkey);

/*! \brief Handle FP Timer expired Event */
bool fastPair_TimerExpired(void);

/*! \brief Handle Power Off Event */
bool fastPair_PowerOff(void);

/*! \brief Handle handset connect allow indication */
bool fastPair_ConManagerHandsetConnectAllowInd(void);

/*! \brief Handle handset connect disallow indication */
bool fastPair_ConManagerHandsetConnectDisallowInd(void);

/*! \brief Handle handset connection status change */
bool fastPair_HandsetConnectStatusChange(fast_pair_state_event_id event_id);

/*! \brief Handle the Pairing activity messages from pairing module */
void fastPair_PairingActivity(PAIRING_ACTIVITY_T *message);

#endif
